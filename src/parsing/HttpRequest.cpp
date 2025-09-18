#include "HttpRequest.hpp"
#include <sys/socket.h>
#include <sys/stat.h> 
#include <unistd.h>
#include "cgiHandler.hpp"
#include <algorithm>

HttpRequest::HttpRequest(): clientSocket(-1), statusCode(OK)
{
}

HttpRequest::HttpRequest(int socket): clientSocket(socket), statusCode(OK)
{
}

HttpRequest::HttpRequest(const HttpRequest &other) : method(other.method),
	path(other.path), version(other.version), headers(other.headers),
	body(other.body), statusCode(other.statusCode)
{
}

HttpRequest &HttpRequest::operator=(const HttpRequest &other)
{
	if (this != &other)
	{
		method = other.method;
		path = other.path;
		version = other.version;
		headers = other.headers;
		body = other.body;
		statusCode = other.statusCode;
	}
	return (*this);
}

HttpRequest::~HttpRequest()
{
}

std::string HttpRequest::getMethod() const
{
	return (method);
}
std::string HttpRequest::getPath() const
{
	return (path);
}
std::string HttpRequest::getVersion() const
{
	return (version);
}
std::map<std::string, std::string> HttpRequest::getHeaders() const
{
	return (headers);
}
std::string HttpRequest::getBody() const
{
	return (body);
}

HttpStatus HttpRequest::getStatusCode() const
{
	return (statusCode);
}

void HttpRequest::setHeader(const std::string &key, const std::string &value) {
    headers[key] = value;
}

void HttpRequest::setMethod(const std::string& method) {
    this->method = method;
}

void HttpRequest::setPath(const std::string& path) {
    this->path = path;
}

void HttpRequest::setHeaders(const std::map<std::string, std::string>& headers) {
    this->headers = headers;
}

void HttpRequest::sendContinueResponse() {
    const std::string continueResponse = "HTTP/1.1 100 Continue\r\n\r\n";
    send(clientSocket, continueResponse.c_str(), continueResponse.size(), 0);
}

void HttpRequest::setStatusCode(HttpStatus code)
{
	statusCode = code;
}

void HttpRequest::setResolvedPath(const std::string &resolvedPath) {
    this->resolvedPath = resolvedPath;
}

const std::string& HttpRequest::getResolvedPath() const {
    return resolvedPath;
}

void HttpRequest::validateResponseBody() {
    if ((statusCode == NO_CONTENT || statusCode == NOT_MODIFIED) && !body.empty()) {
        std::cerr << "Response body must be empty for status code: " << statusCode << "\n";
        body.clear();
    }
}

void HttpRequest::reset() {
    method.clear();
    path.clear();
    version.clear();
    headers.clear();
    body.clear();
    statusCode = OK;
    resolvedPath.clear();
}

bool HttpRequest::parse(const std::string &raw_request, const ServerConfig &config)
{
    //std::cerr << "[DEBUG] Raw request received:\n" << raw_request << "\n";
    reset();
    std::istringstream stream(raw_request);
    std::string line;

    if (!std::getline(stream, line) || line.empty())
	{
		this->setStatusCode(BAD_REQUEST);
		return (false);
	}
    if (!parseRequestLine(line, config))
	{
		return (false);
	} 
    if (!parseHeaders(stream))
    {
        std::cerr << "Failed to parse headers or mandatory headers validation failed\n";
        return (false);
    }
    parseBody(stream);
    validateResponseBody();
    return (true);
}


bool	HttpRequest::extractRequestComponents(const std::string &line, std::string &method,
		std::string &path, std::string &version)
{
	std::istringstream request_line(line);
	
	if (!(request_line >> method >> path >> version))
	{
		this->setStatusCode(BAD_REQUEST);
		std::cerr << "Failed to parse request line components.\n";
		return (false);
	}
	return (true);
}

bool fileExists(const std::string& path) {
    return access(path.c_str(), F_OK) == 0;
}

