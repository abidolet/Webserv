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

static const std::string	toString(const int value)
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

static const std::string	generatePage(const int error_code, const std::string &content)
{
	return ("HTTP/1.1 " + toString(error_code) + " " + getStatusMessage(error_code) + "\r\n" + "Content-Type: text/html\r\n"
		+ "Connection: close\r\n" + "Content-Length: " + toString(content.size()) + "\r\n\r\n" + content);
}

const std::string	Webserv::getErrorPage(const int error_code, const Server& server) const
{
	Log(Log::DEBUG) << "Searching custom error page for code" << error_code << Log::endl();

	if (error_code != 500)
	{
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
	}

	Log(Log::DEBUG) << "Custom error page not found, returning default" << Log::endl();
	return (generatePage(error_code, ""));
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
		const Location& loc = server.locations[i];
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
		while (full_path[0] == '/')
		{
			full_path.erase(0, 1);
			Log(Log::DEBUG) << "Removed leading slash from path:" << full_path << Log::endl();
		}
		while (full_path[full_path.length() - 1] == '/')
		{
			full_path.erase(full_path.length() - 1);
			Log(Log::DEBUG) << "Removed trailing slash from path:" << full_path << Log::endl();
		}
		while (!request.path.empty() && request.path[0] == '/')
		{
			request.path.erase(0, 1);
			Log(Log::DEBUG) << "Removed leading slash from path:" << request.path << Log::endl();
		}
		while (!request.path.empty() && request.path[request.path.length() - 1] == '/')
		{
			Log(Log::DEBUG) << "Removed trailing slash from path:" << request.path << Log::endl();
			request.path.erase(request.path.length() - 1);
		}
		full_path = '/' + full_path + '/' + request.path;
		Log(Log::DEBUG) << "Full path constructed:" << full_path << Log::endl();

		Log(Log::DEBUG) << "Checking file stats for:" << full_path << Log::endl();
		struct stat	statbuf;
		if (stat(full_path.c_str(), &statbuf) == 0 && S_ISDIR(statbuf.st_mode) && !best_match->directoryListing)
		{
			std::string	index = best_match->index;
			Log(Log::DEBUG) << "Path is a directory adding index:" << index << Log::endl();
			while (index[0] == '/')
			{
				index = index.substr(1);
				Log(Log::DEBUG) << "Removed leading slash from index:" << index << Log::endl();
			}
			if (full_path[full_path.length() - 1] != '/')
			{
				full_path += '/';
				Log(Log::DEBUG) << "Added trailing slash to full path:" << full_path << Log::endl();
			}
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
		.last_modif_date=0,
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
		files.push_back(getFile(path + info->d_name, info)); // TODO: maybe add other info
	}
	closedir(dir);
	return files;
}

std::string getElt(const File& file, const std::string& path)
{
	Log(Log::WARNING) << path + file.name << Log::endl();
	std::stringstream ss;

	ss << "<div class='elt'><a href='" << (path + file.name) << "'>" \
	 << file.name << "</a> <p>" << file.size << "bytes</p></div>";
	return ss.str();
}

std::string getURL(HttpRequest& request, std::string path)
{
	if (request.path.find(request.location.path) == (size_t)-1)
		throw std::runtime_error("path not found in request wtf");

	int idx = request.path.find(request.location.path) + request.location.path.size();
	std::string tmp = path.substr(idx);
	std::string final = request.location.root + tmp;
	return (final);
}

std::string getDirectoryListing(HttpRequest& request)
{
	std::stringstream ss;
	std::vector<File> files = getFilesInDir(request.path);

	ss << "<html>";
	ss << "<head>";
	ss << "	<style>";
	ss << "	.elt {display:flex; flex-direction:row; gap: 65px; align-items: center; justify-content: space-between; width: 25vw;}";
	ss << "	body {display:flex; flex-direction:column;}";
	ss << "	</style>";
	ss << "</head>";
	ss << "<body>";
	ss << "	<h1>" << request.path << "</h1>";
	ss << "	<div class='elt'><p>name </p> <p> | </p> <p>size</p></div>";
	for (size_t i = 0; i < files.size(); i++)
	{
		ss << getElt(files[i], getURL(request, request.path));
	}
	ss << "</body>";
	ss << "</html>";

	return (generatePage(200, ss.str()));
}

