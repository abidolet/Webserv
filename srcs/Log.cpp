#include "Log.hpp"
#include "Webserv.hpp"

#include <ctime>
#include <fstream>
#include <iomanip>
#include <iostream>

uint16_t Log::m_flags = F_LOG | F_DEBUG | F_WARNING | F_SUCCESS | F_ERROR | F_ALERT;
std::ofstream Log::m_logFile;

void Log::displayTimestamp(void)
{
    std::time_t time = std::time(NULL);
	std::tm		*tm = std::localtime(&time);

	int	h = tm->tm_hour;
	int	min = tm->tm_min;
	int	s = tm->tm_sec;
	m_oss << printCol(GRAY) << "["
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

void Log::setupLogFile()
{
	std::stringstream ss;

	std::time_t time = std::time(NULL);
	std::tm		*tm = std::localtime(&time);

	ss << "log_webserv_" << tm->tm_hour << tm->tm_min << tm->tm_sec << ".txt";
	m_logFile.open(ss.str().c_str());
	if (!m_logFile.is_open())
		throw std::runtime_error("cannot open `" + ss.str() + "'");
}


Log::Log(Log::Type type)
{
	m_type = type;
	displayTimestamp();
	switch (type)
	{
		case LOG:
			m_oss << printCol(GRAY) << "[LOG]     " << printCol(RESET);
			break;
		case WARNING:
			m_oss << printCol(YELLOW) << "[WARNING] " << printCol(RESET);
			break;
		case ERROR:
			m_oss << printCol(RED) << "[ERROR]   " << printCol(RESET);
			break;
		case ALERT:
			m_oss << printCol(B_RED) << "[ALERT]   " << printCol(RESET);
			break;
		case DEBUG:
			m_oss << printCol(PURPLE) << "[DEBUG]   " << printCol(RESET);
			break;
		case SUCCESS:
			m_oss << printCol(GREEN) << "[SUCCESS] " << printCol(RESET);

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
	if (m_logFile.is_open())
		m_logFile << m_oss.str() << std::endl;
	else if (m_type == LOG || m_type == DEBUG || m_type == SUCCESS)
		std::cout << m_oss.str() << printCol(RESET) << std::endl;
	else
		std::cerr << m_oss.str() << printCol(RESET) << std::endl;
	return *this;
}

std::string Log::printCol(const std::string& col)
{
	return m_logFile.good() ? "" : col;
}

std::ostream& operator<<(std::ostream &stream, const Log::endl endl)
{
	(void)endl;
	return stream;
}

