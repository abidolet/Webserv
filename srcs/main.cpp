#include "Webserv.hpp"
#include "Log.hpp"
#include "Parser.hpp"
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
	// Log::disableFlags(F_DEBUG);
	signal(SIGINT, handle_signal);
	signal(SIGQUIT, handle_signal);
	signal(SIGTERM, handle_signal);

	std::string	file;

#if ENABLE_LOG_FILE
	Log::setupLogFile();
#endif

	if (argc == 1)
	{
		file = DEFAULT_CONF_PATH;
	}
	else if (argc == 2)
	{
		file = argv[1];
	}
	else
	{
		Log(Log::ERROR) << "Usage: ./webserv [config_file]" << Log::endl();
		return (1);
	}

	try
	{
		Webserv	server(file);
		server.run();
	}
	catch (const std::exception& e)
	{
		if (static_cast<std::string>(e.what()) != "exit")
		{
			Log(Log::ERROR) << e.what() << Log::endl();
			return (1);
		}
	}
	return (0);
}




