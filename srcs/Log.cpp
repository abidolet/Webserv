#include "Log.hpp"

Log::Log() {}

Log::~Log() {}

void	Log::info(const std::string& message)
{
	std::cout << GREEN << "[INFO] " << RESET << message << std::endl;
}

void	Log::warning(const std::string& message)
{
	std::cout << YELLOW << "[WARNING] " << RESET << message << std::endl;
}

void	Log::error(const std::string& message)
{
	std::cerr << RED << "[ERROR] " << RESET << message << std::endl;
}

void	Log::debug(const std::string& message)
{
	std::cout << BLUE << "[DEBUG] " << RESET << message << std::endl;
}

void Log::log(const std::string& message, const std::string& type)
{
	switch (type[0])
	{
		case 'i': info(message); break ;
		case 'w': warning(message); break ;
		case 'e': error(message); break ;
		case 'd': debug(message); break ;
		default:
			std::cerr << "Unknown log type: " << type << std::endl;
	}
}
