#include "ConfigParser.hpp"
#include <fstream>
#include <sstream>
#include <iostream>
#include <algorithm>
#include <stdexcept>
#include <sys/stat.h>
#include "Logger.hpp"

ConfigParser::ConfigParser() {}
ConfigParser::~ConfigParser() {}

std::string getConfigPath(int argc, char *argv[]) {
    std::string config_path = "src/config/prod_server.conf";
    if (argc == 2) {
        config_path = argv[1];
        std::cout << "Using configuration file: " << config_path << std::endl;
    } else if (argc > 2) {
        std::cerr << "Error: number of arguments" << std::endl;
        throw std::runtime_error("Error: number of arguments");
    }
    return config_path;
}

bool isAccessibleFile(const std::string &path) {
    struct stat fileStat;
    if (stat(path.c_str(), &fileStat) != 0) {
        Logger::logError(path + " could not be accessed");
        return false;
    }
    if (!(fileStat.st_mode & S_IRUSR)) {
        Logger::logError(path + " is not readable");
        return false;
    }
    if (S_ISDIR(fileStat.st_mode)) {
        Logger::logError(path + " is a directory, not a file");
        return false;
    }
    std::ifstream file(path.c_str());
    if (!file.is_open() || file.fail()) {
        Logger::logError(path + " could not be opened");
        return false;
    }
    file.close();
    return true;
}

bool ConfigParser::parseConfig(const std::string &file_path, ServerConfig &config) {
    if (!isAccessibleFile(file_path))
    {
        throw std::runtime_error("Configuration file not accessible");
    }
    std::ifstream file(file_path.c_str());
    processConfigFile(file, config);
    file.close();
    return true;
}

void ConfigParser::processConfigFile(std::ifstream &file, ServerConfig &config) {
    std::string line, current_section, current_route_path;
    RouteConfig temp_route;
    while (std::getline(file, line)) {
        processLine(line, current_section, current_route_path, temp_route, config);
    }
}

void ConfigParser::processLine(std::string &line, std::string &current_section, 
                               std::string &current_route_path, RouteConfig &temp_route, ServerConfig &config) {
    std::replace(line.begin(), line.end(), '\r', ' ');  // Remove carriage returns
    trim(line);  // Remove leading/trailing whitespace

    if (shouldIgnoreLine(line)) return;  // Ignore comments and empty lines

    if (line == "server {") {
        enterServerSection(current_section, temp_route);
    } else if (line == "}") {
        handleClosingBracket(current_section, current_route_path, temp_route, config);
    } else if (line.substr(0, 9) == "location ") {
        handleLocationLine(line, current_section, temp_route, current_route_path);
    } else if (line.substr(0, 6) == "return") {
        //std::cout << "[DEBUG] Found return line: " << line << std::endl;
        if (current_section == "route") {
            parseReturn(line.substr(7), temp_route);  // Tie return to current route
        } else {
            std::cerr << "Error: 'return' directive must be inside a location block." << std::endl;
        }
    } else if (current_section == "server") {
        parseServerLine(line, config);
    } else if (current_section == "route") {
        parseRouteLine(line, temp_route);
    }
}

void ConfigParser::parseReturn(const std::string &line, RouteConfig &route) {
    std::istringstream iss(line);
    int statusCode;
    std::string url;
    iss >> statusCode >> url;
    if (statusCode == 301 || statusCode == 302 || statusCode == 303 || statusCode == 307 || statusCode == 308) {
        route.redirects[statusCode] = url;
        //std::cout << "[DEBUG] Parsed return directive: " << statusCode << " " << url << std::endl;
    } else {
        std::cerr << "Error: Unsupported return status code: " << statusCode << std::endl;
    }
}


void ConfigParser::enterServerSection(std::string &current_section, RouteConfig &temp_route) {
    current_section = "server";
    temp_route = RouteConfig();
}

bool ConfigParser::shouldIgnoreLine(const std::string &line) {
    std::string trimmed = line;
    trim(trimmed);
    return trimmed.empty() || trimmed[0] == '#';
}

void ConfigParser::handleClosingBracket(std::string &current_section, std::string &current_route_path, 
                                        RouteConfig &temp_route, ServerConfig &config) {
    if (current_section == "route" && !current_route_path.empty()) {
        config.routes[current_route_path] = temp_route;
        temp_route = RouteConfig();
    }
    current_section = "";
}

void ConfigParser::handleLocationLine(const std::string &line, std::string &current_section, 
                                      RouteConfig &temp_route, std::string &current_route_path) {
    if (!current_route_path.empty()) {
        current_section = "route";
        temp_route = RouteConfig();  // Reset route config for the new location
    }
    current_route_path = line.substr(9, line.size() - 10);  // Extract location path
    trim(current_route_path);
    current_section = "route";
}


void ConfigParser::parseServerLine(const std::string &line, ServerConfig &config) {
    if (line.find("listen") == 0) {
        std::istringstream iss(line.substr(7));
        int port;
        while (iss >> port) {
            config.listen_ports.push_back(port);
        }
    } else if (line.find("server_name ") == 0) {
        config.server_name = line.substr(12);
    } else if (line.find("error_page 404 ") == 0) {
        config.error_page_404 = line.substr(15);
    } else if (line.find("error_page 500 ") == 0) {
        config.error_page_500 = line.substr(15);
    } else if (line.find("client_max_body_size ") == 0) {
        config.client_max_body_size = atoi(line.substr(21).c_str());
    }
}

