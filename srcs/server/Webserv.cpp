#include "Webserv.hpp"
#include "Parser.hpp"
#include "Log.hpp"
#include <sys/epoll.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <algorithm>
#include <dirent.h>
#include "ParserUtils.hpp"
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>

#define MAX_EVENTS 1024

Webserv::Webserv(const std::string& file)
	: _servers(), _epoll_fd(-1), _listener_fds()
{
	Parser	parser(file);
	_servers = parser.populateServerInfos();
}

Webserv::~Webserv()
{
	CLOSE(_epoll_fd);
	for (unsigned int i = 0; i < _listener_fds.size(); ++i)
	{
		CLOSE(_listener_fds[i]);
	}
}

const std::string	toString(const int value)
{
	std::ostringstream	oss;
	oss << value;
	return (oss.str());
}

static const std::string	getStatusMessage(const int code)
{
	switch(code)
	{
		case 200:	return ("OK");
		case 403:	return ("Forbidden");
		case 404:	return ("Not Found");
		case 405:	return ("Method Not Allowed");
		case 413:	return ("Payload Too Large");
		case 500:	return ("Internal Server Error");
		default:	return ("Unknown code");
	}
}

const std::string	generatePage(const int code, const std::string &content)
{
	return ("HTTP/1.1 " + toString(code) + " " + getStatusMessage(code) + "\r\n" + "Content-Type: text/html\r\n"
		+ "Connection: close\r\n" + "Content-Length: " + toString(content.size()) + "\r\n\r\n" + content);
}

const std::string	Webserv::getUrlPage(const int code, const std::string &content, const std::string &location) const
{
	std::ostringstream	response;
	response << "HTTP/1.1 " << code << " " << getStatusMessage(code) << "\r\n";

	if (!location.empty() && (code == 301 || code == 302))
	{
		response << "Location: " << location << "\r\n";
	}

	response << "Content-Type: text/html\r\n" << "Connection: close\r\n"
		<< "Content-Length: " << content.size() << "\r\n\r\n" << content;

	return (response.str());
}

const std::string	Webserv::getErrorPage(const int error_code, const Server& server) const
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
			return (generatePage(error_code, content));
		}
	}

	Log(Log::DEBUG) << "Custom error page not found, returning default" << Log::endl();
	std::string	content = "<html><body><h1>" + toString(error_code) + " " + getStatusMessage(error_code) + "</h1></body></html>";
	return (generatePage(error_code, content));
}