bool HttpRequest::validateMethod(const std::string &method, const std::string &resource, const ServerConfig &config) {
    if (!isMethodImplemented(method)) {
        setStatusCode(NOT_IMPLEMENTED);
        std::cerr << "Method not implemented: " << method << "\n";
        return false;
    }
    
    std::string best_match = findBestMatchingRoute(resource, config);
    if (best_match.empty()) {
        setStatusCode(NOT_FOUND);
        std::cerr << "No matching route found for resource: " << resource << std::endl;
        return false;
    }

    if (checkForRedirection(best_match, config)) {
        return false; 
    }

    const RouteConfig &route_config = config.routes.find(best_match)->second;
    if (std::find(route_config.methods.begin(), route_config.methods.end(), method) == route_config.methods.end()) {
        setStatusCode(METHOD_NOT_ALLOWED);
        std::cerr << "Method not allowed: " << method << " for resource: " << resource << std::endl;
        return false;
    }
    
    std::string resolved_path = resolveResourcePath(resource, best_match, config);
    setResolvedPath(resolved_path);

    if (!fileExists(resolved_path)) {
        setStatusCode(NOT_FOUND);
        std::cerr << "Resource not found: " << resolved_path << std::endl;
        return false;
    }
    
    bool directoryHandled = handleDirectoryRequest(resolved_path, config.routes.find(best_match)->second);
    return directoryHandled;

    return true;
}

bool HttpRequest::isMethodImplemented(const std::string &method) {
    static const char* validMethodsArray[] = {"GET", "POST", "PUT", "DELETE"};
    static const std::set<std::string> implementedMethods(
        validMethodsArray,
        validMethodsArray + sizeof(validMethodsArray) / sizeof(validMethodsArray[0])
    );
    bool isImplemented = implementedMethods.find(method) != implementedMethods.end();
    return isImplemented;
}

std::string HttpRequest::findBestMatchingRoute(const std::string &resource, const ServerConfig &config) {
    std::string best_match = "";
    for (std::map<std::string, RouteConfig>::const_iterator it = config.routes.begin(); it != config.routes.end(); ++it) {
        if (resource.find(it->first) == 0) {
            std::string after_match = resource.substr(it->first.length());
            if (after_match.empty() || after_match[0] == '/') {
                if (best_match.empty() || it->first.length() > best_match.length()) {
                    best_match = it->first;
                }
            }
        }
    }
    std::string finalMatch = best_match.empty() ? "/" : best_match;
    return finalMatch;
}

std::string HttpRequest::resolveResourcePath(const std::string &resource, const std::string &best_match, const ServerConfig &config) {
    const RouteConfig &route_config = config.routes.find(best_match)->second;
    std::string root_path = route_config.root;
    if (!root_path.empty() && root_path[root_path.size() - 1] == '/') {
        root_path.erase(root_path.size() - 1);
    }
    std::string relative_path = resource.substr(best_match.length());
    if (!relative_path.empty() && relative_path[0] == '/') {
        relative_path = relative_path.substr(1);
    }
    std::string fullPath = root_path + "/" + relative_path;
    return fullPath;
}

bool HttpRequest::checkForRedirection(const std::string &path, const ServerConfig &config) {
    std::map<std::string, RouteConfig>::const_iterator routeIt;
    for (routeIt = config.routes.begin(); routeIt != config.routes.end(); ++routeIt) {
        const RouteConfig &routeConfig = routeIt->second;
        std::map<int, std::string>::const_iterator redirectIt;
        for (redirectIt = routeConfig.redirects.begin(); redirectIt != routeConfig.redirects.end(); ++redirectIt) {
            if (path == routeIt->first) {
                std::string redirectUrl = redirectIt->second;
                int statusCode = redirectIt->first;
                setStatusCode(static_cast<HttpStatus>(statusCode));
                setHeader("Location", redirectUrl);
                if (statusCode == SEE_OTHER) {
                    setMethod("GET");
                }
                return true;
            }
        }
    }
    return false;
}

bool HttpRequest::handleDirectoryRequest(const std::string &resolved_path, const RouteConfig &route_config) {
    struct stat path_stat;
    stat(resolved_path.c_str(), &path_stat);
    if (S_ISDIR(path_stat.st_mode)) {
        std::string index_file = resolved_path + "index.html";
        if (!fileExists(index_file)) {
            if (!route_config.autoindex) {
                setStatusCode(FORBIDDEN);
                std::cerr << "Directory listing is disabled and index.html not found: " << index_file << "\n";
                return false;
            }
        } else {
            setResolvedPath(index_file);
        }
    }
    return true;
}

