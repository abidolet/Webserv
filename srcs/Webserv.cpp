#include "Webserv.hpp"

Webserv::Webserv()
	: servers()
{
	Server  default_server;

	default_server.port = 8080;
	default_server.host = "0.0.0.0";
	default_server.server_name = "nginx";
	default_server.is_default = true;
	servers.push_back(default_server);
}

Webserv::Webserv(const std::string& file)
	: servers()
{
	(void)file;
}

Webserv::~Webserv()
{
}
