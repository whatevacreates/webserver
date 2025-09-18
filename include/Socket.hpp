#ifndef SOCKET_HPP
#define SOCKET_HPP

#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#include <stdexcept>
#include <cstring>
#include <iostream>
#include <cerrno>
#include <vector>
#include <poll.h>

class Socket {
private:
    int                _socketFd;
    struct sockaddr_in _serverAddress;
    int                _port;

    // Copy constructor/assignment are private to prevent accidental copying.
    Socket(const Socket&);
    Socket& operator=(const Socket&);

public:
    Socket(int port);
    void bindSocket();
    void listenSocket();
    int  acceptConnection();
    int  getSocketFd() const;
    ~Socket();
};

#endif
