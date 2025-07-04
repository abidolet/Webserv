#include "Webserv.hpp"
#include <sys/stat.h>
#include <cstring>
#include <fstream>
#include <iostream>

const std::string	Webserv::handleGetRequest(HttpRequest& request, const Server& server) const
{
	std::string	path = request.path;

	struct stat	statbuf;
	if (stat(path.c_str(), &statbuf) == 0 && S_ISDIR(statbuf.st_mode))
	{
		Log(Log::SUCCESS) << "trying to get to dir listing" << Log::endl();
		if (request.location.directoryListing)
			return getDirectoryListing(request);
		Log(Log::WARNING) << "directory listing is off:'" << path << "'" << Log::endl();
		return (getErrorPage(403, server));
	}

	std::memset(&statbuf, 0, sizeof(struct stat));
	if (stat(path.c_str(), &statbuf) != 0)
	{
		Log(Log::WARNING) << "File not found:'" << path << "'" << Log::endl();
		return (getErrorPage(404, server));
	}
	if (S_ISDIR(statbuf.st_mode))
	{
		Log(Log::SUCCESS) << "trying to get to dir listing" << Log::endl();
		if (request.location.directoryListing)
		{
			return (getDirectoryListing(request));
		}

		Log(Log::WARNING) << "directory listing is off:'" << path << "'" << Log::endl();
		return (getErrorPage(403, server));
	}

	Log(Log::DEBUG) << "File found:" << path << Log::endl();

	std::ifstream	file(path.c_str(), std::ios::binary);
	if (!file)
	{
		Log(Log::WARNING) << "Cannot open " << path << Log::endl();
		return (getErrorPage(403, server));
	}

	if (isTTY(path.c_str()))
	{
		return getErrorPage(404, server); // firefox return 404, chrome nothing
	}

	std::string	content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());

	std::string	page = generatePage(200, content, path);
	size_t		header_end = page.find("Content-Type");

	if (header_end != std::string::npos)
	{
		page.insert(header_end, server.getCookies());
	}

	return (page);
}
