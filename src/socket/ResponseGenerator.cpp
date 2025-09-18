#include "HttpRequest.hpp"
#include "ResponseGenerator.hpp"
#include "Utils.hpp"
#include <sstream>
#include <sys/stat.h> 
#include "Logger.hpp"


ResponseGenerator::ResponseGenerator() : _body(""), _response()
{
}

ResponseGenerator::ResponseGenerator(const ResponseGenerator &other)
{
	*this = other;
}

ResponseGenerator::~ResponseGenerator()
{
}

ResponseGenerator &ResponseGenerator::operator=(const ResponseGenerator &other)
{
	if (this != &other)
	{
		_body = other._body;
		_response.str("");
		_response.clear();
		_response << other._response.str();
	}
	return (*this);
}

std::string ResponseGenerator::determineContentType(const std::string &path)
{
	std::map<std::string, std::string> mimeTypes;
	mimeTypes[".html"] = "text/html";
	mimeTypes[".htm"] = "text/html";
	mimeTypes[".css"] = "text/css";
	mimeTypes[".scss"] = "text/css";
	mimeTypes[".js"] = "application/javascript";
	mimeTypes[".json"] = "application/json";
	mimeTypes[".xml"] = "application/xml";
	mimeTypes[".txt"] = "text/plain";
	mimeTypes[".csv"] = "text/csv";
	mimeTypes[".png"] = "image/png";
	mimeTypes[".jpg"] = "image/jpeg";
	mimeTypes[".jpeg"] = "image/jpeg";
	mimeTypes[".gif"] = "image/gif";
	mimeTypes[".ico"] = "image/x-icon";
	mimeTypes[".svg"] = "image/svg+xml";
	mimeTypes[".pdf"] = "application/pdf";
	mimeTypes[".zip"] = "application/zip";
	mimeTypes[".mp4"] = "video/mp4";
	mimeTypes[".mp3"] = "audio/mpeg";
	mimeTypes[".wav"] = "audio/wav";
	mimeTypes[".ogg"] = "audio/ogg";
	mimeTypes[".webp"] = "image/webp";
	for (std::map<std::string,
		std::string>::iterator it = mimeTypes.begin(); it != mimeTypes.end(); ++it)
	{
		if (path.find(it->first) != std::string::npos)
		{
			//std::cout << "What content type im returning: " << it->second << std::endl;
			return (it->second);
		}
	}
	return ("application/octet-stream");
}

std::string ResponseGenerator::readFileFromDisk(const HttpRequest &request){
    std::string path = request.getResolvedPath(); 
    std::ifstream file(path.c_str(), std::ios::binary);
    if (!file) {
        std::cerr << "File not found: " << path << std::endl;
        return "";
    }
    std::ostringstream fileContent;
    fileContent << file.rdbuf();
	//DEBUG
    //std::cout << "File content read: " << fileContent.str().substr(0, 100) << "..." << std::endl;
    return fileContent.str();
}




void ResponseGenerator::getResponse(const HttpRequest &request,
	const std::string &connectionType)
{
	int	statusCode;

	statusCode = request.getStatusCode();
	std::string path = request.getPath();
	//std::cout << "path in getResponse: " << path << std::endl;
	//std::cout << "path from getResolvedPath: " << request.getResolvedPath() << std::endl;
	std::string contentType = determineContentType(path);
	//std::cout << "Handling GET response with status: " << statusCode << std::endl;
	switch (statusCode)
	{
	case 200:
		_response << "HTTP/1.1 200 OK\r\n";
		break ;
	case 202: // Accepted for processing
        _response << "HTTP/1.1 202 Accepted\r\n";
        _body = "Request accepted for processing.";
        break ;
	case 404:
		_response << "HTTP/1.1 404 Not Found\r\n";
		_body = "<h1>404 Not Found</h1><p>The requested file was not found on this server.</p>";
		contentType = "text/html";
		break ;
	case 403:
		_response << "HTTP/1.1 403 Forbidden\r\n";
		_body = "<h1>403 Forbidden</h1><p>Access to this resource is denied.</p>";
		contentType = "text/html";
		break ;
	case 500:
		_response << "HTTP/1.1 500 Internal Server Error\r\n";
		_body = "<h1>500 Internal Server Error</h1><p>Something went wrong on the server.</p>";
		contentType = "text/html";
		break ;
	default:
		_response << "HTTP/1.1 400 Bad Request\r\n";
		_body = "<h1>400 Bad Request</h1><p>The request could not be understood.</p>";
		contentType = "text/html";
		break ;
	}
	if (statusCode == 200)
	{
		_body = readFileFromDisk(request);
		if (_body.empty())
		{
			_response.str("");
			_response << "HTTP/1.1 404 Not Found\r\n";
			_body = "<h1>404 Not Found</h1><p>The requested file was not found on this server.</p>";
			contentType = "text/html";
		}
	}
	_response << "Content-Length: " << _body.length() << "\r\n"
				<< "Connection: " << connectionType << "\r\n"
				<< "Content-Type: " << contentType << "\r\n"
				<< "\r\n"
				<< _body;
}

