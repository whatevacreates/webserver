#include "ConfigParser.hpp"
#include "FileDescriptorUtil.hpp"
#include "HttpRequest.hpp"
#include "ResponseGenerator.hpp"
#include "Socket.hpp"
#include "Utils.hpp"
#include "cgiHandler.hpp"

bool parseHeader(bool &chunked, std::string &rawRequest, size_t &contentLength, size_t &bodyStartPos) {
    size_t pos;
    size_t contentLengthPos;
    size_t start;
    size_t end;
    size_t tePos;

    std::string method;
    pos = rawRequest.find("\r\n\r\n");
    if (pos != std::string::npos) {
        bodyStartPos = pos + 4;
        std::string headersBlock = rawRequest.substr(0, pos);
        {
            static const std::string clHeader = "Content-Length:";
            contentLengthPos = headersBlock.find(clHeader);
            if (contentLengthPos != std::string::npos) {
                start = contentLengthPos + clHeader.size();
                while (start < headersBlock.size() && (headersBlock[start] == ' ' || headersBlock[start] == '\t')) {
                    start++;
                }
                end = headersBlock.find("\r\n", start);
                if (end != std::string::npos) {
                    std::string contentLengthStr = headersBlock.substr(start, end - start);
                    int contentLengthInt = std::atoi(contentLengthStr.c_str());
                    if (contentLengthInt < 0) {
                        std::cerr << "[ERROR] Invalid Content-Length: " << contentLengthInt << std::endl;
                        return false;
                    }
                    contentLength = static_cast<size_t>(contentLengthInt);
                }
            }
        }
        {
            static const std::string teHeader = "Transfer-Encoding:";
            tePos = headersBlock.find(teHeader);
            if (tePos != std::string::npos) {
                start = tePos + teHeader.size();
                while (start < headersBlock.size() && (headersBlock[start] == ' ' || headersBlock[start] == '\t')) {
                    start++;
                }
                end = headersBlock.find("\r\n", start);
                if (end != std::string::npos) {
                    std::string teValue = headersBlock.substr(start, end - start);
                    if (teValue.find("chunked") != std::string::npos) {
                        chunked = true;
                    }
                }
            }
        }
        return true;
    }
    return false;
}

std::string readFullRequest(int clientFd, bool &http2Detected)
{
	char	buffer[8192];
	ssize_t	bytesRead;
	bool	headersParsed;
	size_t	contentLength;
	bool	chunked;
	size_t	bodyStartPos;
	size_t	bodySize;

	std::string rawRequest;
	rawRequest.reserve(8192);
	headersParsed = false;
	contentLength = 0;
	chunked = false;
	bodyStartPos = 0;
	http2Detected = false;

	while ((bytesRead = read(clientFd, buffer, sizeof(buffer))) > 0)
	{
		rawRequest.append(buffer, bytesRead);
        if (rawRequest.find("PRI * HTTP/2.0") == 0) {
            std::cerr << "[ERROR] HTTP/2 detected, rejecting with 505" << std::endl;
            http2Detected = true;
        }
		if (!headersParsed)
		{
			headersParsed = parseHeader(chunked, rawRequest, contentLength,
					bodyStartPos);
		}
		if(!headersParsed && contentLength == 0)
		{
			std::cerr << "[ERROR] Invalid headers or Content-Length" << std::endl;
			break;
		}
		if (headersParsed)
		{
			if (chunked)
			{
				break ;
			}
			bodySize = rawRequest.size() - bodyStartPos;
			if (contentLength == 0)
			{
				break ;
			}
			else
			{
				if (bodySize >= contentLength)
				{
					break ;
				}
				//std::cout << "[DEBUG] Body size: " << bodySize << " Content-Length: " << contentLength << std::endl;
				/*
				if (bodySize < contentLength)
				{
					std::cerr << "[ERROR] Body size exceeds content length" << std::endl;
					break ;
				}
					*/
			}
		}
	}
	//DEBUG
	//std::cout << B_PINK << "RAW REQUEST:" << rawRequest << "\n" << RESET <<std::endl;
	return (rawRequest);
}

bool	isListeningSocket(int fd, const std::vector<Socket *> &servers)
{
	for (size_t i = 0; i < servers.size(); ++i)
	{
		if (servers[i]->getSocketFd() == fd)
			return (true);
	}
	return (false);
}

void	startVariables(const ServerConfig &config, std::vector<int> &ports,
		std::vector<struct pollfd> &pollFds, std::vector<Socket *> &servers)
{
	Socket			*sock;
	struct pollfd	pfd;

	ports = config.listen_ports;
	servers.reserve(ports.size());
	for (size_t i = 0; i < ports.size(); ++i)
	{
		sock = new Socket(ports[i]); 
		sock->bindSocket();
		sock->listenSocket();
		servers.push_back(sock);
	}
	pollFds.reserve(servers.size());
	for (size_t i = 0; i < servers.size(); ++i)
	{
		pfd.fd = servers[i]->getSocketFd();
		pfd.events = POLLIN | POLLOUT;
		pfd.revents = 0;
		pollFds.push_back(pfd);
	}
}

int	creatingClientSocket(std::vector<struct pollfd> &pollFds,
		std::vector<Socket *> &servers, size_t &i, std::map<int,
		time_t> &clientTimestamps)
{
	//std::cout << "[DEBUG main] Accepting new connection on fd " << pollFds[i].fd << std::endl;

	Socket *serverSock = NULL;

	for (size_t s = 0; s < servers.size(); ++s)
	{
		if (servers[s]->getSocketFd() == pollFds[i].fd)
		{
			serverSock = servers[s];
			break ;
		}
	}
	if (!serverSock)
	{
		std::cerr << "[DEBUG main] Error: listening socket not found" << std::endl;
		return (-1);
	}

	int clientFd = serverSock->acceptConnection();
	/*
	#ifdef __APPLE__
		if (!FileDescriptorUtil::setNonBlockingMode(clientFd))
		{
			std::cerr << "Failed to set non-blocking mode on socket" << std::endl;

			close(clientFd);
			clientTimestamps.erase(clientFd);
			pollFds.erase(pollFds.begin() + i);
			i--;
			return (-1);
		}
	#endif
	*/

	struct pollfd clientPfd;
	clientPfd.fd = clientFd;
	clientPfd.events = POLLIN | POLLOUT;
	clientPfd.revents = 0;
	pollFds.push_back(clientPfd);
	clientTimestamps[clientFd] = time(NULL);
	//<< "[DEBUG main] Accepted new client fd=" << clientFd << std::endl;
	return (0);
}