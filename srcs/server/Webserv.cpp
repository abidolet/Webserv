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
#define CLOSE(fd) if (fd > 1) close(fd)

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
		Log(Log::DEBUG) << "Custom error page for code " << error_code << ": " << it->second << Log::endl();

		std::string		error_path = it->second;
		std::ifstream	file(error_path.c_str(), std::ios::binary);

		if (file)
		{
			Log(Log::DEBUG) << "Custom error page found: " << error_path << Log::endl();

			std::string			content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
			std::ostringstream	oss;
			oss << content.size();

			return ("HTTP/1.1 " + toString(error_code) + " " + getStatusMessage(error_code) + "\r\n" +
				"Content-Type: text/html\r\n" +
				"Content-Length: " + oss.str() + "\r\n\r\n" +
				content);
		}
		else
		{
			Log(Log::DEBUG) << "Custom error page not found: " << error_path << Log::endl();
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
		"Content-Length: " + oss.str() + "\r\n\r\n" +
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
	Log(Log::DEBUG) << "Get request for: " << path << Log::endl();

	size_t	start = path.find_first_not_of(" \t");
	if (start != std::string::npos)
	{
		path = path.substr(start);
	}

	size_t	end = path.find_last_not_of(" \t");
	if (end != std::string::npos)
	{
		path = path.substr(0, end + 1);
	}

	Log(Log::DEBUG) << "Get request trimmed: " << path << Log::endl();

	for (size_t	i = 0; i < server.locations.size(); ++i)
	{
		size_t		slash_pos = path.find('/', 1);
		std::string	path_part = (slash_pos != std::string::npos) ? path.substr(0, slash_pos) : path;
		Log(Log::DEBUG) << "Checking location: " << server.locations[i].root << " | " << path_part << Log::endl();

		if (server.locations[i].root == path_part)
		{
			Log(Log::DEBUG) << "Location matched: " << server.locations[i].path << Log::endl();
			path = server.locations[i].path + '/' + server.locations[i].index;
			break ;
		}
	}

	std::string		full_path = server.root + path;

	Log(Log::DEBUG) << "Get request final: " << full_path << Log::endl();

	struct stat	statbuf;
	if (stat(full_path.c_str(), &statbuf) != 0)
	{
		Log(Log::ERROR) << "File not found: '" << full_path << "'" << Log::endl();
		return (getErrorPage(404));
	}

	Log(Log::DEBUG) << "File found: '" << full_path << "'" << Log::endl();

	std::ifstream	file(full_path.c_str(), std::ios::binary);
	if (!file)
	{
		Log(Log::ERROR) << "Cannot open " << full_path << Log::endl();
		return (getErrorPage(403));
	}

	std::string			content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
	std::ostringstream	oss;
	oss << content.size();

	std::string	response = "HTTP/1.1 200 OK\r\n";
	response += "Content-Type: text/html\r\n";
	response += "Content-Length: " + oss.str() + "\r\n\r\n";
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
	response += "Content-Length: " + content_length + "\r\n\r\n";
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
		return ("HTTP/1.1 200 OK\r\n\r\nFile deleted successfully :)\r\n");
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
		Log(Log::ALERT) << "Failed to create epoll instance" << Log::endl();
		return ;
	}

	std::cout << "Webserv started on " << _servers.size() << " server(s)" << std::endl;

	for (size_t i = 0; i < _servers.size(); ++i)
	{
		Log(Log::DEBUG) << "Server " << i << " initialized" << Log::endl();
		int	listener_fd = socket(AF_INET, SOCK_STREAM, 0);
		if (listener_fd < 0)
		{
			Log(Log::ALERT) << "Failed to create socket" << Log::endl();
			return ;
		}

		int	opt = 1;
		if (setsockopt(listener_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) == -1)
		{
			Log(Log::ALERT) << "setsockopt failed" << Log::endl();
			return ;
		}

		struct sockaddr_in	server_addr;
		server_addr.sin_family = AF_INET;
		server_addr.sin_addr.s_addr = INADDR_ANY;
		server_addr.sin_port = htons(_servers[i].listen.begin()->second);

		if (bind(listener_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1)
		{
			perror("bind");
			Log(Log::ALERT) << "bind failed" << Log::endl();
			CLOSE(listener_fd);
			return ;
		}

		if (listen(listener_fd, SOMAXCONN) == -1)
		{
			Log(Log::ALERT) << "listen failed" << Log::endl();
			return ;
		}

		struct epoll_event	ev;
		ev.events = EPOLLIN | EPOLLET;
		ev.data.fd = listener_fd;
		if (epoll_ctl(_epoll_fd, EPOLL_CTL_ADD, listener_fd, &ev) == -1)
		{
			Log(Log::ALERT) << "epoll_ctl failed" << Log::endl();
			return ;
		}

		_listener_fds.push_back(listener_fd);
	}

	struct epoll_event	events[MAX_EVENTS];
	int					nfds;
	bool				getAllowed = (_servers[0].allowed_methods.end() != std::find(_servers[0].allowed_methods.begin(), _servers[0].allowed_methods.end(), "GET"));
	bool				postAllowed = (_servers[0].allowed_methods.end() != std::find(_servers[0].allowed_methods.begin(), _servers[0].allowed_methods.end(), "POST"));
	bool				deleteAllowed = (_servers[0].allowed_methods.end() != std::find(_servers[0].allowed_methods.begin(), _servers[0].allowed_methods.end(), "DELETE"));

	while (true)
	{
		nfds = epoll_wait(_epoll_fd, events, MAX_EVENTS, 1000);
		if (nfds == -1)
		{
			Log(Log::ERROR) << "epoll_wait failed" << Log::endl();
			continue ;
		}

		for (unsigned int i = 0; i < _listener_fds.size(); ++i)
		{
			for (int n = 0; n < nfds; ++n)
			{
				if (events[n].data.fd == _listener_fds[i])
				{
					int	client = accept(_listener_fds[i], NULL, NULL);
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

					struct epoll_event	ev;
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
						CgiHandler	cgi(httpReq.method, "", ""); //Need ContentType and ContentLength (if apply)
						std::string	response;

						if (cgi.cgiRequest(httpReq, this->_servers.data()->locations))
						{
							// if (contentLength != 0) cgi.addBody();
							Log(Log::LOG) << "launching cgi" << Log::endl();
							response = cgi.launch();
						}
						else if (httpReq.method == "GET")
						{
							if (!getAllowed)
							{
								Log(Log::ERROR) << "Method GET not allowed" << Log::endl();
								response = getErrorPage(405);
							}
							else
							{
								response = handleGetRequest(_servers[i], httpReq.path);
							}
						}
						else if (httpReq.method == "POST")
						{
							if (!postAllowed)
							{
								Log(Log::ERROR) << "Method POST not allowed" << Log::endl();
								response = getErrorPage(405);
							}
							else
							{
								response = handlePostRequest(httpReq.body);
							}
						}
						else if (httpReq.method == "DELETE")
						{
							if (!deleteAllowed)
							{
								Log(Log::ERROR) << "Method DELETE not allowed" << Log::endl();
								response = getErrorPage(405);
							}
							else
							{
								response = handleDeleteRequest(httpReq.path);
							}
						}

						if (send(events[n].data.fd, response.c_str(), response.size(), 0) == -1)
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
}
