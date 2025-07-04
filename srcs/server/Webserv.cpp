#include "Webserv.hpp"
#include "Parser.hpp"
#include "ParserUtils.hpp"
#include "CgiHandler.hpp"

#include <sys/epoll.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <dirent.h>
#include <netdb.h>
#include <ctime>

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
		}
	}

	size_t	body_pos = rawRequest.find("\r\n\r\n");
	if (body_pos != std::string::npos)
	{
		request.body = rawRequest.substr(body_pos + 4);
		Log(Log::DEBUG) << "Body size:" << request.body.size() << "bytes" << Log::endl();
	}

	Log(Log::DEBUG) << "Checking location for:" << request.path << Log::endl();
	const Location*	best_match = getLocation(request.path, server);
	request.method_allowed = false;

	request.server = server;
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
		full_path += '/' + Utils::strtrim(request.path, "/");
		Log(Log::DEBUG) << "Full path constructed:" << full_path << Log::endl();

		Log(Log::DEBUG) << "Trimming" << full_path << Log::endl();
		full_path = '/' + Utils::strtrim(full_path, "/");
		Log(Log::DEBUG) << "Checking file stats for:" << full_path << Log::endl();
		struct stat	statbuf;
		if (stat(full_path.c_str(), &statbuf) == 0 && S_ISDIR(statbuf.st_mode))
		{
			std::string	index = best_match->index;
			Log(Log::DEBUG) << "Path is a directory adding index:" << index << Log::endl();
			full_path += '/' + best_match->index;
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

bool isTTY(const char* name)
{

	struct stat st;
	if (stat(name, &st) == -1)
	{
		ERROR("cannot check is tty");
		return false;
	}
	return S_ISCHR(st.st_mode);
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

void	Webserv::init_servers()
{
	for (size_t	i = 0; i < _servers.size(); ++i)
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

			struct addrinfo	hints;
			std::memset(&hints, 0, sizeof(hints));
			hints.ai_family = AF_INET;
			hints.ai_flags = AI_NUMERICHOST;

			struct addrinfo*	res = NULL;
			int	ret = getaddrinfo(it->addr.c_str(), NULL, &hints, &res);
			if (ret != 0)
			{
				THROW_GAI("getaddrinfo failed: ", ret);
			}

			if (res)
			{
				uint32_t	addr = strToAddr(it->addr);
				server_addr.sin_addr.s_addr = addr;
				freeaddrinfo(res);
			}
			else
			{
				THROW_GAI("getaddrinfo returned no results: ", ret);
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
}

void	Webserv::run()
{
	Log() << "Running web server..." << Log::endl();

	_epoll_fd = epoll_create(EPOLL_CLOEXEC);
	if (_epoll_fd == -1)
	{
		THROW("Failed to create epoll instance: ");
	}

	init_servers();

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
						for (std::vector<std::string>::const_iterator name_it = s.server_names.begin();
							name_it != s.server_names.end(); ++name_it)
						{
							if (it->port == port && (host_header.empty() || *name_it == host_header))
							{
								server = &s;
								server->lastUID = addr.sin_addr.s_addr;
								break ;
							}
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
				CgiHandler	cgi(httpReq.method, httpReq.headers["Content-Type"], httpReq.headers["Content-Length"], *server);
				Server::registerSession(addr.sin_addr.s_addr);

				if (!httpReq.location.redirection.second.empty())
				{
					response = getUrlPage(httpReq.location.redirection.first, httpReq.location.redirection.second);
				}
				else if (!httpReq.method_allowed)
				{
					response = getErrorPage(405, *server);
				}
				else if (cgi.cgiRequest(httpReq, server->locations))
				{
					if (!httpReq.headers["Content-Length"].empty())
					{
						cgi.sendFd(fd);
					}

					Log(Log::DEBUG) << "launching cgi" << Log::endl();
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
				else if (httpReq.method == "OPTIONS")
				{
					response = getErrorPage(200, *server);
				}
				else
				{
					response = getErrorPage(501, *server);
				}

				ssize_t	bytes_send = write(fd, response.c_str(), response.size());
				if (bytes_send < 0)
				{
					ERROR("write error:");
				}
				else if (bytes_send < static_cast<ssize_t>(response.size()))
				{
					Log(Log::WARNING) << "Function write did't return enough bytes:" << static_cast<ssize_t>(response.size()) - bytes_send << "bytes haven't been send" << Log::endl();
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

	return (stream);
}

