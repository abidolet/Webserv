#include "Webserv.hpp"
#include "Parser.hpp"
#include "Log.hpp"
#include <sys/epoll.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <fcntl.h>
#include <cstring>
#include <sstream>
#include <string>
#include <sys/stat.h>
#include <dirent.h>
#include <stdlib.h>
#include <algorithm>

#define MAX_EVENTS 1024
#define CLOSE(fd) if (fd > 1) {close(fd); fd = -1;}

Webserv::Webserv(const std::string& file)
	: _servers(), _epoll_fd(-1), _listener_fds()
{
	Parser	parser(file);
	_servers = parser.populateServerInfos();
	cookies = parser.getCookies(_servers[0]);
}

Webserv::~Webserv()
{
	CLOSE(_epoll_fd);
	for (unsigned int i = 0; i < _listener_fds.size(); ++i)
	{
		CLOSE(_listener_fds[i]);
	}
}

std::string	toString(int value)
{
	std::ostringstream oss;
	oss << value;
	return (oss.str());
}

std::string Webserv::getErrorPage(int error_code) const
{
	const Server&	server = _servers[0];
	std::map<int, std::string>::const_iterator it = server.error_pages.find(error_code);

	if (it != server.error_pages.end())
	{
		Log(Log::DEBUG) << "Searching custom error page for code" << error_code << ":" << it->second << Log::endl();

		std::string		error_path = it->second;
		std::ifstream	file(error_path.c_str(), std::ios::binary);

		if (file)
		{
			Log(Log::DEBUG) << "Custom error page found:" << error_path << Log::endl();

			std::string			content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
			std::ostringstream	oss;
			oss << content.size();

			return ("HTTP/1.1 " + toString(error_code) + " " + getStatusMessage(error_code) + "\r\n" +
				"Content-Type: text/html\r\n" +
				"Content-Length: " + oss.str() + "\r\n" +
				"Connection: close\r\n\r\n" +
				content);
		}
		else
		{
			Log(Log::DEBUG) << "Custom error page not found:" << error_path << Log::endl();
		}
	}

	std::ostringstream	oss;
	oss << error_code;
	std::string	default_content = "<html><body><h1>" + oss.str() + " " +
								getStatusMessage(error_code) + "</h1></body></html>";

	oss.str("");
	oss << default_content.size();

	return ("HTTP/1.1 " + toString(error_code) + " " + getStatusMessage(error_code) + "\r\n" +
		"Content-Type: text/html\r\n" +
		"Content-Length: " + oss.str() + "\r\nConnection: close\r\n\r\n" +
		default_content);
}

std::string Webserv::getStatusMessage(int code) const
{
	switch(code)
	{
		case 403:	return ("Forbidden");
		case 404:	return ("Not Found");
		case 405:	return ("Method Not Allowed");
		default:	return ("Error");
	}
}