const HttpRequest Webserv::parseRequest(const std::string& rawRequest, const Server& server) const
{
	std::istringstream	stream(rawRequest);
	std::string			line;
	HttpRequest			request;

	if (std::getline(stream, line))
	{
		Log(Log::DEBUG) << "Request line:" << line << Log::endl();

		std::istringstream	request_line(line);
		request_line >> request.method >> request.path;
		request.path = '/' + Utils::strtrim(request.path, "/");

		Log(Log::DEBUG) << "Method:" << request.method << "| Path:" << request.path << Log::endl();
	}

	Log(Log::DEBUG) << "Headers:" << Log::endl();
	while (std::getline(stream, line) && line != "\r" && !line.empty())
	{
		Log(Log::DEBUG) << "Header line:" << line << Log::endl();
		size_t	pos = line.find(':');
		if (pos != std::string::npos)
		{
			std::string	key = line.substr(0, pos);
			std::string	value = line.substr(pos + 1);

			size_t		start = value.find_first_not_of(" \t\r\n");
			if (start != std::string::npos)
			{
				size_t	end = value.find_last_not_of(" \t\r\n");
				value = value.substr(start, end - start + 1);
			}

			request.headers[key] = value;
			Log(Log::DEBUG) << "Header parsed:" << key << "=" << value << Log::endl();
		}
	}

	size_t	body_pos = rawRequest.find("\r\n\r\n");
	if (body_pos != std::string::npos)
	{
		request.body = rawRequest.substr(body_pos + 4);
		Log(Log::DEBUG) << "Body size:" << request.body.size() << "bytes" << Log::endl();
	}

	Log(Log::DEBUG) << "Checking location for:" << request.path << Log::endl();
	const Location*	best_match = NULL;
	size_t	best_match_length = 0;
	request.method_allowed = false;

	for (size_t	i = 0; i < server.locations.size(); ++i)
	{
		const Location&	loc = server.locations[i];
		Log(Log::DEBUG) << "Checking location" << i << ":" << loc.root << Log::endl();

		if (request.path.compare(0, loc.root.length(), loc.root) == 0)
		{
			Log(Log::DEBUG) << "Potential match found:" << loc.root << "(length:" << loc.root.length() << ")" << Log::endl();

			if (loc.root.length() > best_match_length)
			{
				best_match = &loc;
				best_match_length = loc.root.length();
				Log(Log::DEBUG) << "New best match:" << best_match->root << Log::endl();
			}
		}
	}

	if (best_match)
	{
		Log(Log::DEBUG) << "Best location match:" << best_match->root << "with path:" << best_match->path << Log::endl();
		request.location = *best_match;
		request.path = request.path.substr(best_match->root.length());
		Log(Log::DEBUG) << "Checking allowed methods for location..." << Log::endl();
		for (std::vector<std::string>::const_iterator it = best_match->allowed_methods.begin();
			it != best_match->allowed_methods.end(); ++it)
		{
			if (*it == request.method)
			{
				request.method_allowed = true;
				break ;
			}
		}

		if (!request.method_allowed)
		{
			Log(Log::WARNING) << "Method" << request.method << "not allowed for this location" << Log::endl();
			return (request);
		}

		std::string	full_path = best_match->path;
		Log(Log::DEBUG) << "Constructing full path from location path:" << full_path << Log::endl();
		full_path += '/' + request.path;
		Log(Log::DEBUG) << "Full path constructed:" << full_path << Log::endl();

		Log(Log::DEBUG) << "Checking file stats for:" << full_path << Log::endl();
		struct stat	statbuf;
		if (stat(full_path.c_str(), &statbuf) == 0 && S_ISDIR(statbuf.st_mode) && !best_match->directoryListing)
		{
			std::string	index = best_match->index;
			Log(Log::DEBUG) << "Path is a directory adding index:" << index << Log::endl();
			full_path += best_match->index;
			Log(Log::DEBUG) << "Added index file:" << full_path << Log::endl();
		}

		request.path = full_path;
		request.method_allowed = true;
		Log(Log::DEBUG) << "Final path:" << request.path << Log::endl();
	}
	else
	{
		Log(Log::WARNING) << "No matching location found for path:" << request.path << "adding root:" << server.root << Log::endl();
		request.path = server.root + request.path;

		if (std::find(server.allowed_methods.begin(), server.allowed_methods.end(), request.method) != server.allowed_methods.end())
		{
			request.method_allowed = true;
		}
	}
	return (request);
}

File getFile(std::string filename, struct dirent* infos)
{
	struct stat st;
	stat(filename.c_str(), &st);

	return (File) {
		.name=infos->d_name,
		.size=(st.st_size),
	};
}

std::vector<File> getFilesInDir(const std::string path)
{
	std::vector<File> files;

	DIR* dir = opendir(path.c_str());
	if (dir == NULL)
	{
		Log(Log::WARNING) << "Directory listing is enable but dir can't be open";
		return files;
	}
	struct dirent* info;
	while ((info = readdir(dir)) != NULL)
	{
		files.push_back(getFile(path + "/" + info->d_name, info)); // TODO: maybe add other info
	}
	closedir(dir);
	return files;
}

std::string getElt(const File& file, const std::string& path)
{
	Log(Log::WARNING) << path + " | " + file.name << Log::endl();
	std::stringstream ss;

	std::string uri = path[path.size() - 1] == '/' ? path + file.name : path + "/" + file.name;
	ss << "<div class='elt'><a href='" << uri << "'>" \
	 << file.name << "</a> <p>" << file.size << "bytes</p></div>";
	return ss.str();
}

