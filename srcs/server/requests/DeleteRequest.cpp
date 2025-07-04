#include "Webserv.hpp"

#include <sys/stat.h>
#include <cstdio>

const std::string	Webserv::handleDeleteRequest(const std::string& path, const Server& server) const
{
	Log() << "Delete request for: " << path << Log::endl();

	struct stat	statbuf;
	if (stat(path.c_str(), &statbuf) != 0)
	{
		Log(Log::WARNING) << "File not found:" << path << "'" << Log::endl();
		return (getErrorPage(404, server));
	}
	if (S_ISDIR(statbuf.st_mode))
	{
		Log(Log::WARNING) << "Cannot delete directory:" << path << Log::endl();
		return (getErrorPage(403, server));
	}

	if (std::remove(path.c_str()) == 0)
	{
		Log(Log::SUCCESS) << "File deleted !" << path << Log::endl();
		return (generatePage(200, "File deleted successfully\n", ".html"));
	}
	else
	{
		Log(Log::ERROR) << "Cannot delete " << path << Log::endl();
		return (getErrorPage(403, server));
	}
}
