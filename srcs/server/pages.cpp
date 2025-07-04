#include "Webserv.hpp"
#include <fstream>

static const std::string	getStatusMessage(const int code)
{
	switch(code)
	{
		case 200:	return ("OK");
		case 201:	return ("Created");
		case 301:	return ("Moved Permanently");
		case 400:	return ("Bad Request");
		case 403:	return ("Forbidden");
		case 404:	return ("Not Found");
		case 405:	return ("Method Not Allowed");
		case 409:	return ("Conflict");
		case 413:	return ("Payload Too Large");
		case 500:	return ("Internal Server Error");
		case 501:	return ("Not Implemented");
		case 504:	return ("Gateway Timeout");
		default:	return ("Unknown status");
	}
}

std::string getContentType(const std::string& filename)
{
	if (filename.find(".jpg") != (size_t)-1 || filename.find(".jpeg") != (size_t)-1)
		return "image/jpeg";
	if (filename.find(".png") != (size_t)-1)
		return "image/png";
	if (filename.find(".gif") != (size_t)-1)
		return "image/gif";
	if (filename.find(".html") != (size_t)-1)
		return "text/html";
	if (filename.find(".css") != (size_t)-1)
		return "text/css";
	if (filename.find(".js") != (size_t)-1)
		return "text/javascript";

	return "text/plain";
}

const std::string getCORSHeaders()
{
	return "Access-Control-Allow-Methods: *\r\nAccess-Control-Allow-Origin: *\r\nAccess-Control-Allow-Headers: request_type\r\nAccess-Control-Allow-Headers: UID\r\n";
}

const std::string	generatePage(const int code, const std::string &content, const std::string &name)
{
	return ("HTTP/1.1 " + toString(code) + " " + getStatusMessage(code) + "\r\n" + "Content-Type: " + getContentType(name) + "\r\n"
		+ "Connection: close\r\n" + getCORSHeaders() + "Content-Length: "+ toString(content.size()) + "\r\n\r\n" + content);
}

const std::string	getUrlPage(const int code, const std::string &location)
{
	std::ostringstream	response;
	response << "HTTP/1.1 " << code << " " << getStatusMessage(code) << "\r\n";

	if (!location.empty())
	{
		response << "Location: " << location << "\r\n";
	}

	const std::string	content = "<html><body><h1>Redirecting...</h1></body></html>\r\n";

	response << "Content-Type: text/html\r\n" << "Connection: close\r\n"
		<< "Content-Length: " << content.length() << "\r\n\r\n" << content;

	return (response.str());
}

const std::string	getErrorPage(const int error_code, const Server& server)
{
	Log(Log::DEBUG) << "Searching custom error page for code" << error_code << Log::endl();

	std::map<int, std::string>::const_iterator it = server.error_pages.find(error_code);
	if (it != server.error_pages.end())
	{
		std::ifstream	file(it->second.c_str(), std::ios::binary);
		if (file)
		{
			Log(Log::DEBUG) << "Custom error page found:" << it->second << Log::endl();
			std::string	content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
			return (generatePage(error_code, content, it->second));
		}
	}

	Log(Log::DEBUG) << "Custom error page not found, returning default" << Log::endl();
	std::string	content = "<html><body><h1>" + toString(error_code) + " " + getStatusMessage(error_code) + "</h1></body></html>";
	return (generatePage(error_code, content, ".html"));
}