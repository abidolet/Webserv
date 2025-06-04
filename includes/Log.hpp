#ifndef LOG_HPP
# define LOG_HPP

# define RESET		"\033[0m"
# define BLACK		"\033[0;30m"
# define RED		"\033[0;31m"
# define GREEN		"\033[0;32m"
# define YELLOW		"\033[0;33m"
# define BLUE		"\033[0;34m"
# define PURPLE		"\033[0;35m"
# define CYAN		"\033[0;36m"
# define WHITE		"\033[0;37m"
# define GRAY		"\033[90m"
# define BOLD		"\033[1m"

# define B_BLACK	"\033[40m"
# define B_RED		"\033[41m"
# define B_GREEN	"\033[42m"
# define B_YELLOW	"\033[43m"
# define B_BLUE		"\033[44m"
# define B_PURPLE	"\033[45m"
# define B_CYAN		"\033[46m"
# define B_WHITE	"\033[47m"

#include <sstream>
#include <iostream>

class Log
{
public:
	//
	// Internal class
	//
	enum Type
	{
		LOG = 0,
		DEBUG,
		WARNING,
		ERROR,
		ALERT,
	};

	class endl
	{

	};

private:
	std::ostringstream	m_oss;
	Type				m_type;

public:
	Log(Log::Type = Log::LOG);
	void displayTimestamp(void);


	template <typename T>
	Log& operator<<(const T& value)
	{
		m_oss << value << " ";
		return *this;
	}

	template <>
	Log& operator<<<Log::endl>(const Log::endl& value)
	{
		(void)value;
		if (m_type == LOG || m_type == DEBUG)
			std::cout << m_oss.str() << RESET << std::endl;
		else
			std::cerr << m_oss.str() << RESET << std::endl;
		return *this;
	}

};

#endif
