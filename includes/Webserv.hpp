#ifndef WEBSERV_HPP
# define WEBSERV_HPP

# include <iostream>
# include <vector>
# include <map>
# include <unistd.h>
# include <algorithm>

# include "Block.hpp"
# include "Log.hpp"
# include "Location.hpp"
# include "Server.hpp"
# include "Session.hpp"

#ifndef RUN_SERV_SELF_CHECK
# define RUN_SERV_SELF_CHECK 1
#endif

# define CLOSE(fd) if (fd > 1) {close(fd); fd = -1;}
# define THROW(msg) throw std::runtime_error(msg + static_cast<std::string>(strerror(errno)));
# define THROW_GAI(msg, ret) throw std::runtime_error("getaddrinfo failed" + static_cast<std::string>(gai_strerror(ret)))
# define ERROR(msg) Log(Log::ERROR) << msg << strerror(errno) << Log::endl();

struct	HttpRequest
{
	std::string							method;
	std::string							body;
	std::string							path;
	std::map<std::string, std::string>	headers;
	Location							location;
	Server								server;
	bool								method_allowed;
};
std::ostream& operator<<(std::ostream& stream, HttpRequest& request);

struct File
{
	std::string name;
	long int	size;
};

class	Webserv
{
	private:
		std::vector<Server>	_servers;
		int					_epoll_fd;
		std::vector<int>	_listener_fds;
		std::map<int, int>	_client_to_server;

		const HttpRequest	parseRequest(const std::string& rawRequest, const Server& server) const;
		const std::string	handleGetRequest(HttpRequest& request, const Server& server) const;
		const std::string	handlePostRequest(const HttpRequest& request, const Server& server) const;
		const std::string	handleDeleteRequest(const std::string& path, const Server& server) const;

	public:
		Webserv(const std::string& file);
		~Webserv();

		void	run();
		void	init_servers();
};

const std::string	toString(const int value);
const std::string	generatePage(const int code, const std::string &content, const std::string &name);
const std::string	getUrlPage(const int code, const std::string &location);
const std::string	getErrorPage(const int error_code, const Server& server);

std::string getDirectoryListing(HttpRequest& request);
std::vector<File> getFilesInDir(const std::string path);

bool isTTY(const char* name);

std::vector<Session> readSessions(const std::string& sessionFilepath);


#endif
