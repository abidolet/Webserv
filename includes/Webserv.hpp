#ifndef WEBSERV
#define WEBSERV

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
	int							port;
	std::string					host;
	std::string					server_name;
	bool						is_default;
	std::map<int, std::string>	error_pages;
	size_t						client_max_body_size;
	std::vector<Location>		locations;
};

class Webserv
{
	private:
		std::vector<Server>	servers;
	
	public:
		Webserv();
		Webserv(const std::string& file);
		~Webserv();
};

#endif
