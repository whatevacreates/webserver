#ifndef CONFIGPARSER_HPP
#define CONFIGPARSER_HPP

#include <string>
#include <map>
#include <vector>

struct RouteConfig {
    std::string                         root;
    std::vector<std::string>            methods;
    std::string                         index;
    std::string                         upload_store;
    std::vector<std::string>            cgi_extensions;
    std::map<int, std::string>          redirects;
    bool                                autoindex;

    RouteConfig() : root(""), index(""), upload_store(""), autoindex(false) {}
};

struct ServerConfig {
    std::vector<int>                    listen_ports;
    std::string                         server_name;
    std::map<std::string, RouteConfig>  routes;
    std::string                         error_page_404;
    std::string                         error_page_500;
    int                                 client_max_body_size;

    ServerConfig() : listen_ports(0), server_name(""), error_page_404(""), error_page_500(""), client_max_body_size(0) {}
};

class ConfigParser {
public:
    ConfigParser();
    ~ConfigParser();

    bool parseConfig(const std::string &file_path, ServerConfig &config);
    static void loadConfiguration(const std::string &config_file_path, ServerConfig &config);

private:
    void parseServerLine(const std::string &line, ServerConfig &config);
    void parseRouteLine(const std::string &line, RouteConfig &route);
    void parseMethods(const std::string &methods_str, RouteConfig &route);
    void parseCgiExtensions(const std::string &extensions_str, RouteConfig &route);

    void trim(std::string &s);
    void processConfigFile(std::ifstream &file, ServerConfig &config);
    void processLine(std::string &line, std::string &current_section,
                     std::string &current_route_path, RouteConfig &temp_route, ServerConfig &config);

    bool shouldIgnoreLine(const std::string &line);
    void handleClosingBracket(std::string &current_section, std::string &current_route_path,
                              RouteConfig &temp_route, ServerConfig &config);
    void handleLocationLine(const std::string &line, std::string &current_section,
                            RouteConfig &temp_route, std::string &current_route_path);

    void enterServerSection(std::string &current_section, RouteConfig &temp_route);
    void parseRedirect(const std::string &redirect_str, RouteConfig &route);
    void parseReturn(const std::string &return_str, RouteConfig &route);
};

bool isAccessibleFile(const std::string &file_path);
std::string getConfigPath(int argc, char *argv[]);

#endif