void ResponseGenerator::deleteResponse(const HttpRequest &request,
	const std::string &connectionType)
{
	int	statusCode;

	statusCode = request.getStatusCode();
	std::string contentType = "text/plain";
	std::cout << "Handling DELETE response with status: " << statusCode << std::endl;
	switch (statusCode)
	{
	case 200: // Successful deletion
		_response << "HTTP/1.1 200 OK\r\n";
		_body = "Resource successfully deleted.";
		break ;
	case 202: // Accepted for processing
        _response << "HTTP/1.1 202 Accepted\r\n";
        _body = "Request accepted for processing.";
        break ;
	case 204: // No content (successful delete but no response body needed)
		_response << "HTTP/1.1 204 No Content\r\n";
		_body = "";
		break ;
	case 400: // Bad request
		_response << "HTTP/1.1 400 Bad Request\r\n";
		_body = "Invalid request parameters.";
		break ;
	case 403: // Forbidden
		_response << "HTTP/1.1 403 Forbidden\r\n";
		_body = "You do not have permission to delete this resource.";
		break ;
	case 404: // Resource not found
		_response << "HTTP/1.1 404 Not Found\r\n";
		_body = "The requested resource was not found.";
		break ;
	case 409: // Conflict
		_response << "HTTP/1.1 409 Conflict\r\n";
		_body = "The resource cannot be deleted due to dependencies.";
		break ;
	default:
		_response << "HTTP/1.1 500 Internal Server Error\r\n";
		_body = "Unexpected error while processing the request.";
		break ;
	}
	_response << "Content-Length: " << _body.length() << "\r\n"
				<< "Connection: " << connectionType << "\r\n"
				<< "Content-Type: " << contentType << "\r\n"
				<< "\r\n"
				<< _body;
	std::cerr << "[DEBUG] Response generated: " << _response.str() << std::endl;
}

void ResponseGenerator::putResponse(const HttpRequest &request,
	const std::string &connectionType)
{
	int	statusCode;

	statusCode = request.getStatusCode();
	std::string contentType = "text/plain";
	std::string path = request.getPath();
	/* Hi Eva, I dont understand why we have this part of the code. 
	The post_data.txt file belows looks a specific test and does not cover all the cases.
	if (path.find("/cgi-bin/") != 0)
	{
		std::string body = request.getBody();
		std::ofstream outputFile("post_data.txt", std::ios::app);
		if (outputFile.is_open())
		{
			outputFile << "Received POST request on path: " << path << "\n";
			outputFile << "Body:\n" << body << "\n";
			outputFile.close();
		}
	}
	std::string contentType = "text/plain";
	std::cout << "Handling PUT response with status: " << statusCode << std::endl;
	*/
	switch (statusCode)
	{
	case 200: // Resource updated
		_response << "HTTP/1.1 200 OK\r\n";
		_body = "Resource successfully updated.";
		break ;
	case 201: // Resource created
		_response << "HTTP/1.1 201 Created\r\n";
		_body = "New resource created.";
		_response << "Location: /new-resource-id\r\n";
		// Example: provide new resource path
		break ;
	case 202: // Accepted for processing
    	_response << "HTTP/1.1 202 Accepted\r\n";
        _body = "Request accepted for processing.";
        break ;
	case 400: // Bad request
		_response << "HTTP/1.1 400 Bad Request\r\n";
		_body = "Invalid input or missing required fields.";
		break ;
	case 403: // Forbidden
		_response << "HTTP/1.1 403 Forbidden\r\n";
		_body = "You do not have permission to modify this resource.";
		break ;
	case 404: // Resource not found
		_response << "HTTP/1.1 404 Not Found\r\n";
		_body = "The requested resource does not exist.";
		break ;
	case 409: // Conflict
		_response << "HTTP/1.1 409 Conflict\r\n";
		_body = "The resource could not be updated due to a conflict.";
		break ;
	default:
		_response << "HTTP/1.1 500 Internal Server Error\r\n";
		_body = "Unexpected error while processing the request.";
		break ;
	}
	_response << "Content-Length: " << _body.length() << "\r\n"
				<< "Connection: " << connectionType << "\r\n"
				<< "Content-Type: " << contentType << "\r\n"
				<< "\r\n"
				<< _body;
}

