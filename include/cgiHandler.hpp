#ifndef CGI_HANDLER_HPP
#define CGI_HANDLER_HPP

#include <string>
#include <map>
#include <iostream>
#include <cstdlib>
#include <unistd.h>
#include <sys/wait.h>
#include <sstream>
#include <vector>

class CgiHandler
{
private:
    std::string _request;
    std::string _method;
    std::string _contentType;

public:

    CgiHandler();
    CgiHandler(const std::string &request, const std::string &method, const std::string &contentType);
    CgiHandler(const CgiHandler &other);
    CgiHandler &operator=(const CgiHandler &other);
    
    ~CgiHandler();

    static std::string unchunkBody(const std::string &body);
    std::string executeCgi(const std::string &scriptPath);

};

#endif
