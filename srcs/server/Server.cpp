#include <algorithm>
#include <list>
#include <fstream>

#include "Webserv.hpp"
#include "Block.hpp"
#include "ParserUtils.hpp"
#include "Parser.hpp"

std::vector<Session> readSessions(const std::string& sessionFilepath);

std::map<std::string, std::string> getData(std::string body)
{
	std::map<std::string, std::string> data;

	std::vector<std::string> argSplit = Utils::strsplit(body, '&');
	std::vector<std::string>::iterator it = argSplit.begin();

	for ( ; it != argSplit.end(); ++it)
	{
		std::vector<std::string> subSplit = Utils::strsplit(*it, '=');
		if (subSplit.size() != 2)
			throw std::runtime_error("wtf is that request");
		data.insert(std::pair<std::string, std::string>(subSplit[0], subSplit[1]));
	}
	return data;
}

std::string Server::handlePostRequest(HttpRequest request) const
{
	std::map<std::string, std::string>::iterator it = request.headers.find("request_type");
	if (it == request.headers.end())
		return request.body;

	std::stringstream ss;

	if (it->second == "client_credentials")
	{
		ss << lastUID;
		return ss.str();
	}
	else if (it->second == "upload")
	{
		std::string filename = "upload_" + toString(time(NULL));
		std::map<std::string, std::string>::const_iterator it = request.headers.find("Content-Disposition");
		if (it != request.headers.end())
		{
			size_t pos = it->second.find("filename=\"");
			if (pos != std::string::npos)
			{
				filename = it->second.substr(pos + 10);
				filename = filename.substr(0, filename.find("\""));
			}
		}

		std::string filepath = request.location.path + request.location.upload_dir + "/" + filename;
		Log(Log::ALERT) << filepath << Log::endl();
		std::ofstream outfile(filepath.c_str(), std::ios::binary);
		if (!outfile)
		{
			return (generatePage(500, "Failed to open file for writing: " + filepath));
		}
		outfile.write(request.body.c_str(), request.body.size());
		outfile.close();

		return (generatePage(201, "File uploaded successfully to " + filepath));
	}
	if (it->second == "client_visits")
	{
		std::map<std::string, std::string>::iterator uid = request.headers.find("UID");
		if (uid == request.headers.end())
			return "missing uid options";

		std::vector<Session> sessions = readSessions("./.sessions");
		ss << Session::find(sessions, std::atoi(uid->second.c_str()))->visitCount;
		return ss.str();
	}

	return request.body;
}

void Server::init(Block &block)
{
	server_names = block.loadDirectives("server_name");
	for (size_t i = 0; i < server_names.size(); i++)
	{
		for (int j = i - 1; j >= 0; j--)
		{
			if (server_names[j] == server_names[i])
				throw std::runtime_error("cannot have duplicate server name; `" + server_names[i] + "'");
		}
	}

	block.loadSingleDirective("root", root);
	setupListen(block);
	setupMaxBodySize(block);
	setupRedirections(block);
}

void Server::cookiesAssert()
{
	std::vector<std::string>::iterator it = cookies.begin();
	for ( ; it != cookies.end(); ++it)
	{
		size_t i = 0;
		std::string str = *it;
		while (std::isalnum(str[i]) || str[i] == '_')
			i++;
		if (i == 0)
			throw std::runtime_error("syntax error in cookie directive; `" + *it + "'");
		if (str[i] == '=')
			i++;
		size_t last_i = i;
		while (std::isalnum(str[i]) || str[i] == '_')
			i++;
		if (str[i] != ';' && (str[i] != '\0' || i == last_i))
			throw std::runtime_error("syntax error in cookie directive; `" + *it + "'");
	}
}

