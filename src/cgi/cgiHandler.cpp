#include "Utils.hpp"
#include "cgiHandler.hpp"
#include <fcntl.h>
#include <csignal>   // SIGKILL, std::raise
#include <signal.h>  // kill (POSIX)
#include <unistd.h>  // getpid (POSIX)


extern char	**environ;

CgiHandler::CgiHandler() : _request(""), _method(""), _contentType("")
{
}

CgiHandler::CgiHandler(const std::string &request, const std::string &method,
	const std::string &contentType) : _request(request), _method(method),
	_contentType(contentType)
{
}

CgiHandler::CgiHandler(const CgiHandler &other)
{
	*this = other;
}

CgiHandler &CgiHandler::operator=(const CgiHandler &other)
{
	if (this != &other)
	{
		_request = other._request;
		_method = other._method;
		_contentType = other._contentType;
	}
	return (*this);
}

CgiHandler::~CgiHandler()
{
}

std::string CgiHandler::unchunkBody(const std::string &body) {
    std::stringstream stream(body);
    std::stringstream unchunked;
    std::string line;
    size_t chunkSize = 0;

    //std::cout << "BODY RECEIVED FOR UNCHUNKING:\n" << body << "|\n" << std::endl;

    while (std::getline(stream, line)) {
        std::cout << "Reading chunk size: " << line << std::endl;

        // Ignore CRLF from the previous chunk (if it exists)
        if (!line.empty() && line[line.size() - 1] == '\r')
            line.erase(line.size() - 1);  

        std::istringstream hexStream(line);
        hexStream >> std::hex >> chunkSize;

        //Remi: Eva we have an error if we keep chunkSize < 0: 
		//comparison of unsigned expression in ‘< 0’ is always false [-Werror=type-limits]
		//if (hexStream.fail() || chunkSize < 0) {  // 🚨 Invalid chunk size (non-hex input)
        if (hexStream.fail()) {
		    std::cerr << "[ERROR] Malformed chunk size: " << line << std::endl;
            return ""; // Return empty to indicate failure
        }

        if (chunkSize == 0) {  // End of chunked data
            std::cout << "[DEBUG] Reached last chunk (0 bytes). Finishing unchunking." << std::endl;
            break;
        }

        std::vector<char> buffer(chunkSize);
        stream.read(&buffer[0], chunkSize);
        if (stream.gcount() != (ssize_t)chunkSize) {  // 🚨 Unexpected EOF inside chunk
            std::cerr << "[ERROR] Incomplete chunk data! Expected: " << chunkSize << " bytes, got: " << stream.gcount() << std::endl;
            return ""; // Return empty to indicate failure
        }

        unchunked.write(&buffer[0], chunkSize);

        // Ensure that there is a CRLF after the chunk
        if (!std::getline(stream, line) || line != "\r") {
            std::cerr << "[ERROR] Malformed chunk termination (missing CRLF after chunk)\n";
            return ""; // 🚨 Invalid format
        }
    }

    std::cout << "UNCHUNKED BODY:\n" << unchunked.str() << "|\n" << std::endl;
    return unchunked.str();
}



/*
std::string CgiHandler::executeCgi(const std::string &scriptPath)
{
	ssize_t	bytesRead;
	int		status;
	int		inputPipe[2], outputPipe[2];
	pid_t	pid;
	ssize_t	bytes;
	char	buffer[1024];
	char	*args[] = {const_cast<char *>(scriptPath.c_str()), NULL};
	size_t	chunkSize;
	std::string queryString;
	std::string path;



	if(scriptPath.find("?") != std::string::npos)
	{
		queryString = scriptPath.substr(scriptPath.find("?") + 1, scriptPath.length()-1);
		std::cout << "-------query string: " << queryString << std::endl;
		path = scriptPath.substr(0, scriptPath.find("?"));

	}
	else
	{
		path = scriptPath;
	}
	
	

	fcntl(inputPipe[0], F_SETFL, O_NONBLOCK);
	fcntl(inputPipe[1], F_SETFL, O_NONBLOCK);
	fcntl(outputPipe[0], F_SETFL, O_NONBLOCK);
	fcntl(outputPipe[1], F_SETFL, O_NONBLOCK);
	bytes = 0;
	if (pipe(inputPipe) == -1 || pipe(outputPipe) == -1)
		throw std::runtime_error("Failed to create pipes");
	pid = fork();
	if (pid < 0)
	{
		close(inputPipe[0]);
		close(inputPipe[1]);
		close(outputPipe[0]);
		close(outputPipe[1]);
		throw std::runtime_error("Fork failed");
	}
	if (pid == 0)
	{
		std::cout << " line 90 cgi handler" << std::endl;
		close(inputPipe[1]);
		close(outputPipe[0]);
		if (dup2(inputPipe[0], STDIN_FILENO) == -1)
			throw std::runtime_error("Failed to redirect inputPipe to STDIN");
		if (dup2(outputPipe[1], STDOUT_FILENO) == -1)
			throw std::runtime_error("Failed to redirect outputPipe to STDOUT");
		std::cout << "this again?" << std::endl;
		close(outputPipe[1]);
		std::ostringstream clStream;
		clStream << _request.size();
		setenv("REQUEST_METHOD", _method.c_str(), 1);
		setenv("CONTENT_LENGTH", clStream.str().c_str(), 1);
		setenv("CONTENT_TYPE", _contentType.c_str(), 1);
		setenv("SCRIPT_FILENAME", path.c_str(), 1);
		

		//td::cout << "checking request method: " << _method.c_str() << std::endl;
		if (_method.compare("GET") == 0)
		{

			setenv("QUERY_STRING", queryString.c_str(), 1);
			std::cout << "checking the query string" <<  queryString.c_str() << std::endl;
			//std::cout << "logging query string: " << queryString.c_str() << std::endl;
		}
		
		std::cout << " line 115 cgi handler" << std::endl;
		execve(path.c_str(), args, environ);
		std::cout << "execve failed" << std::endl;
		//_exit(1);
	}
	else
	{
		close(inputPipe[0]);
		close(outputPipe[1]);
		chunkSize = 1024;
		if (_method.compare("POST") == 0)
		{
			for (size_t i = 0; i < _request.size(); i += chunkSize)
			{
				chunkSize = (_request.size()
						- i > chunkSize) ? chunkSize : (_request.size() - i);
				bytes += write(inputPipe[1], _request.c_str() + i, chunkSize);
			}std::cout << "bytes: " << bytes << std::endl;
		close(inputPipe[1]);
		}
		
		std::ostringstream output;
		while ((bytesRead = read(outputPipe[0], buffer, sizeof(buffer))) > 0)
		{
			output.write(buffer, bytesRead);
		}
		close(outputPipe[0]);
		waitpid(pid, &status, 0);
		std::cout << " line 137 cgi handler" << std::endl;
		std::ostringstream response;
		response << "HTTP/1.1 200 OK\r\n"
					<< "Content-Type: text/html\r\n"
					<< "Content-Length: " << output.str().size() << "\r\n"
					<< "\r\n"
					<< output.str();
		std::cout << " line 144 cgi handler" << std::endl;
		return (response.str());
	}
}
*/

