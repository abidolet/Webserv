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

#define MAX_EVENTS 1024

Webserv::Webserv()
	: _servers(), _epoll_fd(-1), _listener_fd(-1)
{
	Server	default_server;

	default_server.port = 8080;
	default_server.host = "0.0.0.0";
	default_server.server_name = "nginx";
	default_server.is_default = true;
	_servers.push_back(default_server);
}

Webserv::Webserv(const std::string& file)
	: _servers(), _epoll_fd(-1), _listener_fd(-1)
{
	Parser parser(file);
	(void)file;
}

Webserv::~Webserv()
{
	if (_epoll_fd != -1)
	{
		close(_epoll_fd);
	}
	if (_listener_fd != -1)
	{
		close(_listener_fd);
	}
}

int setnonblocking(int sock)
{
	int	flags = fcntl(sock, F_GETFL, 0);
	if (flags == -1)
	{
		return (-1);
	}

	return (fcntl(sock , F_SETFL , flags | O_NONBLOCK));
}

void Webserv::run()
{
	Log() << "Running web server..." << Log::endl();

	_epoll_fd = epoll_create1(0);
	if (_epoll_fd == -1)
	{
		Log(Log::ERROR) << "Failed to create epoll instance" << Log::endl();
		return ;
	}

	_listener_fd = socket(AF_INET, SOCK_STREAM, 0);
	if (_listener_fd < 0)
	{
		Log(Log::ERROR) << "Failed to create socket" << Log::endl();
		return ;
	}

	int opt = 1;
	if (setsockopt(_listener_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) == -1)
	{
		Log(Log::ERROR) << "setsockopt failed" << Log::endl();
		return ;
	}

	struct	sockaddr_in server_addr;
	memset(&server_addr, 0, sizeof(server_addr));
	server_addr.sin_family = AF_INET;
	server_addr.sin_addr.s_addr = INADDR_ANY;
	server_addr.sin_port = htons(_servers[0].port);

	if (bind(_listener_fd, (struct	sockaddr *)&server_addr, sizeof(server_addr)) == -1)
	{
		Log(Log::ERROR) << "bind failed" << Log::endl();
		return;
	}

	if (setnonblocking(_listener_fd) == -1)
	{
		Log(Log::ERROR) << "setnonblocking failed" << Log::endl();
		return;
	}

	if (listen(_listener_fd, SOMAXCONN) == -1)
	{
		Log(Log::ERROR) << "listen failed" << Log::endl();
		return;
	}

	struct	epoll_event ev;
	ev.events = EPOLLIN | EPOLLET;
	ev.data.fd = _listener_fd;
	if (epoll_ctl(_epoll_fd, EPOLL_CTL_ADD, _listener_fd, &ev) == -1)
	{
		Log(Log::ERROR) << "epoll_ctl failed" << Log::endl();
		return;
	}

	struct		epoll_event events[MAX_EVENTS];
	const int	maxevents = MAX_EVENTS;
	int			nfds;

	while (true)
	{
		nfds = epoll_wait(_epoll_fd, events, maxevents, -1);
		if (nfds == -1)
		{
			if (errno == EINTR)
			{
				continue ;
			}
			Log(Log::ERROR) << "epoll_wait failed" << Log::endl();
			break ;
		}

		for (int n = 0; n < nfds; ++n)
		{
			if (events[n].data.fd == _listener_fd)
			{
				struct	sockaddr_in local;
				socklen_t addrlen = sizeof(local);
				int client = accept(_listener_fd, (struct	sockaddr *)&local, &addrlen);
				if (client < 0)
				{
					if (errno != EAGAIN && errno != EWOULDBLOCK)
					{
						Log(Log::ERROR) << "accept failed" << Log::endl();
					}
					continue ;
				}

				if (setnonblocking(client) == -1)
				{
					close(client);
					continue ;
				}

				ev.events = EPOLLIN | EPOLLET | EPOLLRDHUP;
				ev.data.fd = client;
				if (epoll_ctl(_epoll_fd, EPOLL_CTL_ADD, client, &ev) == -1)
				{
					Log(Log::ERROR) << "epoll_ctl for client failed" << Log::endl();
					close(client);
				}
			}
			else
			{
				if (events[n].events & EPOLLRDHUP)
				{
					close(events[n].data.fd);
				}
				else if (events[n].events & EPOLLIN)
				{
					char buffer[4096];
					ssize_t bytes_read;
					std::string request;

					while ((bytes_read = recv(events[n].data.fd, buffer, sizeof(buffer), 0)) > 0)
					{
						request.append(buffer, bytes_read);
					}

					if (bytes_read == -1 && errno != EAGAIN)
					{
						Log(Log::ERROR) << "recv error" << Log::endl();
						close(events[n].data.fd);
						continue;
					}

					std::string	response = "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n";
					response += "<html><body><h1>Hello from webserv</h1></body></html>\n";

					ssize_t	bytes_sent = send(events[n].data.fd, response.c_str(), response.size(), 0);
					if (bytes_sent == -1)
					{
						Log(Log::ERROR) << "send error" << Log::endl();
					}

					close(events[n].data.fd);
				}
			}
		}
	}
}
