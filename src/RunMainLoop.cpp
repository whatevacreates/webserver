
#include "ConfigParser.hpp"
#include "FileDescriptorUtil.hpp"
#include "HttpRequest.hpp"
#include "ResponseGenerator.hpp"
#include "Socket.hpp"
#include "Utils.hpp"
#include "cgiHandler.hpp"
#include "Logger.hpp"
#include <unistd.h> 

bool	isThereOneValidSocket(std::vector<Socket *> servers)
{
	for (size_t i = 0; i < servers.size(); ++i)
	{
		if (servers[i]->getSocketFd() != -1)
		{
			return (true);
		}
	}
	return (false);
}

/*
void	runMainLoop(ServerConfig &config)
{
	std::vector<Socket *> servers;
	std::vector<int> ports;
	std::vector<struct pollfd> pollFds;

	std::map<int, time_t> clientTimestamps;
	std::map<int, std::string> responseBank;
	startVariables(config, ports, pollFds, servers);
	if (!isThereOneValidSocket(servers))
		throw std::runtime_error("No valid listening sockets available. Exiting.");

	while (true)
	{
		for (std::vector<struct pollfd>::iterator it = pollFds.begin(); it != pollFds.end();)
		{
			if (it->fd == -1)
				it = pollFds.erase(it);
			else
				++it;
		}
		timingConnections(clientTimestamps, pollFds);
		int ready;
		ready = poll(&pollFds[0], pollFds.size(), 1000);
		if (ready < 0)
			throw std::runtime_error("poll() failure");
		if (pollFds.empty())
		{
			std::cout << "No active connections, waiting..." << std::endl;
			continue ;
		}
		for (size_t i = 0; i < pollFds.size(); ++i)
		{
			if (pollFds[i].revents & POLLIN)
			{
				if (isListeningSocket(pollFds[i].fd, servers))
				{
					if (creatingClientSocket(pollFds, servers, i,
							clientTimestamps) == -1)
						continue ;
				}
				else
				{
					bool http2Detected = false;
					std::string rawRequest = readFullRequest(pollFds[i].fd,
							http2Detected);
					if (rawRequest.empty())
					{
						close(pollFds[i].fd);
                        std::swap(pollFds[i], pollFds.back());
                        pollFds.pop_back();
						i--;
						continue ;
					}
					clientTimestamps[pollFds[i].fd] = time(NULL);
					std::string connectionType = "close";
					HttpRequest request(pollFds[i].fd);
					parseRequestHelper(request, rawRequest, connectionType,
						config);
					if (http2Detected)
						request.setStatusCode(HTTP_VERSION_NOT_SUPPORTED);
					std::string path = request.getPath();
					if ((path.find("/cgi-bin/") == 0
							&& request.getStatusCode() == 200)
						|| (request.getHeader("Content-Type").find("multipart/form-data") != std::string::npos
							&& request.getStatusCode() == 200))
					{
						CgiHandler cgiHandler(rawRequest, request.getMethod(),
							request.getHeader("Content-Type"));
						std::string cgiScript = (request.getHeader("Content-Type").find("multipart/form-data") != std::string::npos) ? "./cgi-bin/upload.py" : "."
							+ path;
						std::string cgiOutput = cgiHandler.executeCgi(cgiScript);
						responseBank[pollFds[i].fd] = cgiOutput;
						if (handleResponse(responseBank, pollFds[i]))
						{
							close(pollFds[i].fd);
                            std::swap(pollFds[i], pollFds.back());
                            pollFds.pop_back();
							i--;
						}
					}
					else
					{
						generateResponseHelper(request, connectionType, pollFds,
							i, responseBank);
					}
				}
			}
		}
	}

	for (size_t i = 0; i < servers.size(); ++i)
	{
		delete servers[i];
	}
	servers.clear();
	pollFds.clear();
	clientTimestamps.clear();
}
	*/

	void runMainLoop(ServerConfig &config)
	{
		std::vector<Socket *> servers;
		std::vector<int> ports;
		std::vector<struct pollfd> pollFds;
		std::map<int, time_t> clientTimestamps;
		std::map<int, std::string> responseBank;
	
		try {
			startVariables(config, ports, pollFds, servers);
			if (!isThereOneValidSocket(servers))
				throw std::runtime_error("No valid listening sockets available. Exiting.");
	
			while (true) {
				for (std::vector<struct pollfd>::iterator it = pollFds.begin(); it != pollFds.end(); ) {
					if (it->fd == -1)
						it = pollFds.erase(it);
					else
						++it;
				}
	
				timingConnections(clientTimestamps, pollFds);
				int ready = poll(&pollFds[0], pollFds.size(), 1000);
				if (ready < 0)
					throw std::runtime_error("poll() failure");
	
				if (pollFds.empty()) {
					std::cout << "No active connections, waiting..." << std::endl;
					continue;
				}
	
				for (size_t i = 0; i < pollFds.size(); ++i) {
					if (pollFds[i].revents & POLLIN) {
						if (isListeningSocket(pollFds[i].fd, servers)) {
							if (creatingClientSocket(pollFds, servers, i, clientTimestamps) == -1)
								continue;
						} else {
							bool http2Detected = false;
							std::string rawRequest = readFullRequest(pollFds[i].fd, http2Detected);
							if (rawRequest.empty()) {
								close(pollFds[i].fd);
								std::swap(pollFds[i], pollFds.back());
								pollFds.pop_back();
								i--;
								continue;
							}
							clientTimestamps[pollFds[i].fd] = time(NULL);
							std::string connectionType = "close";
							HttpRequest request(pollFds[i].fd);
							parseRequestHelper(request, rawRequest, connectionType, config);
							if (http2Detected)
								request.setStatusCode(HTTP_VERSION_NOT_SUPPORTED);
							std::string path = request.getPath();
							if ((path.find("/cgi-bin/") == 0 && request.getStatusCode() == 200) ||
								(request.getHeader("Content-Type").find("multipart/form-data") != std::string::npos && request.getStatusCode() == 200)) {
								CgiHandler cgiHandler(rawRequest, request.getMethod(), request.getHeader("Content-Type"));
								std::string cgiScript = (request.getHeader("Content-Type").find("multipart/form-data") != std::string::npos) ? "./cgi-bin/upload.py" : "." + path;
								std::string cgiOutput = cgiHandler.executeCgi(cgiScript);
								responseBank[pollFds[i].fd] = cgiOutput;
								if (handleResponse(responseBank, pollFds[i])) {
									close(pollFds[i].fd);
									std::swap(pollFds[i], pollFds.back());
									pollFds.pop_back();
									i--;
								}
							} else {
								generateResponseHelper(request, connectionType, pollFds, i, responseBank);
							}
						}
					}
				}
			}
		} catch (const std::exception &e) {
			std::cerr << "Exception caught: " << e.what() << std::endl;
		}
		
		for (size_t i = 0; i < servers.size(); ++i) {
			delete servers[i];
		}
		servers.clear();
		pollFds.clear();
		clientTimestamps.clear();
	}
	