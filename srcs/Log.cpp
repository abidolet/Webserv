#include "Log.hpp"
#include "Webserv.hpp"

#include <ctime>
#include <iomanip>
#include <iostream>

uint16_t Log::m_flags = F_LOG | F_DEBUG | F_WARNING | F_SUCCESS | F_ERROR | F_ALERT;

void Log::displayTimestamp(void)
{
    std::time_t time = std::time(NULL);
	std::tm		*tm = std::localtime(&time);

	int	h = tm->tm_hour;
	int	min = tm->tm_min;
	int	s = tm->tm_sec;
	m_oss << GRAY << "["
		<< std::setfill('0') << std::setw(2) << h << ":"
		<< std::setfill('0') << std::setw(2) << min << ":"
		<< std::setfill('0') << std::setw(2) << s << "] ";
}

void Log::setFlags(uint16_t flags)
{
	m_flags = flags;
}

void Log::enableFlags(uint16_t flags)
{
	m_flags |= flags;
}


void Log::disableFlags(uint16_t flags)
{
	m_flags &= ~flags;
}


void Log::toggleFlags(uint16_t flags)
{
	m_flags ^= flags;
}


Log::Log(Log::Type type)
{
	m_type = type;	
	displayTimestamp();	
	switch (type)
	{
		case LOG:
			m_oss << GRAY << "[LOG]     " << RESET;
			break;
		case WARNING:
			m_oss << YELLOW << "[WARNING] " << RESET;
			break;
		case ERROR: 
			m_oss << RED << "[ERROR]   " << RESET;
			break;
		case ALERT:
			m_oss << B_RED<< "[ALERT]    " << RESET;
			break;
		case DEBUG:
			m_oss << PURPLE << "[DEBUG]   " << GRAY;
			break;
		case SUCCESS:
			m_oss << GREEN << "[SUCCESS] " << RESET;

	}
}

template <>
Log& Log::operator<<<Log::endl>(const Log::endl& value)
{

	switch (m_type)
	{
		case LOG:
			if (!(m_flags & F_LOG))
				return *this;
		break;
		case DEBUG:
			if (!(m_flags & F_DEBUG))
				return *this;
		break;
		case WARNING:
			if (!(m_flags & F_WARNING))
				return *this;
		break;
		case SUCCESS:
			if (!(m_flags & F_SUCCESS))
				return *this;
		break;
		case ERROR:
			if (!(m_flags & F_ERROR))
				return *this;
		break;
		case ALERT:
			if (!(m_flags & F_ALERT))
				return *this;
		break;
	}

	(void)value;
	if (m_type == LOG || m_type == DEBUG || m_type == SUCCESS)
		std::cout << m_oss.str() << RESET << std::endl;
	else
		std::cerr << m_oss.str() << RESET << std::endl;
	return *this;
}

std::ostream& operator<<(std::ostream &stream, const Log::endl endl)
{
	(void)endl;
	return stream;
}

std::ostream& operator<<(std::ostream& stream, const Cgi_Type& type)
{
	switch (type)
	{
		case NONE:
			stream << "NONE";
			break;
		case PHP:
			stream << "PHP";
			break;
		default:
			stream << "unknow type in std::ostream& operator<<(std::ostream& stream, const Cgi_Type& type); " << __FILE__ << ":" << __LINE__;
		break;
	}
	return stream;
}