std::string Server::getCookies() const
{
	const std::string declaration = "Set-Cookie: ";
	std::string cookies;

	for (size_t i = 0; i < cookies.size(); i++)
	{
		cookies += declaration;
		cookies += cookies[i];
		cookies += "\r\n";
	}
	//TODO: need to add the session uid as a cookie
	std::vector<Session> sessions = readSessions("./.sessions");
	Session* session = Session::find(sessions, lastUID);

	if (session != NULL)
	{
		std::stringstream ss;
		ss << session->uid;
		cookies += declaration;
		cookies += "session_uid=" + ss.str();
		cookies += "\r\n";
	}

	return cookies;
}

std::string Server::getCookiesCgi() const
{
	return Utils::strUnite(cookies, ";");
}


void Server::runSelfCheck()
{
#if RUN_SERV_SELF_CHECK

	if (Utils::dirAccess(root) == false)
		Log(Log::WARNING) << Parser::InvalidDirOrFileException(root).what() << Log::endl();

	// checking for invlid error_page path
	std::map<int, std::string>::iterator it = error_pages.begin();
	for ( ; it != error_pages.end(); ++it)
	{
		if (Utils::fileAccess(it->second) == false)
			Log(Log::WARNING) << Parser::InvalidDirOrFileException(it->second).what() << Log::endl();
	}

	// checking if mutiple location are the same
	std::vector<std::string> names;
	for (size_t i = 0; i < locations.size(); i++)
	{
		for (size_t j = 0; j < names.size(); j++)
		{
			if (names[j] == locations[i].root)
				throw std::runtime_error("duplicate location directive; `" + names[j] + "'");
		}
		names.push_back(locations[i].root);
	}

	Log(Log::SUCCESS) << "server self check pass!" << Log::endl();
#else
	Log(Log::WARNING) << "server self checks are disable (set RUN_SERV_SELF_CHECK to 1 to enable)" << Log::endl();
#endif
}

Location* Server::searchLocationByName(const std::string& name)
{
	std::vector<Location>::iterator it = locations.begin();
	for ( ; it != locations.end(); ++it)
	{
		if (it->root == name)
			return &(*it);
	}
	return NULL;
}

void createFile(const std::string& filepath)
{
	std::ofstream stream;

	stream.open(filepath.c_str());
	if (stream.is_open() == false)
		throw std::runtime_error("cannot open file; `" + filepath + "'");
	stream.close();
}

std::vector<Session> readSessions(const std::string& sessionFilepath)
{

	if (Utils::fileAccess(sessionFilepath) == false)
		createFile(sessionFilepath);

	std::ifstream			stream;
	std::vector<Session>	sessions;
	std::string				line;

	stream.open(sessionFilepath.c_str());
	if (stream.is_open() == false)
		throw Parser::InvalidDirOrFileException(sessionFilepath);

	while (std::getline(stream, line))
	{
		if (line.empty())
			continue;
		sessions.push_back(Session::stringToSession(line));
	}
	stream.close();

	return sessions;
}

void Server::registerSession(const uint uid)
{
	const std::string	sessionFilepath = "./.sessions";
	std::ofstream		stream;
	std::vector<Session> sessions = readSessions(sessionFilepath);

	stream.open(sessionFilepath.c_str());
	if (stream.is_open() == false)
		throw Parser::InvalidDirOrFileException(sessionFilepath);

	if (sessions.size() > 0)
	{
		std::vector<Session>::iterator it = sessions.begin();
		for ( ; it != sessions.end(); ++it)
		{
			if (it->uid != uid)
				stream << it->sessionToString() << "\n";
			else
			{
				it->visitCount++;
				stream << it->sessionToString() << "\n";
			}
		}
	}
	if (Session::find(sessions, uid) == NULL)
	{
		Session session(uid);
		stream << session.sessionToString() << "\n";
	}

	stream.close();
}

