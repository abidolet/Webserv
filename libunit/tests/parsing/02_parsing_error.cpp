#include "Parser.hpp"
#include "Webserv.hpp"
#include "Block.hpp"

#define COLORS // color define already present in Log.hpp
#include "libunit.hpp"


int	Err_listen1()
{
	Libunit::Redirect_log();

	Block block("server");
	block.directives.push_back("listen 8080:");
	Server serv;
	
	try
	{
		serv.init(block);
	}
	catch (std::exception& e)
	{
		return 0;
	}

	return 1;
}

int	Err_listen2()
{
	Libunit::Redirect_log();

	Block block("server");
	block.directives.push_back("listen 0.0.0.0:");
	Server serv;
	
	try
	{
		serv.init(block);
	}
	catch (std::exception& e)
	{
		return 0;
	}

	return 1;
}

int	Err_listen3()
{
	Libunit::Redirect_log();

	Block block("server");
	block.directives.push_back("listen :8080");
	Server serv;
	
	try
	{
		serv.init(block);
	}
	catch (std::exception& e)
	{
		return 0;
	}

	return 1;
}

int	Err_listen4()
{
	Libunit::Redirect_log();

	Block block("server");
	block.directives.push_back("listen 0.0.00:8080");
	Server serv;
	
	try
	{
		serv.init(block);
	}
	catch (std::exception& e)
	{
		return 0;
	}

	return 1;
}

int	Err_listen5()
{
	Libunit::Redirect_log();

	Block block("server");
	block.directives.push_back("listen 0.0.0.0.0.0:8080");
	Server serv;
	
	try
	{
		serv.init(block);
	}
	catch (std::exception& e)
	{
		return 0;
	}

	return 1;
}

int	Err_listen6()
{
	Libunit::Redirect_log();

	Block block("server");
	block.directives.push_back("listen :0.0.0.0:8080");
	Server serv;
	
	try
	{
		serv.init(block);
	}
	catch (std::exception& e)
	{
		return 0;
	}

	return 1;
}

int	Err_noBeginBrace()
{
	Libunit::Redirect_log();

	try
	{
		Parser parser("./configs/errors/noBeginBrace.conf");
		Server serv = parser.populateServerInfos();
		
	}
	catch(const std::exception& e)
	{
		return 0;
	}
	
	
	return 1;
}


int	Err_noEndBrace()
{
	Libunit::Redirect_log();

	try
	{
		Parser parser("./configs/errors/noEndBrace.conf");
		Server serv = parser.populateServerInfos();
		
	}
	catch(const std::exception& e)
	{
		return 0;
	}
	
	return 1;
}

int	Err_Cgi()
{
	Libunit::Redirect_log();

	try
	{
		Parser parser("./configs/errors/Cgi.conf");
		Server serv = parser.populateServerInfos();
		
	}
	catch(const std::exception& e)
	{
		return 0;
	}
	
	return 1;
}

int	Err_wrongLocation()
{
	Libunit::Redirect_log();

	try
	{
		Parser parser("./configs/errors/invalidDirName.conf");
		Server serv = parser.populateServerInfos();
		
	}
	catch(const std::exception& e)
	{
		return 0;
	}
	
	return 1;
}

int	Err_Cookies1()
{
	Libunit::Redirect_log();
	Parser parser("./configs/ok/empty.conf");
	Server serv = parser.populateServerInfos();

	serv.cookies.push_back("=");
	serv.cookiesAssert();

	return 0;
}

int	Err_Cookies2()
{
	Libunit::Redirect_log();
	Parser parser("./configs/ok/empty.conf");
	Server serv = parser.populateServerInfos();

	serv.cookies.push_back("=test");
	serv.cookiesAssert();

	return 0;
}

int	Err_Cookies3()
{
	Libunit::Redirect_log();
	Parser parser("./configs/ok/empty.conf");
	Server serv = parser.populateServerInfos();

	serv.cookies.push_back("test=");
	serv.cookiesAssert();

	return 0;
}

int	Err_Cookies4()
{
	Libunit::Redirect_log();
	Parser parser("./configs/ok/empty.conf");
	Server serv = parser.populateServerInfos();

	serv.cookies.push_back("test test");
	serv.cookiesAssert();

	return 0;
}

int	Err_Cookies5()
{
	Libunit::Redirect_log();
	Parser parser("./configs/ok/empty.conf");
	Server serv = parser.populateServerInfos();

	serv.cookies.push_back("=test=");
	serv.cookiesAssert();

	return 0;
}