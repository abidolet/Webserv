#include <algorithm>
#include <list>
#include <fstream>

#include "Webserv.hpp"
#include "Block.hpp"
#include "ParserUtils.hpp"
#include "Parser.hpp"

void Server::init(Block &block)
{
	block.loadSingleDirective("server_name", server_name);
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
		if (str[i] != '\0' || i == last_i)
			throw std::runtime_error("syntax error in cookie directive; `" + *it + "'");
	}
}

void Server::runSelfCheck()
{
#if RUN_SERV_SELF_CHECK

	if (Utils::dirAccess(root) == false)
		throw Parser::InvalidDirOrFileException(root);

	// checking for invlid error_page path
	std::map<int, std::string>::iterator it = error_pages.begin();
	for ( ; it != error_pages.end(); ++it)
	{
		if (Utils::fileAccess(it->second) == false)
			throw Parser::InvalidDirOrFileException(it->second);
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

//TODO: assert file
std::map<std::string, int> parseSession(std::vector<std::string>& lines)
{
	std::map<std::string, int> sessions;

	std::vector<std::string>::iterator it = lines.begin();
	for ( ; it != lines.end(); ++it)
	{
		std::vector<std::string> split = Utils::strsplit(*it, ',');
		if (split.size() != 2)
			throw std::runtime_error("broken session file ");
		char* endl;
		sessions.insert(std::pair<std::string, int>(split[0], std::strtol(split[1].c_str(), &endl, 10)));
	}
	return sessions;
}

#if old
//TODO voir avec alexi pour implementer le register session
void Server::registerSession(const std::string& key)
{
	const std::string	sessionFilepath = "./.sessions";
	std::fstream		stream;

	stream.open(sessionFilepath.c_str(), std::fstream::out);
	if (stream.is_open() == false)
		throw Parser::InvalidDirOrFileException(sessionFilepath);

	std::vector<std::string>	lines;
	std::string					line;
	while (std::getline(stream, line))
		lines.push_back(line);
	stream.close();

	stream.open(sessionFilepath.c_str());
	if (stream.is_open() == false)
		throw Parser::InvalidDirOrFileException(sessionFilepath);

	std::map<std::string, int> sessions = parseSession(lines);
	std::map<std::string, int>::iterator it = sessions.begin();
	for ( ; it != sessions.end(); ++it)
	{
		if (it->first != key)
			stream << it->first << "," << it->second << "\n";
		else
		{
			stream << key << "," << (it->second + 1);
		}
	}
	if (sessions.find(key) == sessions.end())
		stream << key << ",0\n";

	stream.close();
}
#endif

void Server::registerSession(const std::string& key)
{
	const std::string	sessionFilepath = "./.sessions";
	std::fstream		stream;

	stream.open(sessionFilepath.c_str(), std::fstream::out);
	if (stream.is_open() == false)
		throw Parser::InvalidDirOrFileException(sessionFilepath);

	std::vector<Session>	sessions;
	std::string				line;
	while (std::getline(stream, line))
		sessions.push_back(Session::stringToSession(line));
	stream.close();

	stream.open(sessionFilepath.c_str());
	if (stream.is_open() == false)
		throw Parser::InvalidDirOrFileException(sessionFilepath);

	std::vector<Session>::iterator it = sessions.begin();
	for ( ; it != sessions.end(); ++it)
	{
		if (it->key != key)
			stream << Session::sessionToString(*it) << "\n";
		else
		{
			it->visitCount++;
			stream << Session::sessionToString(*it) << "\n";
		}
	}
	if (Session::find(sessions, key) == NULL)
	{
		Session session;
		session.key = key;
		stream << Session::sessionToString(*it) << "\n";
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
	if (end[0] != '\0')
	{
		if ((end[0] == 'm' || end[0] == 'M') && end[1] == '\0')
			client_max_body_size *= tmp * 1000000;
		else
			throw std::runtime_error("invalid client_max_body_size: `" + max_size + "'");
	}
	else
		client_max_body_size = tmp;
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
	std::map<std::string, int>	result;

	std::vector<std::string> found = block.loadDirectives("listen");
	if (found.empty())
		return;

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
			result.insert(std::pair<std::string, int>("0.0.0.0", port));
		}
		else
		{
			if (split.empty() || split.size() > 2)
				throw Parser::InvalidDirectiveException("listen", *it);

			port = std::strtol(split[1].c_str(), &endl, 10);
			if (endl[0] != '\0')
				throw Parser::InvalidDirectiveException("listen", *it);

			result.insert(std::pair<std::string, int>(split[0], port));
		}

	}
	listen = result;
}