bool	HttpRequest::validatePath(const std::string &path)
{
	if (path.empty() || path[0] != '/')
	{
		std::cerr << "Invalid HTTP path: " << path << "\n";
		return (false);
	}
	return (true);
}

bool	HttpRequest::validateVersion(const std::string &version)
{
	if (version != "HTTP/1.0" && version != "HTTP/1.1")
	{
		setStatusCode(HTTP_VERSION_NOT_SUPPORTED);
		std::cerr << "Unsupported HTTP version: " << version << "\n";
		return (false);
	}
	return (true);
}

bool HttpRequest::parseRequestLine(const std::string &line, const ServerConfig &config) {
    std::string method_temp, path_temp, version_temp;

    if (!extractRequestComponents(line, method_temp, path_temp, version_temp)) {
		 setStatusCode(BAD_REQUEST);
        std::cerr << "[DEBUG parseRequestLine] Failed to extract request components: " << line << std::endl;
        return false;
    }
    if (path_temp.length() > 2048) { 
        setStatusCode(URI_TOO_LONG);
        std::cerr << "[DEBUG parseRequestLine] URI too long: " << path_temp << std::endl;
        return false;
    }

    if (!validateMethod(method_temp, path_temp, config)) {
		 //setStatusCode(METHOD_NOT_ALLOWED);
        //std::cerr << "[DEBUG parseRequestLine] Invalid method: " << method_temp << std::endl;
        return false;
    }

    if (!validatePath(path_temp)) {
		if(path_temp.size() > 2048)
		{
			setStatusCode(URI_TOO_LONG);
		}
        std::cerr << "[DEBUG parseRequestLine] Invalid path: " << path_temp << std::endl;
        return false;
    }

    if (!validateVersion(version_temp)) {
        std::cerr << "[DEBUG parseRequestLine] Invalid version: " << version_temp << std::endl;
        return false;
    }

    method = method_temp;
    path = path_temp;
    version = version_temp;
    return true;
}

void HttpRequest::trimString(std::string& str) {
    while (!str.empty() && isspace(str[str.size() - 1])) {
        str.erase(str.size() - 1);
    }
    while (!str.empty() && isspace(str[0])) {
        str.erase(0, 1);
    }
}

bool HttpRequest::validateContentLength(const std::string& value) {
    std::stringstream ss(value);
    int contentLength = 0;
    ss >> contentLength;

    if (ss.fail() || contentLength < 0) {
        setStatusCode(LENGTH_REQUIRED);
        std::cerr << "Invalid Content-Length: " << value << "\n";
        return false;
    }

    return true;
}

bool HttpRequest::validateHostHeader(const std::string& value) {
    if (value.empty()) {
        std::cerr << "Empty Host header value\n";
        return false;
    }
    if (value.find(' ') != std::string::npos) {
        std::cerr << "Invalid Host header format: " << value << "\n";
        return false;
    }
    return true;
}

bool HttpRequest::processHeader(const std::string &key, const std::string &value, bool &hostHeaderFound) {
    // Tableau statique pour initialiser les types MIME supportés
    static const char* supportedTypesArray[] = {
        "text/html", 
        "application/json", 
        "application/x-www-form-urlencoded",
        "multipart/form-data"
    };
    static const std::set<std::string> supportedTypes(
        supportedTypesArray, 
        supportedTypesArray + sizeof(supportedTypesArray) / sizeof(supportedTypesArray[0])
    );
    if (key == "Content-Type") {
        if (value.find("multipart/form-data") != std::string::npos) {
            if (value.find("boundary=") == std::string::npos) {
                setStatusCode(BAD_REQUEST);
                std::cerr << "Missing boundary in multipart/form-data Content-Type\n";
                return false;
            }
        }
        else if (supportedTypes.find(value) == supportedTypes.end()) {
            setStatusCode(UNSUPPORTED_MEDIA_TYPE);
            std::cerr << "Unsupported Media Type: " << value << "\n";
            return false;
        }
    } else if (key == "Host") {
        if (!validateHostHeader(value)) {
            return false;
        }
        hostHeaderFound = true;
    } else if (key == "Content-Length") {
        if (!validateContentLength(value)) {
            return false;
        }
    }
    headers[key] = value;
    return true;
}