void ResponseGenerator::postResponse(const HttpRequest &request,
	const std::string &connectionType)
{
	int	statusCode;

	statusCode = request.getStatusCode();
	//std::cerr << "[DEBUG] Status code in postResponse: " << statusCode << std::endl;

	// Si un statut d'erreur est déjà défini, génère une réponse d'erreur immédiatement
    if (statusCode >= 400) {
        std::cerr << "[DEBUG] Error detected in request, generating error response: " << statusCode << "\n";
        generateErrorResponse(statusCode, connectionType);
        return;
    }

	std::string contentType = "text/plain";
	std::cout << "Handling POST response with status: " << statusCode << std::endl;
	std::string path = request.getPath();
	std::string body = request.getBody();
	/* Hi Eva, I dont understand why we have this part of the code. 
	The post_data.txt file belows looks a specific test and does not cover all the cases.
	if (path.find("/cgi-bin/") != 0)
	{
		if (body.empty())
		{
			statusCode = 411;
		}
		else
		{
			std::ofstream outputFile("post_data.txt", std::ios::app);
			if (!outputFile.is_open())
			{
				statusCode = 500; // Internal Server Error
			}
			else
			{
				outputFile << "Received POST request on path: " << path << "\n";
				outputFile << "Body:\n" << body << "\n";
				outputFile.close();
				statusCode = 201;
				_response << "HTTP/1.1 201 Created\r\n";
				_body = "POST request successfully processed.";
			}
		}
	}
	*/
	switch (statusCode)
	{
	case 200: // Success (but rare for POST)
		_response << "HTTP/1.1 200 OK\r\n";
		_body = "Request processed successfully.";
		break ;
	case 201: // Resource created
		_response << "HTTP/1.1 201 Created\r\n";
		_response << "Location: /new-resource-id\r\n"; // Example header
		_body = "Resource successfully created.";
		break ;
    case 202: // Accepted for processing
        _response << "HTTP/1.1 202 Accepted\r\n";
        _body = "Request accepted for processing.";
        break ;
	case 204: // No Content (valid if the request had no body)
		_response << "HTTP/1.1 204 No Content\r\n";
		_response << "Content-Length: 0\r\n";
		_response << "Connection: " << connectionType << "\r\n";
		_response << "\r\n"; // No body
		return ;
	case 400: // Bad Request (e.g., invalid input, malformed JSON)
		_response << "HTTP/1.1 400 Bad Request\r\n";
		_body = "Invalid input or missing required fields.";
		break ;
	case 403: // Forbidden (e.g., posting to a protected resource)
		_response << "HTTP/1.1 403 Forbidden\r\n";
		_body = "You do not have permission to modify this resource.";
		break ;
	case 404: // Not Found (e.g., trying to post to a non-existing endpoint)
		_response << "HTTP/1.1 404 Not Found\r\n";
		_body = "Requested resource not found.";
		break ;
	case 405: // Method Not Allowed (POST is not allowed for this URL)
		_response << "HTTP/1.1 405 Method Not Allowed\r\n";
		_body = "POST method is not supported for this endpoint.";
		break ;
	case 409: // Conflict (e.g., resource already exists)
		_response << "HTTP/1.1 409 Conflict\r\n";
		_body = "Resource already exists.";
		break ;
	case 411: // Length Required (Content-Length is missing)
		_response << "HTTP/1.1 411 Length Required\r\n";
		_body = "Content-Length header is required.";
		break ;
	case 413: // Payload Too Large (request body exceeds limit)
		_response << "HTTP/1.1 413 Payload Too Large\r\n";
		_body = "Request payload exceeds the allowed limit.";
		break ;
	case 415: // Unsupported Media Type (wrong Content-Type)
		_response << "HTTP/1.1 415 Unsupported Media Type\r\n";
		_body = "Content-Type is not supported.";
		break ;
	case 429: // Too Many Requests (rate limiting)
		_response << "HTTP/1.1 429 Too Many Requests\r\n";
		_body = "You have exceeded the allowed request rate.";
		break ;
	default: // Internal Server Error (fallback)
		_response << "HTTP/1.1 500 Internal Server Error\r\n";
		_body = "Unexpected error while processing the request.";
		break ;
	}
	_response << "Content-Length: " << _body.length() << "\r\n"
				<< "Connection: " << connectionType << "\r\n"
				<< "Content-Type: " << contentType << "\r\n"
				<< "\r\n"
				<< _body;
}

