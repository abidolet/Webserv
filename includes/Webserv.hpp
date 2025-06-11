#ifndef WEBSERV_HPP
# define WEBSERV_HPP

# include <iostream>
# include <vector>
# include <map>

#include "CgiHandler.hpp"

struct	HttpRequest
{
	std::string							method;
	std::string							body;
	std::string							path;
	std::map<std::string, std::string>	headers;
};

struct	Location
{
#if old
	Location()
		: path("/var/www/html"), root("/"), index("index.html"), cgi_pass("TODO"), cgi_extensions(std::vector<std::string>(1, "TODO")) {}
#endif

	Location()
	: path("/var/www/html"), root("/"), index("index.html"), cgi_pass("TODO"), cgi_extension("TODO") {}

		std::string		path;
		std::string		root;
		std::string		index;
		std::string		cgi_pass;
		std::string		cgi_extension;
};

struct	Server
{
	Server() // TODO: penser a recheck les valeur
		: server_name("localhost"), client_max_body_size(0)
	{
		listen.insert(std::pair<std::string, int>("0.0.0.0", 8080));
	}

	std::string					server_name;
	size_t						client_max_body_size;

	std::vector<std::string>	allowed_methods;
	std::vector<Location>		locations;

	std::map<int, std::string>	error_pages;
	std::map<std::string, int>	listen;
};

class	Webserv
{
	private:
		std::vector<Server>	_servers;

		int					_epoll_fd;
		int					_listener_fd;

		// HttpRequest	parseRequest(const std::string& rawRequest);
		// std::string	handleGetRequest(const std::string& request);
		// std::string	handlePostRequest(void);
		// std::string	handleDeleteRequest(const std::string& request);

	public:
		Webserv();
		Webserv(const std::string& file);
		~Webserv();

		void	run();
};

#endif