std::string getURL(HttpRequest& request)
{
	if (request.path.find(request.location.path) == (size_t)-1)
		THROW("path not found in request wtf");

	int idx = request.path.find(request.location.path) + request.location.path.size();
	std::string tmp = request.path.substr(idx);
	std::string final = request.location.root + tmp;
	return (final);
}

std::string getDirectoryListing(HttpRequest& request)
{
	std::stringstream ss;
	std::vector<File> files = getFilesInDir(request.path);

	// std::cout << request << std::endl;

	ss << "<html>";
	ss << "<head>";
	ss << "	<style>";
	ss << "	.elt {display:flex; flex-direction:row; gap: 65px; align-items: center; justify-content: space-between; width: 33svw;}";
	ss << "	body {display:flex; flex-direction:column;}";
	ss << "	</style>";
	ss << "</head>";
	ss << "<body>";
	ss << "	<h1>" << request.path << "</h1>";
	ss << "	<div class='elt'><p>name </p> <p> | </p> <p>size</p></div>";
	for (size_t i = 0; i < files.size(); i++)
	{
		ss << getElt(files[i], getURL(request));
	}
	ss << "</body>";
	ss << "</html>";

	return (generatePage(200, ss.str()));
}

const std::string	Webserv::handleGetRequest(HttpRequest& request, const Server& server) const
{
	std::string	path = request.path;
	if (path.find(".") == (size_t)-1)
	{
		if (Utils::dirAccess(request.path))
		{
			Log(Log::SUCCESS) << "trying to get to dir listing" << Log::endl();
			if (request.location.directoryListing)
				return getDirectoryListing(request);
			Log(Log::WARNING) << "directory listing is off:'" << path << "'" << Log::endl();
			return (getErrorPage(403, server));
		}
		Log(Log::WARNING) << "directory not found:'" << path << "'" << Log::endl();
		return (getErrorPage(404, server));
	}

	struct stat	statbuf;
	if (stat(path.c_str(), &statbuf) != 0)
	{
		Log(Log::WARNING) << "File not found:'" << path << "'" << Log::endl();
		return (getErrorPage(404, server));
	}

	Log(Log::DEBUG) << "File found:" << path << Log::endl();

	std::ifstream	file(path.c_str(), std::ios::binary);
	if (!file)
	{
		Log(Log::WARNING) << "Cannot open " << path << Log::endl();
		return (getErrorPage(403, server));
	}

	std::string	content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
	std::string	page = generatePage(200, content);
	size_t		header_end = page.find("\r\n\r\n");
	if (header_end != std::string::npos)
	{
		page.insert(header_end, "\r\n" + server.getCookies());
	}
	return (page);
}

const std::string	Webserv::handlePostRequest(const HttpRequest& request, const Server& server) const
{
	if (request.body.size() > server.client_max_body_size && server.client_max_body_size > 0) //! TODO le truc empeche de faire de request post avec le server
	{
		Log(Log::WARNING) << "Body size exceeds client_max_body_size" << Log::endl();
		return (getErrorPage(413, server));
	}
	Log(Log::WARNING) << "Upload directory is not set for POST request" << Log::endl();
	Log(Log::SUCCESS) << "Post request answered !" << Log::endl();
	return (generatePage(200, server.handlePostRequest(request)));

}

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
		return (generatePage(200, "File deleted successfully\n"));
	}
	else
	{
		Log(Log::ERROR) << "Cannot delete " << path << Log::endl();
		return (getErrorPage(403, server));
	}
}

