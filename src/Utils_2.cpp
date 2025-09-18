#include "ConfigParser.hpp"
#include "FileDescriptorUtil.hpp"
#include "HttpRequest.hpp"
#include "ResponseGenerator.hpp"
#include "Socket.hpp"
#include "Utils.hpp"
#include "cgiHandler.hpp"
#include <sys/stat.h>


void generateResponseHelper(HttpRequest &request, std::string &connectionType,
        std::vector<struct pollfd> &pollFds, size_t &i, std::map<int, std::string> &responseBank)
{
    ResponseGenerator responseGenerator;

    // Si le statut est une erreur, arrête le traitement ici

	std::string response = responseGenerator.generateResponse(request, connectionType);
    responseBank[pollFds[i].fd] = response;

    // Log le statut HTTP pour débogage
    std::istringstream responseStream(response);
    std::string httpVersion;
    int statusCode;
    std::string statusMessage;

    responseStream >> httpVersion >> statusCode >> statusMessage;
    std::cout << "Response Status Code: " << statusCode << std::endl;

	if (handleResponse(responseBank, pollFds[i]))
	{
		if (connectionType == "close")
		{
			close(pollFds[i].fd);
			pollFds.erase(pollFds.begin() + i);
			i--;
		}
	}
}

void	timingConnections(std::map<int, time_t> &clientTimestamps,
		std::vector<struct pollfd> &pollFds)
{
	time_t	now;

	now = time(NULL);
	std::map<int, time_t>::iterator next;
	for (std::map<int,
		time_t>::iterator it = clientTimestamps.begin(); it != clientTimestamps.end();)
	{
		next = it;
		next++;
		if (now - it->second >= 30)
		{
			{
				//std::cout << "Connection Timeout: " << (now
					//- it->second) << "s. Closing the client socket: " << it->first << std::endl;
				close(it->first);
			}
			for (size_t j = 0; j < pollFds.size(); ++j)
			{
				if (pollFds[j].fd == it->first)
				{
					pollFds.erase(pollFds.begin() + j);
					break ;
				}
			}
			clientTimestamps.erase(it);
			it = next;
		}
		else
		{
			++it;
		}
	}
}

bool	handleResponse(std::map<int, std::string> &responseBank,
		pollfd &onePollFd)
{
	ssize_t	bytesSent;

	if (onePollFd.revents & POLLOUT)
	{
		std::map<int,
			std::string>::iterator it = responseBank.find(onePollFd.fd);
		if (it != responseBank.end())
		{
			while (!it->second.empty())
			{
				bytesSent = send(onePollFd.fd, it->second.c_str(),
						it->second.size(), 0);
				if (bytesSent <= 0)
				{
					return (false);
				}
				it->second.erase(0, bytesSent);
				if (bytesSent == 0)
				{
					break ;
				}
			}
			if (it->second.empty())
			{
				responseBank.erase(it);
				return (true);
			}
		}
	}
	return (false);
}
