#include "ConfigParser.hpp"
#include "FileDescriptorUtil.hpp"
#include "HttpRequest.hpp"
#include "ResponseGenerator.hpp"
#include "Socket.hpp"
#include "Utils.hpp"
#include "cgiHandler.hpp"
#include "Logger.hpp"
#include <unistd.h> 

bool logAccessible = true;

int	main(int argc, char *argv[])
{
	ServerConfig config;
	std::string config_path = getConfigPath(argc, argv);
    try {
        ConfigParser::loadConfiguration(config_path, config);
    } catch (const std::runtime_error &e) {
        std::cerr << "Failed to load configuration: " << e.what() << std::endl;
        return 1;
    }

    std::string logFilePath = "src/log/logfile.log";
    try {
        Logger::init(logFilePath);
    } catch (const std::runtime_error &e) {
        std::cerr << "Internal Server Error: " << e.what() << std::endl;
        logAccessible = false;
    }

	try
	{
        runMainLoop(config);
    }
	catch (const std::exception &e)
	{
		std::cerr << "Exiting... Caught Exception: " << e.what() << std::endl;
	}

	return (0);
}