const std::string	Webserv::handleGetRequest(HttpRequest& request, const Server& server) const
{
	std::string path = request.path;

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
	if (request.body.size() > server.client_max_body_size && server.client_max_body_size > 0)
	{
		Log(Log::WARNING) << "Body size exceeds client_max_body_size" << Log::endl();
		return (getErrorPage(413, server));
	}
	else if (request.location.upload_dir.empty())
	{
		Log(Log::WARNING) << "Upload directory is not set for POST request" << Log::endl();
		Log(Log::SUCCESS) << "Post request answered !" << Log::endl();
		return (generatePage(200, server.handlePostRequest(request.body)));
	}
	else
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

		std::string filepath = request.location.upload_dir + "/" + filename;
		std::ofstream outfile(filepath.c_str(), std::ios::binary);
		if (!outfile)
		{
			return (getErrorPage(500, server));
		}
		outfile.write(request.body.c_str(), request.body.size());
		outfile.close();

		return (generatePage(200, "File uploaded successfully to " + filepath));
	}
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

	if (remove(path.c_str()) == 0)
	{
		Log(Log::SUCCESS) << "File deleted !" << path << Log::endl();
		return (generatePage(200, "File deleted successfully"));
	}
	else
	{
		Log(Log::ERROR) << "Cannot delete " << path << Log::endl();
		return (getErrorPage(403, server));
	}
}

void Webserv::run()
{
	Log() << "Running web server..." << Log::endl();

	_epoll_fd = epoll_create(EPOLL_CLOEXEC);
	if (_epoll_fd == -1)
	{
		throw std::runtime_error("Failed to create epoll instance: " + static_cast<std::string>(strerror(errno)));
	}

	for (size_t i = 0; i < _servers.size(); ++i)
	{
		Log(Log::DEBUG) << "Initializing server" << i << Log::endl();

		for (std::vector<std::pair<std::string, int> >::const_iterator	it = _servers[i].listen.begin();
			it != _servers[i].listen.end(); ++it)
		{
			int	listener_fd = socket(AF_INET, SOCK_STREAM, 0);
			if (listener_fd < 0)
			{
				throw std::runtime_error("Failed to create socket: " + static_cast<std::string>(strerror(errno)));
			}

			_listener_fds.push_back(listener_fd);

			int	opt = 1;
			if (setsockopt(listener_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) == -1)
			{
				throw std::runtime_error("setsockopt adress failed: " + static_cast<std::string>(strerror(errno)));
			}

			struct sockaddr_in	server_addr;
			server_addr.sin_family = AF_INET;
			server_addr.sin_addr.s_addr = INADDR_ANY;
			server_addr.sin_port = htons(it->second);

			Log(Log::DEBUG) << "Server listening on port " << it->second << Log::endl();

			if (bind(listener_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1)
			{
				throw std::runtime_error("bind failed: " + static_cast<std::string>(strerror(errno)));
			}

			if (listen(listener_fd, SOMAXCONN) == -1)
			{
				throw std::runtime_error("listen failed: " + static_cast<std::string>(strerror(errno)));
			}

			struct epoll_event	ev;
			ev.events = EPOLLIN;
			ev.data.fd = listener_fd;
			if (epoll_ctl(_epoll_fd, EPOLL_CTL_ADD, listener_fd, &ev) == -1)
			{
				throw std::runtime_error("epoll_ctl failed: " + static_cast<std::string>(strerror(errno)));
			}
		}
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
				client_ev.events = EPOLLIN | EPOLLRDHUP;
				client_ev.data.fd = client;
				if (epoll_ctl(_epoll_fd, EPOLL_CTL_ADD, client, &client_ev) == -1)
				{
					Log(Log::ERROR) << "epoll_ctl for client failed:" << strerror(errno) << Log::endl();
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
					for (std::vector<std::pair<std::string, int> >::const_iterator	it = s.listen.begin(); it != s.listen.end(); ++it)
					{
						if (it->second == port && (host_header.empty() || s.server_name == host_header))
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
						for (std::vector<std::pair<std::string, int> >::const_iterator it = s.listen.begin(); it != s.listen.end(); ++it)
						{
							if (it->second == port)
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
				std::string	response;
				CgiHandler	cgi(httpReq.method, "text/html", ""); //Need ContentType and ContentLength (if apply)

				Server::registerSession(addr.sin_addr.s_addr);

				if (!httpReq.method_allowed)
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
					Log(Log::ERROR) << "send error:" << strerror(errno) << Log::endl();
				}

				epoll_ctl(_epoll_fd, EPOLL_CTL_DEL, fd, NULL);
				CLOSE(fd);
			}
		}
	}
}

std::ostream& operator<<(std::ostream& stream, const HttpRequest& request)
{
	// stream << "" << request.
	(void)request;
	return stream;
}