std::string ResponseGenerator::getStatusMessage(int statusCode) const {
    switch (statusCode) {
        case 200: return "OK";
        case 201: return "Created";
        case 202: return "Accepted";
        case 204: return "No Content";
        case 301: return "Moved Permanently";
        case 302: return "Found";
        case 303: return "See Other";
        case 307: return "Temporary Redirect";
        case 308: return "Permanent Redirect";
        case 400: return "Bad Request";
        case 401: return "Unauthorized";
        case 403: return "Forbidden";
        case 404: return "Not Found";
        case 405: return "Method Not Allowed";
        case 408: return "Request Timeout";
        case 411: return "Length Required";
        case 413: return "Payload Too Large";
        case 414: return "URI Too Long";
        case 415: return "Unsupported Media Type";
        case 429: return "Too Many Requests";
        case 500: return "Internal Server Error";
        case 501: return "Not Implemented";
        case 502: return "Bad Gateway";
        case 503: return "Service Unavailable";
        case 504: return "Gateway Timeout";
        case 505: return "HTTP Version Not Supported";
        default: return "Unknown Status";
    }
}

void ResponseGenerator::generateRedirectionResponse(HttpRequest& request, int statusCode, const std::string &location) {
    std::string contentType = "text/plain";

    if (statusCode == 303) {
        request.setMethod("GET"); 
    }

    switch (statusCode) {
        case 301:
            _response << "HTTP/1.1 301 Moved Permanently\r\n";
            _body = "The requested resource has been permanently moved to: " + location;
            break;
        case 302:
            _response << "HTTP/1.1 302 Found\r\n";
            _body = "The requested resource is temporarily located at: " + location;
            break;
        case 303:
            _response << "HTTP/1.1 303 See Other\r\n";
            _body = "The requested resource can be found at: " + location;
            break;
        case 307:
            _response << "HTTP/1.1 307 Temporary Redirect\r\n";
            _body = "The requested resource is temporarily located at: " + location;
            break;
        case 308:
            _response << "HTTP/1.1 308 Permanent Redirect\r\n";
            _body = "The requested resource has been permanently moved to: " + location;
            break;
        default:
            _response << "HTTP/1.1 " << statusCode << " " << getStatusMessage(statusCode) << "\r\n";
            _body = "The requested resource is located at: " + location;
            break;
    }

    _response << "Location: " << location << "\r\n"
              << "Content-Length: " << _body.length() << "\r\n"
              << "Connection: close\r\n"
              << "Content-Type: " << contentType << "\r\n"
              << "\r\n"
              << _body;
}


std::string ResponseGenerator::getResponse() const {
    return _response.str();
}




std::string ResponseGenerator::readErrorPage(int statusCode) {
    std::stringstream ss;
    ss << "./public/errors/" << statusCode << ".html";
    std::string errorPagePath = ss.str();
    
    std::ifstream file(errorPagePath.c_str(), std::ios::binary);
    if (!file) {
        std::cerr << "Error page not found: " << errorPagePath << std::endl;
        ss.str(""); 
        ss << "<h1>" << statusCode << " " << getStatusMessage(statusCode) << "</h1><p>Error page not found.</p>";
        return ss.str();
    }
    std::ostringstream fileContent;
    fileContent << file.rdbuf();
    return fileContent.str();
}

void ResponseGenerator::generateErrorResponse(int statusCode, const std::string &connectionType) {
    std::string contentType = "text/html";
    _body = readErrorPage(statusCode);

    _response << "HTTP/1.1 " << statusCode << " " << getStatusMessage(statusCode) << "\r\n"
              << "Content-Length: " << _body.length() << "\r\n"
              << "Connection: " << connectionType << "\r\n"
              << "Content-Type: " << contentType << "\r\n"
              << "\r\n"
              << _body;
}


