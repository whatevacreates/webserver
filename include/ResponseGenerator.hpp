#ifndef RESPONSEGENERATOR_HPP
# define RESPONSEGENERATOR_HPP

# include "HttpRequest.hpp"
# include <fstream>
# include <sstream>

class ResponseGenerator
{
  private:
	std::string _body;
	std::ostringstream _response;

  public:
	ResponseGenerator();
	ResponseGenerator(const ResponseGenerator &other);
	ResponseGenerator &operator=(const ResponseGenerator &other);
	~ResponseGenerator();

	std::string generateResponse(HttpRequest &request,
		const std::string &connectionType);
	std::string getBody() const;
	std::string determineContentType(const std::string &path);
	std::string readFileFromDisk(const HttpRequest &request);
	void getResponse(const HttpRequest &request,
		const std::string &connectionType);
	void postResponse(const HttpRequest &request,
		const std::string &connectionType);
	void deleteResponse(const HttpRequest &request,
		const std::string &connectionType);
	void putResponse(const HttpRequest &request,
		const std::string &connectionType);
	void generateErrorResponse(int statusCode, const std::string &connectionType);
	std::string getStatusMessage(int statusCode) const;
    void generateRedirectionResponse(HttpRequest& request, int statusCode, const std::string &location);
    std::string getResponse() const;
	std::string readErrorPage(int statusCode);
};

#endif