void ConfigParser::parseRedirect(const std::string &line, RouteConfig &route) {
    std::istringstream iss(line);
    int statusCode;
    std::string old_path, new_path;
    iss >> statusCode >> old_path >> new_path;
    if (statusCode == 301 || statusCode == 302) {
        route.redirects[statusCode] = new_path;  // Save redirect
        //std::cout << "[DEBUG] Parsed redirect: " << statusCode << " " << old_path << " -> " << new_path << std::endl;
    } else {
        std::cerr << "Error: Unsupported redirect status code: " << statusCode << std::endl;
    }
}




void ConfigParser::parseRouteLine(const std::string &line, RouteConfig &route) {
    std::string trimmed_line = line;
    trim(trimmed_line);
    if (trimmed_line.find("root ") == 0) {
        route.root = trimmed_line.substr(5);
    } else if (trimmed_line.find("methods ") == 0) {
        parseMethods(trimmed_line.substr(8), route);
    } else if (trimmed_line.find("index ") == 0) {
        route.index = trimmed_line.substr(6);
    } else if (trimmed_line.find("upload_store ") == 0) {
        route.upload_store = trimmed_line.substr(13);
    } else if (trimmed_line.find("cgi_extensions ") == 0) {
        parseCgiExtensions(trimmed_line.substr(14), route);
    } else if (trimmed_line.find("redirect ") == 0) { 
        parseRedirect(trimmed_line.substr(9), route);
    } else if (trimmed_line.find("autoindex ") == 0) {
        std::string value = trimmed_line.substr(10);
        route.autoindex = (value == "on");
    }
}

void ConfigParser::parseMethods(const std::string &methods_str, RouteConfig &route) {
    std::istringstream methods_stream(methods_str);
    std::string method;
    while (methods_stream >> method) {
        route.methods.push_back(method);
    }
}

void ConfigParser::parseCgiExtensions(const std::string &extensions_str, RouteConfig &route) {
    std::istringstream cgi_stream(extensions_str);
    std::string cgi_extension;
    while (cgi_stream >> cgi_extension) {
        route.cgi_extensions.push_back(cgi_extension);
    }
}

void ConfigParser::trim(std::string &s) {
    size_t start = s.find_first_not_of(" \t\r");
    size_t end = s.find_last_not_of(" \t\r");
    if (start == std::string::npos || end == std::string::npos) {
        s = "";
    } else {
        s = s.substr(start, end - start + 1);
    }
    size_t comment_pos = s.find('#');
    if (comment_pos != std::string::npos) {
        s = s.substr(0, comment_pos);
    }
    start = s.find_first_not_of(" \t\r");
    end = s.find_last_not_of(" \t\r");
    if (start == std::string::npos || end == std::string::npos) {
        s = "";
    } else {
        s = s.substr(start, end - start + 1);
    }
    if (!s.empty() && s[s.size() - 1] == ';') {
        s.erase(s.size() - 1);
    }
}

void ConfigParser::loadConfiguration(const std::string &config_file_path, ServerConfig &server_config) {
    ConfigParser parser;

    if (parser.parseConfig(config_file_path, server_config)) {
        std::cout << "Configuration parsed successfully!" << std::endl;
        /*
        std::cout << "Listen Ports: ";
        for (size_t i = 0; i < server_config.listen_ports.size(); ++i) {
            std::cout << server_config.listen_ports[i];
            if (i < server_config.listen_ports.size() - 1) {
                std::cout << ", ";
            }
        }
        std::cout << std::endl;
        std::cout << "Server Name: " << server_config.server_name << std::endl;
        std::cout << "404 Error Page: " << server_config.error_page_404 << std::endl;
        std::cout << "500 Error Page: " << server_config.error_page_500 << std::endl;
        std::cout << "Client Max Body Size: " << server_config.client_max_body_size << std::endl;
        for (std::map<std::string, RouteConfig>::iterator it = server_config.routes.begin();
             it != server_config.routes.end(); ++it) {
            std::cout << "Route: " << it->first << std::endl;
            std::cout << "  Root: " << it->second.root << std::endl;
            std::cout << "  Methods: ";
            if (!it->second.methods.empty()) {
                for (size_t i = 0; i < it->second.methods.size(); ++i) {
                    std::cout << it->second.methods[i] << " ";
                }
            } else {
                std::cout << "Not Set";
            }
            std::cout << "\n  Index: " << it->second.index << std::endl;
            std::cout << "  Upload Store: " << it->second.upload_store << std::endl;
            std::cout << "  CGI Extensions: ";
            if (!it->second.cgi_extensions.empty()) {
                for (size_t i = 0; i < it->second.cgi_extensions.size(); ++i) {
                    std::cout << it->second.cgi_extensions[i] << " ";
                }
            } else {
                std::cout << "Not Set";
            }
            std::cout << std::endl;

            if (!it->second.redirects.empty()) {
                std::cout << "  Redirects:" << std::endl;
                for (std::map<int, std::string>::iterator redirect_it = it->second.redirects.begin();
                    redirect_it != it->second.redirects.end(); ++redirect_it) {
                    std::cout << "    " << redirect_it->first << " -> " << redirect_it->second << std::endl;
                }
            } else {
                std::cout << "  Redirects: Not Set" << std::endl;
            }
            std::cout << "  Autoindex: " << (it->second.autoindex ? "on" : "off") << std::endl;
        }
        */
    } else {
        std::cerr << "Failed to parse configuration file." << std::endl;
    }
}