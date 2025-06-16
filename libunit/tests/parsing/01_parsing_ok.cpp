#include "Parser.hpp"
#include "Webserv.hpp"

#define COLORS // color define already present in Log.hpp
#include "libunit.hpp"


int	ok_config()
{
	Libunit::Redirect_log();
	Parser parser("./configs/ok/valid.conf");
	Server serv = parser.populateServerInfos();
	
	return 0;
}

int	multiserv()
{
	Libunit::Redirect_log();

	Parser parser("./configs/ok/multiserv.conf");
	Server serv = parser.populateServerInfos();
	
	return 0;
}

int	whitespace()
{
	Libunit::Redirect_log();
	Parser parser("./configs/ok/whitespace.conf");
	Server serv = parser.populateServerInfos();
	
	return 0;
}

int	multLocation()
{
	Libunit::Redirect_log();
	Parser parser("./configs/ok/multLocation.conf");
	Server serv = parser.populateServerInfos();
	
	if (serv.locations.size() == 4 && serv.locations[3].is_cgi == 1)
		return 0;
	return 1;
}

int	pagePath()
{
	Libunit::Redirect_log();
	Parser parser("./configs/ok/multErrorPages.conf");
	Server serv = parser.populateServerInfos();

	std::map<int, std::string>::iterator it;
	it = serv.error_pages.find(403); 
	if (it->second != "/www/test/errors_pages/403.html")
	{
		Log(Log::ERROR) << it->first << it->second << Log::endl();
		return (1);
	}
	it = serv.error_pages.find(404); 
	if (it->second != "/errors/404.html")
	{
		Log(Log::ERROR) << it->first << it->second << Log::endl();
		return (1);

	}

	return 0;
}

int	empty()
{
	Libunit::Redirect_log();
	Parser parser("./configs/ok/empty.conf");
	Server serv = parser.populateServerInfos();
	
	return 0;
}