/*
void ResponseGenerator::generateErrorResponse(int statusCode, const std::string &connectionType) {
    std::string contentType = "text/plain";

    switch (statusCode) {
        case 400:
            _response << "HTTP/1.1 400 Bad Request\r\n";
            _body = "Invalid input or malformed request.";
            break;
        case 401:
            _response << "HTTP/1.1 401 Unauthorized\r\n";
            _body = "Authentication is required and has failed or has not yet been provided.";
            break;
        case 403:
            _response << "HTTP/1.1 403 Forbidden\r\n";
            _body = "You do not have permission to access this resource.";
            break;
        case 404:
            _response << "HTTP/1.1 404 Not Found\r\n";
            _body = "The requested resource was not found.";
            break;
        case 405:
            _response << "HTTP/1.1 405 Method Not Allowed\r\n";
            _body = "The request method is not supported for the requested resource.";
            break;
        case 408:
            _response << "HTTP/1.1 408 Request Timeout\r\n";
            _body = "The server timed out waiting for the request.";
            break;
        case 411:
            _response << "HTTP/1.1 411 Length Required\r\n";
            _body = "Content-Length header is required.";
            break;
        case 413:
            _response << "HTTP/1.1 413 Payload Too Large\r\n";
            _body = "The request is larger than the server is willing or able to process.";
            break;
        case 414:
            _response << "HTTP/1.1 414 URI Too Long\r\n";
            _body = "The URI provided was too long for the server to process.";
            break;
        case 415:
            _response << "HTTP/1.1 415 Unsupported Media Type\r\n";
            _body = "The request entity has a media type which the server or resource does not support.";
            break;
        case 500:
            _response << "HTTP/1.1 500 Internal Server Error\r\n";
            _body = "Unexpected error occurred.";
            break;
        case 501:
            _response << "HTTP/1.1 501 Not Implemented\r\n";
            _body = "The server does not support the functionality required to fulfill the request.";
            break;
        case 502:
            _response << "HTTP/1.1 502 Bad Gateway\r\n";
            _body = "The server received an invalid response from the upstream server.";
            break;
        case 503:
            _response << "HTTP/1.1 503 Service Unavailable\r\n";
            _body = "The server is currently unable to handle the request due to temporary overloading or maintenance.";
            break;
        case 504:
            _response << "HTTP/1.1 504 Gateway Timeout\r\n";
            _body = "The server did not receive a timely response from the upstream server.";
            break;
        case 505:
            _response << "HTTP/1.1 505 HTTP Version Not Supported\r\n";
            _body = "The server does not support the HTTP protocol version used in the request.";
            break;
        default:
            _response << "HTTP/1.1 500 Internal Server Error\r\n";
            _body = "Unexpected error occurred.";
            break;
    }

    _response << "Content-Length: " << _body.length() << "\r\n"
              << "Connection: " << connectionType << "\r\n"
              << "Content-Type: " << contentType << "\r\n"
              << "\r\n"
              << _body;
}
*/


std::string ResponseGenerator::generateResponse(HttpRequest& request, const std::string& connectionType) {
    _response.clear();

	    if (!Logger::isLogFileAccessible()) {
        generateErrorResponse(500, connectionType);
        std::cerr << "Returning 500 Internal Server Error" << std::endl;
        return _response.str();
    }

    int statusCode = request.getStatusCode();
    //std::cout << "CHECKING THE STATUS CODE: " << statusCode << std::endl;
	
    std::string path = request.getPath();
    std::string contentType = determineContentType(path);

    if (statusCode >= 300 && statusCode < 400) {
        std::map<std::string, std::string> headers = request.getHeaders();

        if (headers.find("Location") != headers.end()) {
            generateRedirectionResponse(request, statusCode, headers["Location"]);
        } else {
            std::cerr << "[ERROR] Redirect status code but no Location header found!" << std::endl;
            generateErrorResponse(500, connectionType);
        }

        return _response.str();
}
 else if (statusCode >= 400) {
		generateErrorResponse(statusCode, connectionType);
		return _response.str();
	}

    //std::cout << "Checking the method: " << request.getMethod() << std::endl;
    if (request.getMethod() == "GET") {
        getResponse(request, connectionType);
    } else if (request.getMethod() == "POST") {
        postResponse(request, connectionType);
    } else if (request.getMethod() == "DELETE") {
        deleteResponse(request, connectionType);
    } else if (request.getMethod() == "PUT") {
        putResponse(request, connectionType);
    } else {
        generateErrorResponse(405, connectionType);
    }

    //std::cout << "RESPONSE: " << _response.str() << std::endl;
    return _response.str();
}


std::string ResponseGenerator::getBody() const
{
	return (_body);
}