bool HttpRequest::parseHeaderLine(const std::string& line, bool& hostHeaderFound) {
    size_t delimiter = line.find(":");
    if (delimiter == std::string::npos) {
        std::cerr << "Malformed header (missing colon): " << line << "\n";
        setStatusCode(BAD_REQUEST);
        return false;
    }
    std::string key = line.substr(0, delimiter);
    std::string value = line.substr(delimiter + 1);
    trimString(key);
    trimString(value);
    if (key.empty()) {
        std::cerr << "Malformed header (empty key): " << line << "\n";
        setStatusCode(BAD_REQUEST);
        return false;
    }
    if (value.find("\\r") != std::string::npos || value.find("\\n") != std::string::npos) {
        std::cerr << "Invalid escaped characters in header value: " << value << "\n";
        setStatusCode(BAD_REQUEST);
        return false;
    }
    //std::cerr << "[DEBUG] Parsed header: '" << key << "': '" << value << "'\n";
    if (key == "Host") {
        hostHeaderFound = true;
        //std::cerr << "[DEBUG] Host header found: " << value << "\n";
    }
    return processHeader(key, value, hostHeaderFound);
}

void HttpRequest::debugPrintHeaders() {
    std::cerr << "[DEBUG] Full headers map contents:\n";
    for (std::map<std::string, std::string>::iterator it = headers.begin();
         it != headers.end(); ++it) {
        std::cerr << "  " << it->first << ": " << it->second << "\n";
    }
}

bool HttpRequest::parseHeaders(std::istringstream& stream) {
    std::string line;
    bool hostHeaderFound = false;
    int headerSize = 0;
    const int MAX_HEADER_SIZE = 8192;
    const int MAX_HEADER_FIELD_SIZE = 4096;  

    while (std::getline(stream, line)) {
        if (line.empty() || line == "\r") {
            break;
        }
        if (line.size() > MAX_HEADER_FIELD_SIZE) {
            std::cerr << "Header field too large: " << line << "\n";
            return false;
        }
        headerSize += line.size();
        if (headerSize > MAX_HEADER_SIZE) {
            std::cerr << "Headers too large (exceeding maximum size)\n";
            return false;
        }
        if (!parseHeaderLine(line, hostHeaderFound)) {
            std::cerr << "Failed to parse header line: " << line << "\n";
            return false;
        }
        
    }

    if (headers.find("Content-Length") != headers.end()) {
        int contentLength = std::atoi(headers["Content-Length"].c_str());
        const int MAX_BODY_SIZE = 10485760; // 10 MB
        if (contentLength > MAX_BODY_SIZE) {
            setStatusCode(PAYLOAD_TOO_LARGE);
            return false;
        }
    }

    //debugPrintHeaders();
    if (version == "HTTP/1.1" && !hostHeaderFound) {
        setStatusCode(BAD_REQUEST);
        std::cerr << "Missing Host header for HTTP/1.1 request.\n";
        return false;
    }
    if (headers.find("Content-Length") == headers.end() && method == "POST") {
        std::cerr << "Content-Length header is required for POST requests.\n";
        setStatusCode(LENGTH_REQUIRED); // 411
        return false;
    }
    return true;
}

	void HttpRequest::parseBody(std::istringstream & stream)
	{
		std::string line;
		body.clear();
		int bodySize = 0;
		const int MAX_BODY_SIZE = 10485760;
		while (std::getline(stream, line))
		{
			bodySize += line.size();
			if(bodySize > MAX_BODY_SIZE)
			{
				setStatusCode(PAYLOAD_TOO_LARGE);
				return;
			}
			body += line + "\n";
		}
		if(bodySize == 0 && method == "POST")
		{
			setStatusCode(NO_CONTENT);
		}
	}

std::string HttpRequest::getHeader(std::string headerName)
{
	for(std::map<std::string, std::string>::iterator name = headers.begin(); name != headers.end(); ++name)
	{
		if(name->first == headerName)
		{
			return name->second;
		}
	}
	return "Header not found";
}