uint32_t	strToAddr(const std::string& str)
{
	std::vector<std::string> split = Utils::strsplit(str, '.');
	if (split.size() != 4)
	{
		Log(Log::ERROR) << "ip addr is in wrong format, abording strToAddr" << Log::endl();
		return (0);
	}

	uint8_t	a, b, c, d;

	a = std::atoi(split[0].c_str());
	b = std::atoi(split[1].c_str());
	c = std::atoi(split[2].c_str());
	d = std::atoi(split[3].c_str());

	return ((a << 24) | (b << 16) | (c << 8) | d);
}

void Webserv::run()
{
	Log() << "Running web server..." << Log::endl();

	_epoll_fd = epoll_create(EPOLL_CLOEXEC);
	if (_epoll_fd == -1)
	{
		THROW("Failed to create epoll instance: ");
	}

	for (size_t i = 0; i < _servers.size(); ++i)
	{
		Log(Log::DEBUG) << "Initializing server" << i << Log::endl();

		for (std::vector<Listen>::const_iterator	it = _servers[i].listen.begin();
			it != _servers[i].listen.end(); ++it)
		{
			int	listener_fd = socket(AF_INET, SOCK_STREAM, 0);
			if (listener_fd < 0)
			{
				THROW("Failed to create socket: ");
			}

			_listener_fds.push_back(listener_fd);

			int	opt = 1;
			if (setsockopt(listener_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) == -1)
			{
				THROW("setsockopt adress failed: ");
			}

			struct sockaddr_in	server_addr;
			server_addr.sin_family = AF_INET;

			struct addrinfo		hints;
			std::memset(&hints, 0, sizeof(hints));
			hints.ai_family = AF_INET;
			hints.ai_flags = AI_NUMERICHOST;

			struct addrinfo*	res = NULL;
			int	ret = getaddrinfo(it->addr.c_str(), NULL, &hints, &res);
			if (ret != 0)
			{
				throw std::runtime_error("getaddrinfo failed" + static_cast<std::string>(gai_strerror(ret)));
			}

			if (res)
			{
				uint32_t	addr = strToAddr(it->addr);
				server_addr.sin_addr.s_addr = addr;
				freeaddrinfo(res);
			}
			else
			{
				throw std::runtime_error("getaddrinfo returned no results: " + static_cast<std::string>(gai_strerror(ret)));
			}

			server_addr.sin_port = htons(it->port);

			Log(Log::DEBUG) << "Server listening on port " << it->port << Log::endl();

			if (bind(listener_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1)
			{
				THROW("bind failed: ");
			}

			if (listen(listener_fd, SOMAXCONN) == -1)
			{
				THROW("listen failed: ");
			}

			struct epoll_event	ev;
			ev.events = EPOLLIN;
			ev.data.fd = listener_fd;
			if (epoll_ctl(_epoll_fd, EPOLL_CTL_ADD, listener_fd, &ev) == -1)
			{
				THROW("epoll_ctl failed: ");
			}
		}
	}

	struct epoll_event	events[MAX_EVENTS];

	while (true)
	{
		int	nfds = epoll_wait(_epoll_fd, events, MAX_EVENTS, 100);
		if (nfds == -1)
		{
			ERROR("epoll_wait failed: ");
			continue ;
		}

		for (int n = 0; n < nfds; ++n)
		{
			int	fd = events[n].data.fd;
			std::vector<int>::iterator	it = std::find(_listener_fds.begin(), _listener_fds.end(), fd);

			if (it != _listener_fds.end())
			{
				int	client = accept(fd, NULL, NULL);
				if (client < 0)
				{
					ERROR("accept failed:");
					continue ;
				}

				if (fcntl(client, F_SETFL, O_NONBLOCK | FD_CLOEXEC) == -1)
				{
					CLOSE(client);
					continue ;
				}

				struct epoll_event	client_ev;
				client_ev.events = EPOLLIN | EPOLLRDHUP;
				client_ev.data.fd = client;
				if (epoll_ctl(_epoll_fd, EPOLL_CTL_ADD, client, &client_ev) == -1)
				{
					ERROR("epoll_ctl for client failed:");
					CLOSE(client);
				}

				continue ;
			}
			else if (events[n].events & EPOLLRDHUP)
			{
				epoll_ctl(_epoll_fd, EPOLL_CTL_DEL, fd, NULL);
				CLOSE(fd);
			}
			else if (events[n].events & EPOLLIN)
			{
				char		buffer[4096];
				ssize_t		bytes_read;
				std::string	request;

				while ((bytes_read = recv(fd, buffer, sizeof(buffer), 0)) > 0)
				{
					request.append(buffer, bytes_read);
				}

				struct sockaddr_in	addr;
				socklen_t			addr_len = sizeof(addr);
				getsockname(fd, (struct sockaddr*)&addr, &addr_len);
				uint16_t			port = ntohs(addr.sin_port);

				HttpRequest	httpReq = parseRequest(request, _servers[0]);
				std::string	host_header;

				std::map<std::string, std::string>::const_iterator	host_it = httpReq.headers.find("Host");
				if (host_it != httpReq.headers.end())
				{
					host_header = host_it->second;
					size_t	colon_pos = host_header.find(':');
					if (colon_pos != std::string::npos)
					{
						host_header = host_header.substr(0, colon_pos);
					}
				}

				Log(Log::DEBUG) << "Host header:" << host_header << Log::endl();

				Server*	server = NULL;
				for (std::vector<Server>::iterator	s_it = _servers.begin(); s_it != _servers.end(); ++s_it)
				{
					Server&	s = *s_it;
					for (std::vector<Listen>::const_iterator	it = s.listen.begin(); it != s.listen.end(); ++it)
					{
						if (it->port == port && (host_header.empty() || s.server_names[0] == host_header))
						{
							server = &s;
							server->lastUID = addr.sin_addr.s_addr;
							break ;
						}
					}
					if (server)
					{
						break ;
					}
				}

				if (!server)
				{
					for (std::vector<Server>::iterator s_it = _servers.begin(); s_it != _servers.end(); ++s_it)
					{
						Server&	s = *s_it;
						for (std::vector<Listen>::const_iterator it = s.listen.begin(); it != s.listen.end(); ++it)
						{
							if (it->port == port)
							{
								server = &s;
								server->lastUID = addr.sin_addr.s_addr;
								break ;
							}
						}
						if (server)
						{
							break ;
						}
					}
				}

				httpReq = parseRequest(request, *server);
				std::string	response = "";
				CgiHandler	cgi(httpReq.method, "text/html", ""); //Need ContentType and ContentLength (if apply)

				Server::registerSession(addr.sin_addr.s_addr);

				if (!httpReq.location.redirection.second.empty())
				{
					response = getUrlPage(httpReq.location.redirection.first,
						"<html><body><h1>Redirecting...</h1></body></html>", httpReq.location.redirection.second);
				}
				else if (!httpReq.method_allowed)
				{
					response = getErrorPage(405, *server);
				}
				else if (cgi.cgiRequest(httpReq, this->_servers.data()->locations))
				{
					// if (contentLength != 0) cgi.addBody();
					Log(Log::LOG) << "launching cgi" << Log::endl();
					response = cgi.launch();
				}
				else if (httpReq.method == "GET")
				{
					response = handleGetRequest(httpReq, *server);
				}
				else if (httpReq.method == "POST")
				{
					response = handlePostRequest(httpReq, *server);
				}
				else if (httpReq.method == "DELETE")
				{
					response = handleDeleteRequest(httpReq.path, *server);
				}

				if (send(fd, response.c_str(), response.size(), 0) == -1)
				{
					ERROR("send error:");
				}

				epoll_ctl(_epoll_fd, EPOLL_CTL_DEL, fd, NULL);
				CLOSE(fd);
			}
		}
	}
}

std::ostream& operator<<(std::ostream& stream, HttpRequest& request)
{
	std::map<std::string, std::string>::iterator it = request.headers.begin();
	for (; it != request.headers.end(); ++it)
	{
		stream << it->first << " | " << it->second << std::endl;
	}

	return stream;
}

