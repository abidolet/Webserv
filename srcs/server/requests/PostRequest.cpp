#include "Webserv.hpp"

#include <sys/stat.h>
#include <ctime>
#include <fstream>

std::string getDirContent(const HttpRequest& request);
std::string getClientVisits(const HttpRequest& request);
std::string upload(const HttpRequest& request);


const std::string	Webserv::handlePostRequest(const HttpRequest& request, const Server& server) const
{
	if (request.body.size() > server.client_max_body_size && server.client_max_body_size > 0)
	{
		Log(Log::WARNING) << "Body size exceeds client_max_body_size" << Log::endl();
		return (getErrorPage(413, server));
	}

	std::map<std::string, std::string>::const_iterator	it = request.headers.find("request_type");
	if (it == request.headers.end())
	{
		return (getErrorPage(200, server));
	}

	Log(Log::DEBUG) << "Handling POST request with request_type:" << it->second << Log::endl();

	std::stringstream ss;
	if (it->second == "client_credentials")
	{
		ss << server.lastUID;
		return generatePage(200, ss.str(), ".html");
	}
	else if (it->second == "dir_content")
	{
		return getDirContent(request);
	}
	else if (it->second == "client_visits")
	{
		return getClientVisits(request);
	}
	else if (it->second == "upload")
	{
		return upload(request);
	}

	return (generatePage(200, request.body, ".html"));
}

//
// return every file of the location
//
std::string getDirContent(const HttpRequest& request)
{
	std::vector<File> files = getFilesInDir(request.path);
	std::stringstream ss;

	for (size_t i = 0; i < files.size(); i++)
	{
		ss << files[i].name;
		if (i + 1 < files.size())
			ss << ";";
	}

	return generatePage(200, ss.str(), ".html");
}

//
// return the number of request made by the client
//
std::string getClientVisits(const HttpRequest& request)
{
	std::stringstream ss;
	std::map<std::string, std::string>::const_iterator uid = request.headers.find("UID");
	
	if (uid == request.headers.end())
		return generatePage(404, "missing UID header", ".html");
	
	std::vector<Session> sessions = readSessions("./.sessions");
	Session* session = Session::find(sessions, std::atoi(uid->second.c_str()));

	if (session == NULL)
		return generatePage(404, "ressource not found", ".html");
	
	ss << session->visitCount;
	return generatePage(200, ss.str(), ".html");
}

//
// try to upload a file to the server then return if it succeded
//
std::string upload(const HttpRequest& request)
{
	Log(Log::DEBUG) << "Handling upload request" << Log::endl();
	std::string	filename = "upload_" + toString(std::time(NULL));
	std::map<std::string, std::string>::const_iterator	it = request.headers.find("Content-Disposition");
	if (it != request.headers.end())
	{
		size_t pos = it->second.find("filename=\"");
		if (pos != std::string::npos)
		{
			filename = it->second.substr(pos + 10);
			filename = filename.substr(0, filename.find("\""));
		}
	}

	std::string	filepath = request.location.path + request.location.upload_dir + "/" + filename;
	Log(Log::DEBUG) << "Saving file to:" << filepath << Log::endl();

	struct stat	statbuf;
	if (stat(filepath.c_str(), &statbuf) == 0)
	{
		return (getErrorPage(409, request.server));
	}

	std::ofstream	outfile(filepath.c_str(), std::ios::binary);
	if (!outfile)
	{
		return (generatePage(500, "Failed to create file: " + filepath, ".html"));
	}

	outfile.write(request.body.data(), request.body.size());
	outfile.close();

	return (generatePage(201, "File saved as " + filename, ".html"));
}
