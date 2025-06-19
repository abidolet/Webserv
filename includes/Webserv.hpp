#ifndef WEBSERV_HPP
# define WEBSERV_HPP

# include <iostream>
# include <vector>
# include <map>

# include "Block.hpp"

#include "CgiHandler.hpp"

#ifndef RUN_SERV_SELF_CHECK
# define RUN_SERV_SELF_CHECK 0
#endif

struct 	Location
{
	Location()
		: path("/"), root("/"), index("index.html"), is_cgi(false) { }
	Location(Block& block);

	std::string		path;
	std::string		root;
	std::string		index;
	std::string		cgi_pass;
	std::string		cgi_extension;
	bool			is_cgi;

	bool			directoryListing;

	std::pair<int, std::string> redirection;

	std::vector<std::string>	allowed_methods;
private:
	void	setupLocationRoot(const Block& block);

};

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

struct	Server
{
	Server()
		: root("/"), server_name("localhost"), client_max_body_size(0)
	{
		listen.push_back(std::pair<std::string, int>("0.0.0.0", 8080));
	}

	std::string handlePostRequest(std::string body) const;
	void		init(Block& block);
	void		runSelfCheck();

	void		cookiesAssert();
	std::string	getCookies() const;

	Location*	searchLocationByName(const std::string &name);
	static void	registerSession(const uint uid);
	
	std::string					root;
	
	std::string					server_name;
	size_t						client_max_body_size;
	
	std::vector<std::string>	allowed_methods;
	std::vector<Location>		locations;
	
	std::map<int, std::string>					error_pages;
	std::vector<std::pair<std::string, int> >	listen;
	
	std::vector<std::string>	cookies;
	uint						lastUID;

private:
	void	setupMaxBodySize(Block& block);
	void	setupRedirections(Block& block);
	void	setupListen(Block &block);

};

struct	HttpRequest
{
	std::string							method;
	std::string							body;
	std::string							path;
	std::map<std::string, std::string>	headers;
	bool								method_allowed;
};

class	Webserv
{
	private:
		std::vector<Server>	_servers;
		int					_epoll_fd;
		std::vector<int>	_listener_fds;
		std::map<int, int>	_client_to_server;
		std::string			cookies;

		const HttpRequest	parseRequest(const std::string& rawRequest, const Server& server) const;
		const std::string	handleGetRequest(std::string& path, const Server& server) const;
		const std::string	handlePostRequest(const std::string& body, const Server& server) const;
		const std::string	handleDeleteRequest(const std::string& path, const Server& server) const;

		const std::string	getErrorPage(const int error_code, const Server& server) const;
		const std::string	getStatusMessage(const int code) const;

	public:
		Webserv(const std::string& file);
		~Webserv();

		void	run();
};

#endif
