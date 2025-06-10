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
#include <stdint.h>
#include <iostream>

#define F_LOG		0b000001
#define F_DEBUG		0b000010
#define F_WARNING	0b000100
#define F_SUCCESS	0b001000
#define F_ERROR		0b010000
#define F_ALERT		0b100000

#define GET_VAR_NAME(name) #name

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
		SUCCESS,
		ERROR,
		ALERT,
	};

	class endl
	{

	};

private:
	std::ostringstream	m_oss;
	Type				m_type;
	static uint16_t		m_flags;

public:

	Log(Log::Type = Log::LOG);
	void displayTimestamp(void);

	static void setFlags(uint16_t flags);
	static void enableFlags(uint16_t flags);
	static void disableFlags(uint16_t flags);
	static void toggleFlags(uint16_t flags);

	template <typename T>
	Log& operator<<(const T& value)
	{
		m_oss << value << " ";
		return *this;
	}

	template <>
	Log& operator<<<Log::endl>(const Log::endl& value)
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

};

#endif
