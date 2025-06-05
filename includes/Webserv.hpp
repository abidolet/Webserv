#ifndef WEBSERV_HPP
# define WEBSERV_HPP

#include <iostream>
#include <vector>
#include <map>

struct Location
{
	std::string					path;
	std::string					root;
	std::string					redirect;
	std::string					cgi_pass;
	std::vector<std::string>	cgi_extensions;
};

struct Server
{
	Server() // TODO: penser a recheck les valeur
		: port(8080), host("localhost"), server_name("localhost"), is_default(true), client_max_body_size(100), fd(-1) {}

	int							port;
	std::string					host;
	std::string					server_name;
	bool						is_default;
	std::map<int, std::string>	error_pages;
	size_t						client_max_body_size;
	std::vector<Location>		locations;
	int							fd;
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
