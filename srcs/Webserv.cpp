#include "Webserv.hpp"
#include "Color.hpp"

Webserv::Webserv()
	: _servers()
{
	Server  default_server;

	default_server.port = 8080;
	default_server.host = "0.0.0.0";
	default_server.server_name = "nginx";
	default_server.is_default = true;
	_servers.push_back(default_server);
}

Webserv::Webserv(const std::string& file)
	: _servers()
{
	(void)file;
}

Webserv::~Webserv()
{
}

void Webserv::run()
{
	std::cout << GRAY << "Running web server..." << RESET << std::endl;
}