bool HttpRequest::ensureUploadDirExists(const std::string &uploadDir) {
    struct stat info;
    if (stat(uploadDir.c_str(), &info) != 0) {
        std::cerr << "[DEBUG] Failed to create upload directory: " << uploadDir << "\n";
        return false;
    } else if (!(info.st_mode & S_IFDIR)) {
        std::cerr << "[DEBUG] Upload directory exists but is not a directory: " << uploadDir << "\n";
        return false;
    }
    return true;
}

bool HttpRequest::parseMultipartFormData(const std::string &body, const std::string &boundary, const std::string &uploadDir) {
    if (!ensureUploadDirExists(uploadDir)) {
        return false;
    }
    
    std::string delimiter = "--" + boundary;
    std::string endDelimiter = delimiter + "--";
    size_t pos = 0;
    size_t endPos = 0;
    
    while ((pos = body.find(delimiter, pos)) != std::string::npos) {
        pos += delimiter.length();
        if (body.substr(pos, 2) == "--") {
            break;
        }
        
        if (!extractHeadersAndContent(body, pos, endPos, delimiter, uploadDir)) {
            return false;
        }
    }
    
    std::cerr << "[DEBUG] Multipart parsing completed successfully\n";
    return true;
}

bool HttpRequest::extractHeadersAndContent(const std::string &body, size_t &pos, size_t &endPos, const std::string &delimiter, const std::string &uploadDir) {
    if ((endPos = body.find("\r\n\r\n", pos)) == std::string::npos) {
        std::cerr << "[DEBUG] Failed to find header/body separator\n";
        return false;
    }
    
    std::string headers = body.substr(pos, endPos - pos);
    pos = endPos + 4;
    
    if ((endPos = body.find(delimiter, pos)) == std::string::npos) {
        std::cerr << "[DEBUG] Failed to find boundary delimiter\n";
        return false;
    }
    
    std::string content = body.substr(pos, endPos - pos);
    pos = endPos;
    
    return processFile(headers, content, uploadDir);
}

bool HttpRequest::processFile(const std::string &headers, const std::string &content, const std::string &uploadDir) {
    std::string filename = extractFilename(headers);
    if (filename.empty()) {
        return true;
    }
    
    //std::cout << "Upload Dir: " << uploadDir << std::endl;
    std::string fullPath = uploadDir + "/" + filename;
    //std::cout << "Full Path: " << fullPath << std::endl;
    
    std::ofstream outFile(fullPath.c_str(), std::ios::binary);
    if (!outFile.is_open()) {
        std::cerr << "[DEBUG] Failed to open file for writing: " << fullPath << "\n";
        return false;
    }
    
    outFile.write(content.c_str(), content.size());
    outFile.close();
    
    std::cerr << "[DEBUG] File saved successfully: " << filename << " at " << fullPath << "\n";
    return true;
}

std::string HttpRequest::extractFilename(const std::string &headers) {
    std::istringstream headerStream(headers);
    std::string headerLine;
    
    while (std::getline(headerStream, headerLine)) {
        if (headerLine.find("Content-Disposition:") != std::string::npos) {
            size_t filenamePos = headerLine.find("filename=\"");
            if (filenamePos != std::string::npos) {
                filenamePos += 10;
                size_t endFilenamePos = headerLine.find("\"", filenamePos);
                if (endFilenamePos != std::string::npos) {
                    return headerLine.substr(filenamePos, endFilenamePos - filenamePos);
                }
            }
        }
    }
    return "";
}

bool HttpRequest::deleteFile(const HttpRequest &request) {
    std::string path = request.getResolvedPath(); 
    struct stat buffer;
    if (stat(path.c_str(), &buffer) == 0) {
        if (remove(path.c_str()) == 0) {
            std::cerr << "[DEBUG] File deleted successfully: " << path << "\n";
            return true;
        } else {
            std::cerr << "[DEBUG] Failed to delete file: " << path << "\n";
            return false;
        }
    } else {
        std::cerr << "[DEBUG] File not found: " << path << "\n";
        return false;
    }
}

