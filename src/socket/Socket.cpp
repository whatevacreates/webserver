#include "Socket.hpp"

Socket::Socket(int port) : _port(port)
{
	int	opt;

	_socketFd = socket(AF_INET, SOCK_STREAM, 0);
	std::cout << "[Socket] Created socket fd=" << _socketFd << " for port " << port << std::endl;
	if (_socketFd < 0)
	{
		std::cerr << "[Socket] socket() error: " << strerror(errno) << std::endl;
		//throw std::runtime_error("Failed to create socket");
        _socketFd = -1;
        return;
	}
	memset(&_serverAddress, 0, sizeof(_serverAddress));
	_serverAddress.sin_family = AF_INET;
	_serverAddress.sin_port = htons(_port);
	_serverAddress.sin_addr.s_addr = INADDR_ANY;
	opt = 1;
	if (setsockopt(_socketFd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0)
	{
		std::cerr << "[Socket] Failed to set SO_REUSEADDR: " << strerror(errno) << std::endl;
		//throw std::runtime_error("Failed to set socket options");
        close(_socketFd);
        return;
	}
	std::cout << "[Socket] Socket constructor done for port " << port << std::endl;
}

void Socket::bindSocket()
{
    if(_socketFd == -1)
    {
        std::cerr << "[Socket] Cannot bind, invalid socket." << std::endl;
        return;
    }
	std::cout << "[Socket] Attempting bind on fd=" << _socketFd << " for port " << _port << std::endl;
	if (bind(_socketFd, (struct sockaddr *)&_serverAddress,
			sizeof(_serverAddress)) < 0)
	{
		std::cerr << "[Socket] bind() error: " << strerror(errno) << " (errno=" << errno << ")" << std::endl;
		//throw std::runtime_error("Failed to bind socket");
        close(_socketFd);
        _socketFd = -1;
        return;
	}
	std::cout << "[Socket] Socket bound to port " << _port << std::endl;
}

void Socket::listenSocket()
{
    if(_socketFd == -1)
    {
        std::cerr << "[Socket] Cannot listen, invalid socket." << std::endl;
        return;
    }
	std::cout << "[DEBUG Socket] Attempting listen on fd=" << _socketFd << " for port " << _port << std::endl;
	if (listen(_socketFd, 100) < 0)
	{
		std::cerr << "[DEBUG Socket] listen() error: " << strerror(errno) << " (errno=" << errno << ")" << std::endl;
		//throw std::runtime_error("Failed to listen on socket");
        close(_socketFd);
        _socketFd = -1;
        return;
	}
	std::cout << "[DEBUG Socket] Listening on port " << _port << std::endl;
}

int Socket::acceptConnection()
{
	if(_socketFd == -1)
    {
        std::cerr << "[Scoket] Cannot accept connections, invalid listening socket." << std::endl;
        return -1;
    }
    
    int	clientFd;


	//std::cout << "[DEBUG Socket] acceptConnection() called on fd=" << _socketFd << std::endl;
	clientFd = accept(_socketFd, NULL, NULL);
	if (clientFd < 0)
	{
		std::cerr << "[DEBUG Socket] accept() error: " << strerror(errno) << " (errno=" << errno << ")" << std::endl;
		//throw std::runtime_error("Failed to accept connection");
        return -1;
	}
	//std::cout << "[DEBUG Socket] Client accepted, fd=" << clientFd << std::endl;
	return (clientFd);
}

int Socket::getSocketFd() const
{
	return (_socketFd);
}

Socket::~Socket()
{
	if (_socketFd >= 0)
	{
		std::cout << "[Socket] Closing socket fd=" << _socketFd << std::endl;
		close(_socketFd);
	}
}
