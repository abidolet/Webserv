#ifndef WEBSERV_HPP
# define WEBSERV_HPP

#include <iostream>
#include <vector>
#include <map>

struct Location
{
	Location()
		: path("/var/www/html"), root("/"), index("index.html"), cgi_pass("TODO"), cgi_extensions(std::vector<std::string>(1, "TODO")) {}

	std::string					path;
	std::string					root;
	std::string					index;
	std::string					cgi_pass;
	std::vector<std::string>	cgi_extensions;
};

struct Server
{
	Server() // TODO: penser a recheck les valeur
		: port(8080), host("0.0.0.0"), server_name("localhost"), client_max_body_size(100), is_default(true), redirection(std::pair<int, std::string>(-1, "NONE")) {}

	int							port;
	std::string					host;
	std::string					server_name;
	
	size_t						client_max_body_size;
	std::vector<std::string>	allowed_methodes;
	std::map<int, std::string>	error_pages;

	std::vector<Location>		locations;

	bool						is_default;
	std::pair<int, std::string>	redirection;
};

class Webserv
{
	private:
		std::vector<Server>	_servers;

		int					_epoll_fd;
		int					_listener_fd;

	public:
		Webserv();
		Webserv(const std::string& file);
		~Webserv();

		void	run();
};

#endif
