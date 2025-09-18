#ifndef HTTPREQUEST_HPP
#define HTTPREQUEST_HPP

#include <string>
#include <map>
#include <sstream>
#include <set>
#include <iostream>
#include <vector>
#include <fstream>
#include "ConfigParser.hpp"

enum HttpStatus {
    // 1xx Informational
    CONTINUE = 100,
    SWITCHING_PROTOCOLS = 101,

    // 2xx Success
    OK = 200,
    CREATED = 201,
    ACCEPTED = 202,
    NO_CONTENT = 204,

    // 3xx Redirection
    MOVED_PERMANENTLY = 301,
    FOUND = 302,
    SEE_OTHER = 303,
    NOT_MODIFIED = 304,
    TEMPORARY_REDIRECT = 307,

    // 4xx Client Error
    BAD_REQUEST = 400,
    UNAUTHORIZED = 401,
    FORBIDDEN = 403,
    NOT_FOUND = 404,
    METHOD_NOT_ALLOWED = 405,
    REQUEST_TIMEOUT = 408,
    LENGTH_REQUIRED = 411,
    PAYLOAD_TOO_LARGE = 413,
    URI_TOO_LONG = 414,
    UNSUPPORTED_MEDIA_TYPE = 415,

    // 5xx Server Error
    INTERNAL_SERVER_ERROR = 500,
    NOT_IMPLEMENTED = 501,
    BAD_GATEWAY = 502,
    SERVICE_UNAVAILABLE = 503,
    GATEWAY_TIMEOUT = 504,
    HTTP_VERSION_NOT_SUPPORTED = 505,
};


class HttpRequest {
public:
    HttpRequest();
    HttpRequest(int socket);
    HttpRequest(const HttpRequest &other);
    HttpRequest &operator=(const HttpRequest &other);
    ~HttpRequest();

    bool parse(const std::string &raw_request, const ServerConfig &config);

    std::string getMethod() const;
    std::string getPath() const;
    std::string getVersion() const;
    std::string getHeader(std::string headerName);
    std::map<std::string, std::string> getHeaders() const;
    std::string getBody() const;
    void setHeader(const std::string &key, const std::string &value);
    void setStatusCode(HttpStatus code); 
    void setMethod(const std::string &method);
    void setPath(const std::string& path);
    void setHeaders(const std::map<std::string, std::string>& headers);
    HttpStatus getStatusCode() const;
    bool parseRequestLine(const std::string &line, const ServerConfig &config); 
    bool validateMethod(const std::string &method, const std::string &resource, const ServerConfig &config);    
    bool validatePath(const std::string &path);
    bool validateVersion(const std::string &version);
    void trimString(std::string& str);
    bool validateContentLength(const std::string& value);
    bool validateHostHeader(const std::string& value);
    bool processHeader(const std::string &key, const std::string &value, bool &hostHeaderFound);
    bool parseHeaderLine(const std::string &line, bool &hostHeaderFound);
    void debugPrintHeaders();
    bool parseHeaders(std::istringstream& stream);
    void parseBody(std::istringstream &stream);
    bool	extractRequestComponents(const std::string &line, std::string &method,
		std::string &path, std::string &version);
    void validateResponseBody();
    void sendContinueResponse();
    const std::string& getResolvedPath() const;
    void setResolvedPath(const std::string &resolvedPath);
    void reset();
    std::string findBestMatchingRoute(const std::string &resource, const ServerConfig &config);
    std::string resolveResourcePath(const std::string &resource, const std::string &best_match, const ServerConfig &config);
    bool handleDirectoryRequest(const std::string &resolved_path, const RouteConfig &route);
    bool isMethodImplemented(const std::string &method);
    bool parseMultipartFormData(const std::string &body, const std::string &boundary, const std::string &uploadDir);    
    bool extractHeadersAndContent(const std::string &body, size_t &pos, size_t &endPos, const std::string &delimiter, const std::string &uploadDir);
    bool processFile(const std::string &headers, const std::string &content, const std::string &uploadDir);
    std::string extractFilename(const std::string &headers);
    bool ensureUploadDirExists(const std::string &upload_dir);
    bool deleteFile(const HttpRequest &request);
    bool checkForRedirection(const std::string &resolved_path, const ServerConfig &config);

private:
    int clientSocket;
    std::string method;
    std::string path;
    std::string version;
    std::map<std::string, std::string> headers;
    std::string body;
    HttpStatus statusCode;
    std::string resolvedPath;
};

#endif