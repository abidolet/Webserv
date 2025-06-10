#include "Webserv.hpp"
#include "Log.hpp"
#include <csignal>
#include <cstdlib>

std::string	get_sig_name(int signal)
{
	switch (signal)
	{
		case SIGINT:	return ("SIGINT");
		case SIGQUIT:	return ("SIGQUIT");
		case SIGTERM:	return ("SIGTERM");
		default:		return ("Other signal");
	}
}

void	handle_signal(int signal)
{
	std::cout << std::endl;
	Log() << "Process terminating with default action of signal"
		<< signal << get_sig_name(signal) << Log::endl();
	throw std::runtime_error("exit");
}

int main (int argc, char *argv[])
{
	if (argc > 2)
	{
		Log(Log::WARNING) << "Usage: ./webserv [config_file]" << Log::endl();
		return (1);
	}

	// Log::disableFlags(F_DEBUG | F_LOG);

	try
	{
		signal(SIGINT, handle_signal);
		signal(SIGQUIT, handle_signal);
		signal(SIGTERM, handle_signal);

		Webserv	server;
		if (argc == 2)
		{
			Webserv	server(argv[1]);
		}

		server.run();
	}
	catch (const std::exception& e)
	{
		if (static_cast<std::string>(e.what()) != "exit")
		{
			Log(Log::ERROR) << e.what() << Log::endl();
		}
		return (1);
	}
	return (0);
}


