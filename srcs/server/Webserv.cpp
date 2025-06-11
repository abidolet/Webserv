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

#define MAX_EVENTS 1024
#define CLOSE(fd) if (fd > 1) close(fd)

Webserv::Webserv(const std::string& file)
	: _servers(), _epoll_fd(-1), _listener_fd(-1)
{
	Parser	parser(file);
	_servers.push_back(parser.populateServerInfos());
}

Webserv::~Webserv()
{
	CLOSE(_epoll_fd);
	CLOSE(_listener_fd);
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

std::string	handleGetRequest(std::string& path)
{
	Log() << "Get request for: " << path << Log::endl();

	if (path.empty() || path == "/")
	{
		path = "www/index.html";
	}
	if (path[0] == '/')
	{
		path = path.substr(1);
	}
	if (!path.empty() && path[path.size() - 1] == '/')
	{
		path += "www/index.html";
	}

	Log() << "Trying to access to: " << path << Log::endl();

	struct stat	statbuf;
	if (stat(path.c_str(), &statbuf) != 0)
	{
		Log(Log::ERROR) << "File not found:" << path << Log::endl();
		return ("HTTP/1.1 404 Not Found\r\nContent-Type: text/html\r\nContent-Length: 0\r\n\r\n");
	}

	if (S_ISDIR(statbuf.st_mode))
	{
		path += "/www/index.html";
		if (stat(path.c_str(), &statbuf) != 0)
		{
			Log(Log::ERROR) << "Directory index not found:" << path << Log::endl();
			return ("HTTP/1.1 403 Forbidden\r\nContent-Type: text/html\r\nContent-Length: 0\r\n\r\n");
		}
	}

	std::ifstream	file(path.c_str(), std::ios::binary);
	if (file.good())
	{
		std::string	content;
		char		buffer[4096];
		while (file.read(buffer, sizeof(buffer)))
		{
			content.append(buffer, file.gcount());
		}
		content.append(buffer, file.gcount());

		std::ostringstream	oss;
		oss << content.size();
		std::string	content_length = oss.str();

		std::string	response = "HTTP/1.1 200 OK\r\n";
		response += "Content-Type: text/html\r\n";
		response += "Content-Length: " + content_length + "\r\n";
		response += "\r\n" + content;

		Log(Log::SUCCESS) << "File served successfully: " << path << Log::endl();

		return (response);
	}
	else
	{
		Log(Log::ERROR) << "Failed to open file: " << path << Log::endl();
		return ("HTTP/1.1 404 Not Found\r\nContent-Type: text/html\r\nContent-Length: 0\r\n\r\n");
	}
}

std::string	handlePostRequest(std::string body)
{
	std::ostringstream	oss;
	oss << body.size();
	std::string	content_length = oss.str();

	Log() << "Post request with body: " << body << Log::endl();

	std::string	response = "HTTP/1.1 200 OK\r\n";
	response += "Content-Type: text/plain\r\n";
	response += "Content-Length: " + content_length + "\r\n\r\n";
	response += body;

	Log(Log::SUCCESS) << "Post request answered !" << Log::endl();

	return (response);
}

std::string	handleDeleteRequest(const std::string& request)
{
	std::string	path = "www" + request;

	Log() << "Delete request for: " << path << Log::endl();

	if (remove(path.c_str()) == 0)
	{
		Log(Log::SUCCESS) << "File deleted !" << path << Log::endl();
		return ("HTTP/1.1 200 OK\r\n\r\nFile deleted successfully :)\r\n");
	}
	else
	{
		Log(Log::ERROR) << "Failed to delete " << path << Log::endl();
		return ("HTTP/1.1 403 Forbidden\r\n"
			"Content-Type: text/plain\r\n"
			"Content-Length: 25\r\n\r\n"
			"Could not delete file :(\n");
	}
}

void Webserv::run()
{
	Log() << "Running web server..." << Log::endl();

	_epoll_fd = epoll_create(10);
	if (_epoll_fd == -1)
	{
		Log(Log::ALERT) << "Failed to create epoll instance" << Log::endl();
		return ;
	}

	_listener_fd = socket(AF_INET, SOCK_STREAM, 0);
	if (_listener_fd < 0)
	{
		Log(Log::ALERT) << "Failed to create socket" << Log::endl();
		return ;
	}

	int	opt = 1;
	if (setsockopt(_listener_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) == -1)
	{
		Log(Log::ALERT) << "setsockopt failed" << Log::endl();
		return ;
	}

	struct sockaddr_in	server_addr;
	server_addr.sin_family = AF_INET;
	server_addr.sin_addr.s_addr = INADDR_ANY;
	server_addr.sin_port = htons(_servers[0].listen.begin()->second);

	if (bind(_listener_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1)
	{
		perror("bind");
		Log(Log::ALERT) << "bind failed" << Log::endl();
		CLOSE(_listener_fd);
		return ;
	}

	if (listen(_listener_fd, SOMAXCONN) == -1)
	{
		Log(Log::ALERT) << "listen failed" << Log::endl();
		return ;
	}

	struct epoll_event	ev;
	ev.events = EPOLLIN | EPOLLET;
	ev.data.fd = _listener_fd;
	if (epoll_ctl(_epoll_fd, EPOLL_CTL_ADD, _listener_fd, &ev) == -1)
	{
		Log(Log::ALERT) << "epoll_ctl failed" << Log::endl();
		return ;
	}

	struct epoll_event	events[MAX_EVENTS];
	int					nfds;

	while (true)
	{
		nfds = epoll_wait(_epoll_fd, events, MAX_EVENTS, 1000);
		if (nfds == -1)
		{
			Log(Log::ERROR) << "epoll_wait failed" << Log::endl();
			continue ;
		}

		for (int n = 0; n < nfds; ++n)
		{
			if (events[n].data.fd == _listener_fd)
			{
				int	client = accept(_listener_fd, NULL, NULL);
				if (client < 0)
				{
					Log(Log::ERROR) << "accept failed" << Log::endl();
					continue ;
				}

				if (fcntl(client, F_SETFL, O_NONBLOCK | FD_CLOEXEC) == -1)
				{
					CLOSE(client);
					continue ;
				}

				ev.events = EPOLLIN | EPOLLET | EPOLLRDHUP;
				ev.data.fd = client;
				if (epoll_ctl(_epoll_fd, EPOLL_CTL_ADD, client, &ev) == -1)
				{
					Log(Log::ERROR) << "epoll_ctl for client failed" << Log::endl();
					CLOSE(client);
				}
			}
			else
			{
				if (events[n].events & EPOLLRDHUP)
				{
					epoll_ctl(_epoll_fd, EPOLL_CTL_DEL, events[n].data.fd, NULL);
					CLOSE(events[n].data.fd);
				}
				else if (events[n].events & EPOLLIN)
				{
					char		buffer[4096];
					ssize_t		bytes_read;
					std::string	request;

					while ((bytes_read = recv(events[n].data.fd, buffer, sizeof(buffer), 0)) > 0)
					{
						request.append(buffer, bytes_read);
					}

					HttpRequest	httpReq = parseRequest(request);
					std::string	response;

					if (httpReq.method == "GET")
					{
						response = handleGetRequest(httpReq.path);
					}
					else if (httpReq.method == "POST")
					{
						response = handlePostRequest(httpReq.body);
					}
					else if (httpReq.method == "DELETE")
					{
						response = handleDeleteRequest(httpReq.path);
					}

					ssize_t	bytes_sent = send(events[n].data.fd, response.c_str(), response.size(), 0);
					if (bytes_sent == -1)
					{
						Log(Log::ERROR) << "send error" << Log::endl();
					}

					epoll_ctl(_epoll_fd, EPOLL_CTL_DEL, events[n].data.fd, NULL);
					CLOSE(events[n].data.fd);
				}
			}
		}
	}
}