HttpRequest	parseRequest(const std::string& rawRequest)
{
	HttpRequest			request;
	std::istringstream	stream(rawRequest);
	std::string			line;

	if (std::getline(stream, line))
	{
		std::istringstream	request_line(line);
		request_line >> request.method >> request.path;
	}

	size_t		pos;
	std::string	key;
	std::string	value;
	while (std::getline(stream, line) && line != "\r")
	{
		pos = line.find(':');
		if (pos != std::string::npos)
		{
			size_t	start = value.find_first_not_of(" \t\r\n");
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
	}
	return (request);
}

std::string	Webserv::handleGetRequest(const Server&	server, std::string& path) const
{
	Log(Log::DEBUG) << "Get request for:" << path << Log::endl();

	while (path != "/" && path[path.size() - 1] == '/')
	{
		path.erase(path.size() - 1);
	}

	Log(Log::DEBUG) << "Get request cleaned path:" << path << Log::endl();

	std::string::size_type	pos = path.find_last_of("/");
	std::string	to_find = path.substr(0, pos);
	std::string	filename = path.substr(pos + 1);
	if (pos == 0)
	{
		to_find = path;
		filename = "";
	}
	Log(Log::DEBUG) << "Get request to_find:" << to_find << Log::endl();
	Log(Log::DEBUG) << "Get request filename:" << filename << Log::endl();

	Log(Log::DEBUG) << "Searching for location with path:" << to_find << Log::endl();

	for (size_t	i = 0; i < server.locations.size(); ++i)
	{
		Log(Log::DEBUG) << "Checking location:" << server.locations[i].root << Log::endl();
		if (server.locations[i].root == to_find)
		{
			Log(Log::DEBUG) << "Location found for path:" << to_find << Log::endl();
			path = server.locations[i].path + '/' + filename;
			DIR*	dir = opendir(path.c_str());
			if (dir)
			{
				Log(Log::DEBUG) << "Directory opened successfully:" << path << Log::endl();
				closedir(dir);
				path += '/' + server.locations[i].index;
			}
			break ;
		}
		else if (i == server.locations.size() - 1)
		{
			Log(Log::DEBUG) << "No location found for path, adding root" << Log::endl();
			path = server.root + path;
			DIR*	dir = opendir(path.c_str());
			if (dir)
			{
				Log(Log::DEBUG) << "Directory opened successfully:" << path << Log::endl();
				closedir(dir);
				return (getErrorPage(403));
			}
		}
	}

	std::string	full_path = path;

	Log(Log::DEBUG) << "Get request final:" << full_path << Log::endl();

	struct stat	statbuf;
	if (stat(full_path.c_str(), &statbuf) != 0)
	{
		Log(Log::WARNING) << "File not found:'" << full_path << "'" << Log::endl();
		return (getErrorPage(404));
	}

	Log(Log::DEBUG) << "File found:'" << full_path << "'" << Log::endl();

	std::ifstream	file(full_path.c_str(), std::ios::binary);
	if (!file)
	{
		Log(Log::WARNING) << "Cannot open " << full_path << Log::endl();
		return (getErrorPage(403));
	}

	std::string			content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
	std::ostringstream	oss;
	oss << content.size();

	std::string	response = "HTTP/1.1 200 OK\r\n";
	response += "Content-Type: text/html\r\n";
	response += "Content-Length: " + oss.str() + "Connection: close\r\n" + cookies + "\r\n\r\n";
	response += content;

	return (response);
}

std::string	Webserv::handlePostRequest(const std::string& body) const
{
	std::ostringstream	oss;
	oss << body.size();
	std::string	content_length = oss.str();

	Log() << "Post request with body: " << body << Log::endl();

	std::string	response = "HTTP/1.1 200 OK\r\n";
	response += "Content-Type: text/plain\r\n";
	response += "Content-Length: " + content_length + "Connection: close\r\n\r\n";
	response += body;

	Log(Log::SUCCESS) << "Post request answered !" << Log::endl();

	return (response);
}

std::string	Webserv::handleDeleteRequest(const std::string& request) const
{
	std::string	path = "www" + request;

	Log() << "Delete request for: " << path << Log::endl();

	struct stat	statbuf;
	if (stat(path.c_str(), &statbuf) != 0)
	{
		Log(Log::ERROR) << "File not found: '" << path << "'" << Log::endl();
		return (getErrorPage(404));
	}

	if (remove(path.c_str()) == 0)
	{
		Log(Log::SUCCESS) << "File deleted !" << path << Log::endl();
		return ("HTTP/1.1 200 OK\r\n\rConnection: close\r\n\r\nFile deleted successfully :)\r\n");
	}
	else
	{
		Log(Log::ERROR) << "Cannot delete " << path << Log::endl();
		return (getErrorPage(403));
	}
}

void Webserv::run()
{
	Log() << "Running web server..." << Log::endl();

	_epoll_fd = epoll_create(10);
	if (_epoll_fd == -1)
	{
		throw std::runtime_error("Failed to create epoll instance: " + static_cast<std::string>(strerror(errno)));
	}

	Log(Log::DEBUG) << "Webserv started on" << _servers.size() << "server(s)" << Log::endl();

	for (size_t i = 0; i < _servers.size(); ++i)
	{
		Log(Log::DEBUG) << "Initializing server" << i << Log::endl();
		int	listener_fd = socket(AF_INET, SOCK_STREAM, 0);
		if (listener_fd < 0)
		{
			throw std::runtime_error("Failed to create socket: " + static_cast<std::string>(strerror(errno)));
		}

		int	opt = 1;
		if (setsockopt(listener_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) == -1)
		{
			throw std::runtime_error("setsockopt failed: " + static_cast<std::string>(strerror(errno)));
		}
		if (setsockopt(listener_fd, SOL_SOCKET, SO_REUSEPORT, &opt, sizeof(opt)) == -1)
		{
			throw std::runtime_error("setsockopt failed:" + static_cast<std::string>(strerror(errno)));
		}

		if (fcntl(listener_fd, F_SETFL, O_NONBLOCK | FD_CLOEXEC) == -1)
		{
			throw std::runtime_error("fcntl failed: " + static_cast<std::string>(strerror(errno)));
		}

		struct sockaddr_in	server_addr;
		server_addr.sin_family = AF_INET;
		server_addr.sin_addr.s_addr = INADDR_ANY;
		server_addr.sin_port = htons(_servers[i].listen.begin()->second);

		if (bind(listener_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1)
		{
			throw std::runtime_error("bind failed: " + static_cast<std::string>(strerror(errno)));
		}

		if (listen(listener_fd, 4096) == -1)
		{
			throw std::runtime_error("listen failed: " + static_cast<std::string>(strerror(errno)));
		}

		struct epoll_event	ev;
		ev.events = EPOLLIN | EPOLLET;
		ev.data.fd = listener_fd;

		// struct timeval tv;
		// tv.tv_sec = 5;
		// tv.tv_usec = 0;
		// setsockopt(listener_fd, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof(tv));

		if (epoll_ctl(_epoll_fd, EPOLL_CTL_ADD, listener_fd, &ev) == -1)
		{
			throw std::runtime_error("epoll_ctl failed: " + static_cast<std::string>(strerror(errno)));
		}

		_listener_fds.push_back(listener_fd);
	}

	struct epoll_event	events[MAX_EVENTS];

	while (true)
	{
		int	nfds = epoll_wait(_epoll_fd, events, MAX_EVENTS, 100);
		if (nfds == -1)
		{
			Log(Log::ERROR) << "epoll_wait failed: " << strerror(errno) << Log::endl();
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
					Log(Log::ERROR) << "accept failed:" << strerror(errno) << Log::endl();
					continue ;
				}

				if (fcntl(client, F_SETFL, O_NONBLOCK | FD_CLOEXEC) == -1)
				{
					CLOSE(client);
					continue ;
				}

				struct epoll_event	client_ev;
				client_ev.events = EPOLLIN | EPOLLET | EPOLLRDHUP;
				client_ev.data.fd = client;
				if (epoll_ctl(_epoll_fd, EPOLL_CTL_ADD, client, &client_ev) == -1)
				{
					Log(Log::ERROR) << "epoll_ctl for client failed:" << strerror(errno) << Log::endl();
					CLOSE(client);
				}
				continue ;
			}
			else if (events[n].events & (EPOLLRDHUP | EPOLLHUP))
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

				HttpRequest	httpReq = parseRequest(request);
				CgiHandler	cgi(httpReq.method, "", ""); //Need ContentType and ContentLength (if apply)
				std::string	response;

				struct sockaddr_in	addr;
				socklen_t			addr_len = sizeof(addr);
				getsockname(fd, (struct sockaddr*)&addr, &addr_len);
				uint16_t			port = ntohs(addr.sin_port);

				Server*	server = NULL;
				for (std::vector<Server>::iterator	s_it = _servers.begin(); s_it != _servers.end(); ++s_it)
				{
					Server& s = *s_it;
					if (s.listen.begin()->second == port)
					{
						server = &s;
						break ;
					}
				}

				if (!server)
				{
					response = getErrorPage(500);
				}
				else if (cgi.cgiRequest(httpReq, this->_servers.data()->locations))
				{
					// if (contentLength != 0) cgi.addBody();
					Log(Log::LOG) << "launching cgi" << Log::endl();
					response = cgi.launch();
				}
				else if (httpReq.method == "GET")
				{
					// if (!getAllowed)
					// {
					// 	Log(Log::ERROR) << "Method GET not allowed" << Log::endl();
					// 	response = getErrorPage(405);
					// }
					// else
					// {
						response = handleGetRequest(*server, httpReq.path);
					// }
				}
				else if (httpReq.method == "POST")
				{
					// if (!postAllowed)
					// {
						// Log(Log::ERROR) << "Method POST not allowed" << Log::endl();
						// response = getErrorPage(405);
					// }
					// else
					// {
						response = handlePostRequest(httpReq.body);
					// }
				}
				else if (httpReq.method == "DELETE")
				{
					// if (!deleteAllowed)
					// {
						// Log(Log::ERROR) << "Method DELETE not allowed" << Log::endl();
						// response = getErrorPage(405);
					// }
					// else
					// {
						response = handleDeleteRequest(httpReq.path);
					// }
				}

				if (send(fd, response.c_str(), response.size(), 0) == -1)
				{
					Log(Log::ERROR) << "send error:" << strerror(errno) << Log::endl();
				}

				epoll_ctl(_epoll_fd, EPOLL_CTL_DEL, fd, NULL);
				CLOSE(fd);
			}
		}
	}
}
