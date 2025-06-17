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

struct	HttpRequest
{
	std::string							method;
	std::string							body;
	std::string							path;
	std::map<std::string, std::string>	headers;
};

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

	int				directoryListing;

	std::pair<int, std::string> redirection;

	std::vector<std::string>	allowed_methods;
private:
	void	setupLocationRoot(const Block& block);

};

struct Session
{
	std::string	key;
	size_t	uid;
	size_t		visitCount;

	static Session		registerSession(const std::string& key); // use the $USER for seed ?
	static Session		stringToSession(std::string &str);
	static std::string	sessionToString(const Session& session);
	static Session*		find(std::vector<Session> &sessions, size_t);
	static Session*		find(std::vector<Session> &sessions, const std::string &key);
};

struct	Server
{
	Server()
		: root("/"), server_name("localhost"), client_max_body_size(0)
	{
		listen.insert(std::pair<std::string, int>("0.0.0.0", 8080));
	}

	void		init(Block& block);
	void		runSelfCheck();
	void		cookiesAssert();

	Location*	searchLocationByName(const std::string &name);
	static void	registerSession(const std::string &key);

	std::string					root;

	std::string					server_name;
	size_t						client_max_body_size;

	std::vector<std::string>	allowed_methods;
	std::vector<Location>		locations;

	std::map<int, std::string>	error_pages;
	std::map<std::string, int>	listen;

	std::vector<std::string>	cookies;
	std::map<std::string, int>	sessions;

private:
	void	setupMaxBodySize(Block& block);
	void	setupRedirections(Block& block);
	void	setupListen(Block &block);

};

class	Webserv
{
	private:
		std::vector<Server>	_servers;

		int					_epoll_fd;
		int					_listener_fd;

		std::string			cookies;

	public:
		Webserv(const std::string& file);
		~Webserv();

		void	run();

		std::string	handleGetRequest(std::string& path) const;
		std::string	handlePostRequest(const std::string& body) const;
		std::string	handleDeleteRequest(const std::string& request) const;

		std::string	getErrorPage(int error_code) const;
		std::string	getStatusMessage(int code) const;
};

#endif