void Server::setupMaxBodySize(Block& block)
{
	std::vector<std::string> found = block.loadDirectives("client_max_body_size");
	if (found.empty())
		return;

	char* end;
	std::string max_size = found[0];

	float tmp = std::strtod(max_size.c_str(), &end);
	if (tmp < 0)
		throw std::runtime_error("invalid client_max_body_size: `" + max_size + "'");
	client_max_body_size = tmp;
	if (end[0] != '\0')
	{
		if ((end[0] == 'm' || end[0] == 'M') && end[1] == '\0')
			client_max_body_size *= 1000000;
		else
			throw std::runtime_error("invalid client_max_body_size: `" + max_size + "'");
	}
}

void Server::setupRedirections(Block& block)
{
	std::vector<std::string> found = block.loadDirectives("error_page");
	if (found.empty())
		return;

	for (size_t j = 0; j < found.size(); j++)
	{
		std::string str = found[j];
		std::vector<std::string> redirs = Utils::strsplit(str, ' ');
		if (redirs.size() <= 1)
			throw Parser::InvalidDirectiveException("error_page", str);

		// isolating the error page
		std::string error_page = redirs[redirs.size() - 1];
		redirs.pop_back();

		if (Utils::is_number(redirs) == false)
			if (error_page[0] != '/')
				throw Parser::InvalidDirectiveException("error_page", str);

		for (size_t i = 0; i < redirs.size(); i++)
		{
			int code = std::atoi(redirs[i].c_str());
			if (error_pages.find(code) != error_pages.end())
				error_pages.find(code)->second = error_page;
			else
				error_pages.insert(std::pair<int, std::string>(code, error_page));
		}
	}
}

void assertListen(const std::string& listen)
{
	if (listen.find(':') == (size_t)-1) // so listen is only the port (e.g: 443)
	{
		if (Utils::is_number(listen) == false)
			throw Parser::InvalidDirectiveException("listen", listen);
		return;
	}

	// listen must be in this format: aa.bb.cc.dd:ee
	size_t ipPart = 0;
	for (size_t i = 0; i < listen.size(); )
	{
		size_t longestSpan = 0;
		while (std::isdigit(listen[i]))
		{
			longestSpan++;
			i++;
		}
		if (i == listen.size()) // we are at the host part of the config
			return;

		if (longestSpan > 3)
			throw Parser::InvalidDirectiveException("listen", listen);

		longestSpan = 0;
		char separator = ipPart == 3 ? ':' : '.';
		while (listen[i] == separator)
		{
			longestSpan++;
			i++;
		}
		if (longestSpan != 1)
			throw Parser::InvalidDirectiveException("listen", listen);
		ipPart++;
	}

	(void)listen;
}

void	Server::setupListen(Block &block)
{
	std::vector<std::string> found = block.loadDirectives("listen");
	if (found.empty())
		return;

	std::vector<Listen> result;
	std::vector<std::string>::iterator it = found.begin();
	for ( ; it != found.end(); ++it)
	{
		char *endl;
		int port;

		assertListen(*it);
		std::vector<std::string> split = Utils::strsplit(*it, ':');
		if (split.size() == 1)
		{
			port = std::strtol(split[0].c_str(), &endl, 10);
			if (endl[0] != '\0')
				throw Parser::InvalidDirectiveException("listen", *it);
			result.push_back(Listen("0.0.0.0", port));
		}
		else
		{
			if (split.empty() || split.size() > 2)
				throw Parser::InvalidDirectiveException("listen", *it);

			port = std::strtol(split[1].c_str(), &endl, 10);
			if (endl[0] != '\0')
				throw Parser::InvalidDirectiveException("listen", *it);

			result.push_back(Listen(split[0], port));
		}
	}
	listen = result;
}


//
// Listen
//
size_t find(std::vector<Listen> vec, Listen toFind)
{
	for (size_t i = 0; i < vec.size(); i++)
	{
		if (toFind == vec[i])
			return i;
	}
	return -1;
}

bool Listen::operator==(const Listen& other)
{
	return (other.addr == addr && other.port == port);
}