int parseRequest(HttpRequest &request, std::string &rawRequest, const ServerConfig &config) {
    if (!request.parse(rawRequest, config)) {
        //std::cerr << "[DEBUG main] Failed to parse HTTP request" << std::endl;
        return (-1);
    }
    return 0;
}

void handleHeaders(HttpRequest &request, std::string &rawRequest, std::string &connectionType) {
    if (request.getVersion() == "HTTP/1.1") {
        std::map<std::string, std::string> headers = request.getHeaders();
        for (std::map<std::string, std::string>::iterator it = headers.begin(); it != headers.end(); ++it) {
            if (it->first == "Connection" && it->second == "keep-alive") {
                connectionType = "keep-alive";
            }
            if (it->first == "Transfer-Encoding" && it->second == "chunked") {
                rawRequest = CgiHandler::unchunkBody(rawRequest);
            }
            if (it->first == "Expect" && it->second == "100-continue") {
                request.sendContinueResponse();
            }
        }
    }
}

int handleRedirections(HttpRequest &request, const ServerConfig &config) {
    std::map<std::string, RouteConfig>::const_iterator routeIt;
    for (routeIt = config.routes.begin(); routeIt != config.routes.end(); ++routeIt) {
        const RouteConfig &routeConfig = routeIt->second;
        std::map<int, std::string>::const_iterator redirectIt;
        for (redirectIt = routeConfig.redirects.begin(); redirectIt != routeConfig.redirects.end(); ++redirectIt) {
            if (request.getPath() == routeIt->first) {
                std::string redirectUrl = redirectIt->second;
                int statusCode = redirectIt->first;
                request.setStatusCode(static_cast<HttpStatus>(statusCode));
                request.setHeader("Location", redirectUrl);
                if (statusCode == SEE_OTHER) {
                    request.setMethod("GET");
                }
                return 0;
            }
        }
    }
    return -1;
}

void handleDeleteMethod(HttpRequest &request) {
    if (request.getMethod() == "DELETE") {
        if (request.deleteFile(request)) {
            request.setStatusCode(NO_CONTENT);
        } else {
            request.setStatusCode(NOT_FOUND);
        }
    }
}

void handlePostOrPutMethod(HttpRequest &request) {
    if ((request.getMethod() == "POST" || request.getMethod() == "PUT") && request.getStatusCode() == OK) {
        std::string body = request.getBody();
        if (body.find("task=start-processing") != std::string::npos) {
            request.setStatusCode(ACCEPTED);
            std::cerr << "[DEBUG] Async job accepted, setting status code to 202" << std::endl;
        }
    }
}

int handleMultipartFormData(HttpRequest &request, std::map<std::string, std::string> headers) {
    if (headers["Content-Type"].find("multipart/form-data") != std::string::npos) {
        size_t boundaryPos = headers["Content-Type"].find("boundary=");
        if (boundaryPos != std::string::npos) {
            std::string boundary = headers["Content-Type"].substr(boundaryPos + 9);
            if (boundary.empty()) {
                std::cerr << "Boundary is empty\n";
                request.setStatusCode(BAD_REQUEST);
                return -1;
            }
            if (!request.parseMultipartFormData(request.getBody(), boundary, "./public/uploads")) {
                std::cerr << "Failed to parse multipart/form-data\n";
                request.setStatusCode(BAD_REQUEST);
                return -1;
            }
            request.setStatusCode(CREATED);
        } else {
            std::cerr << "No boundary found in multipart/form-data Content-Type\n";
            request.setStatusCode(BAD_REQUEST);
            return -1;
        }
    }
    return 0;
}

int parseRequestHelper(HttpRequest &request, std::string &rawRequest, std::string &connectionType, const ServerConfig &config) {
    if (parseRequest(request, rawRequest, config) == -1) {
        return -1;
    }
    handleHeaders(request, rawRequest, connectionType);
    if (handleRedirections(request, config) == 0) {
        return 0;
    }
    handleDeleteMethod(request);
    handlePostOrPutMethod(request);
    std::map<std::string, std::string> headers = request.getHeaders();
    if (handleMultipartFormData(request, headers) == -1) {
        return -1;
    }
    return 0;
}
