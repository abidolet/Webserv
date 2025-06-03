#include "Log.hpp"
#include <stdarg.h>

#include <ctime>
#include <iomanip>
#include <iostream>

void displayTimestamp(void)
{
    std::time_t time = std::time(NULL);
	std::tm		*tm = std::localtime(&time);

	int	h = tm->tm_hour;
	int	min = tm->tm_min;
	int	s = tm->tm_sec;
	std::cout << GRAY << "["
		<< std::setfill('0') << std::setw(2) << h << ":"
		<< std::setfill('0') << std::setw(2) << min << ":"
		<< std::setfill('0') << std::setw(2) << s << "] ";
}

Log::Log(Log::Type type)
{
	displayTimestamp();	
	switch (type)
	{
		case LOG:
			std::cout << GRAY << "[LOG]     " << RESET;
			break;
		case WARNING:
			std::cout << YELLOW << "[WARNING] " << RESET;
			break;
		case ERROR: 
			std::cout << RED << "[ERROR]   " << RESET;
			break;
		case ALERT:
			std::cout << B_RED<< "[ALERT]    " << RESET;
			break;
		case DEBUG:
			std::cout << PURPLE << "[DEBUG]   " << RESET;
			break;
	}
}


Log::~Log()
{
	std::cout << oss.str() << RESET << std::endl;
}

