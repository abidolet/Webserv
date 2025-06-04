#include "Webserv.hpp"
#include "Log.hpp"

int main (int argc, char *argv[])
{
	if (argc > 2)
	{
		Log(Log::WARNING) << "Usage: ./webserv [config_file]" << Log::endl();
		return (1);
	}

	try
	{
		Webserv	server;
		if (argc == 2)
		{
			Webserv	server(argv[1]);
		}

		server.run();
	}
	catch (const std::exception& e)
	{
		Log(Log::ERROR) << e.what() << Log::endl();
		return (1);
	}
	return (0);
}


