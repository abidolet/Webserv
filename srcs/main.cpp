#include "Webserv.hpp"
#include "Log.hpp"

int main (int argc, char *argv[])
{
	if (argc > 2)
	{
		log("Usage: ./webserv [config_file]", "e");
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
		std::cerr << RED << "Error: " << e.what() << RESET << std::endl;
		return (1);
	}
	return (0);
}
