#ifndef WEBSERV_HPP
# define WEBSERV_HPP

# include <iostream>
# include <vector>
# include <map>

# include "Block.hpp"

#include "CgiHandler.hpp"

#ifndef RUN_SERV_SELF_CHECK
# define RUN_SERV_SELF_CHECK 1
#endif

# define CLOSE(fd) if (fd > 1) {close(fd); fd = -1;}
# define THROW(msg) throw std::runtime_error(msg + static_cast<std::string>(strerror(errno)));
# define ERROR(msg) Log(Log::ERROR) << msg << strerror(errno) << Log::endl();

struct 	Location
{
	Location()
	:	path("/"), root("/"), index(""),
		is_cgi(false), directoryListing(false), redirection(-1, "") // ca casse si on met dirListing a true
		{
			std::string	defaults[] = {"GET", "POST", "DELETE"};
			allowed_methods = std::vector<std::string>(defaults, defaults + 3);
		}

	Location(Block& block);

	std::string		path;
	std::string		root;
	std::string		index;
	std::string		cgi_pass;
	std::string		cgi_extension;
	bool			is_cgi;

	bool			directoryListing;
	std::string		upload_dir;

	std::pair<int, std::string> redirection;

	std::vector<std::string>	allowed_methods;
private:
	void	setupLocationRoot(const Block& block);

};

struct	HttpRequest
{
	std::string							method;
	std::string							body;
	std::string							path;
	std::map<std::string, std::string>	headers;
	Location							location;
	bool								method_allowed;
};
std::ostream& operator<<(std::ostream& stream, HttpRequest& request);
struct Session
{
	size_t	uid;
	size_t	visitCount;

	Session();
	Session(uint _uid);

	std::string			sessionToString();
	static Session		stringToSession(std::string &str);
	static Session*		find(std::vector<Session> &sessions, size_t uid);
};
std::ostream& operator<<(std::ostream& stream, const Session& session);

struct Listen
{
	std::string	addr;
	int 		port;

	Listen(std::string _addr, int _port) : addr(_addr), port(_port) { };
	bool operator==(const Listen& other);
};
size_t find(std::vector<Listen> vec, Listen toFind);

struct	Server
{
	Server()
		: root("/"), client_max_body_size(0)
	{
		server_names.push_back("localhost");
		listen.push_back(Listen("0.0.0.0", 8080));
	}

	std::string handlePostRequest(HttpRequest request) const;
	void		init(Block& block);
	void		runSelfCheck();

	void		cookiesAssert();
	std::string	getCookies() const;

	Location*	searchLocationByName(const std::string &name);
	static void	registerSession(const uint uid);

	std::string					root;
	std::vector<std::string>	server_names;
	size_t						client_max_body_size;

	std::vector<std::string>	allowed_methods;
	std::vector<Location>		locations;

	std::map<int, std::string>	error_pages;
	std::vector<Listen>			listen;

	bool						is_default;

	std::vector<std::string>	cookies;
	uint						lastUID;

private:
	void	setupMaxBodySize(Block& block);
	void	setupRedirections(Block& block);
	void	setupListen(Block &block);

};

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

		const std::string	getErrorPage(const int error_code, const Server& server) const;
		const std::string	getUrlPage(const int error_code, const std::string &content, const std::string &location) const;
	public:
		Webserv(const std::string& file);
		~Webserv();

		void	run();
};

const std::string	toString(const int value);
const std::string	generatePage(const int code, const std::string &content);

#endif