extern char **environ;

std::string CgiHandler::executeCgi(const std::string &scriptPath)
{
    int inputPipe[2], outputPipe[2];
    pid_t pid;
    char buffer[1024];
    ssize_t bytesRead;
    std::string queryString;
    std::string path = scriptPath;
    
    if (scriptPath.find("?") != std::string::npos)
    {
        queryString = scriptPath.substr(scriptPath.find("?") + 1);
        path = scriptPath.substr(0, scriptPath.find("?"));
    }

    if (pipe(inputPipe) == -1 || pipe(outputPipe) == -1)
        throw std::runtime_error("Failed to create pipes");

    pid = fork();
    if (pid < 0)
    {
        close(inputPipe[0]);
        close(inputPipe[1]);
        close(outputPipe[0]);
        close(outputPipe[1]);
        throw std::runtime_error("Fork failed");
    }
    if (pid == 0) // Processus enfant
    {
        close(inputPipe[1]);
        close(outputPipe[0]);

        if (dup2(inputPipe[0], STDIN_FILENO) == -1)
            throw std::runtime_error("Failed to redirect inputPipe to STDIN");
        if (dup2(outputPipe[1], STDOUT_FILENO) == -1)
            throw std::runtime_error("Failed to redirect outputPipe to STDOUT");

        close(inputPipe[0]);
        close(outputPipe[1]);

        // Construction des variables d'environnement
        std::ostringstream clStream;
        clStream << _request.size();

        std::vector<std::string> envVars;
        envVars.push_back("REQUEST_METHOD=" + _method);
        envVars.push_back("CONTENT_LENGTH=" + clStream.str());
        envVars.push_back("CONTENT_TYPE=" + _contentType);
        envVars.push_back("SCRIPT_FILENAME=" + path);
        if (_method == "GET")
            envVars.push_back("QUERY_STRING=" + queryString);

        // Conversion du vector en tableau `char*`
        std::vector<char *> envp;
        for (size_t i = 0; i < envVars.size(); i++)
            envp.push_back(const_cast<char *>(envVars[i].c_str()));
        envp.push_back(NULL); // `execve` attend un tableau NULL-terminé

        char *args[] = {const_cast<char *>(path.c_str()), NULL};

        execve(path.c_str(), args, envp.data());

        std::cerr << "execve failed" << std::endl;
        kill(getpid(), SIGKILL); // Terminer proprement le processus enfant
		return "";
	}
    else // Processus parent
    {
        close(inputPipe[0]);
        close(outputPipe[1]);

        // Lire la sortie du CGI
        std::ostringstream output;
        while ((bytesRead = read(outputPipe[0], buffer, sizeof(buffer))) > 0)
            output.write(buffer, bytesRead);

        close(outputPipe[0]);
        waitpid(pid, NULL, 0);

        // Construire et renvoyer la réponse HTTP
        std::ostringstream response;
        response << "HTTP/1.1 200 OK\r\n"
                 << "Content-Type: text/html\r\n"
                 << "Content-Length: " << output.str().size() << "\r\n"
                 << "\r\n"
                 << output.str();

        return response.str();
    }
}
