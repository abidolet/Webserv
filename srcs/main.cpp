#include "Webserv.hpp"

int main (int argc, char *argv[])
{
	if (argc > 2)
	{
		std::cerr << "Error: Too many arguments" << std::endl;
		return (1);
	}

	try
	{
		if (argc == 1)
		{
			Webserv	serv;
		}
		else
		{
			Webserv	serv(argv[1]);
		}
		// Other functions
		return (0);
	}
	catch (const std::exception& e)
	{
		std::cerr << "Error: " << e.what() << std::endl;
		return (1);
	}
}
