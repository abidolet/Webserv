#ifndef PARSER_HPP
#define PARSER_HPP

#include <string>
#include <vector>
#include <fstream>
#include <exception>

struct Server;

#define DEFAULT_CONF_PATH "./conf/default.conf"

class Block
{
public:
	Block(std::string name) : block_name(name) {}


	std::string block_name;

	std::vector<std::string>	content;
	std::vector<Block>			inners;
};

class Parser
{
public:
	Parser(const std::string& filepath);
	~Parser() {};

	Server	populateServerInfos();

private:
	Block	loadBlock(std::vector<std::string>::iterator& it, std::string name);
	void	parseBlock();
	void	loadFile();

private:
	std::vector<std::string>	m_file;
	Block						m_block;
	std::ifstream				m_stream;	
	std::string					m_filepath;
	std::string					default_config_path;


public:
	class InvalidArgumentException : public std::exception
	{
	public:
		const char* what() const throw()
		{
			static std::string error = "invalid identifier: `" + m_wrong + "'; did you mean: `" + m_right + "' ?";
			return error.c_str();
		}

		virtual ~InvalidArgumentException() throw() {}
		InvalidArgumentException(std::string wrong, std::string right) : m_wrong(wrong), m_right(right) {}

	private:
		std::string m_wrong;
		std::string m_right;
	};

	class InvalidDirectiveException : public std::exception
	{
	public:
		const char* what() const throw()
		{
			static std::string error = "invalid `" + m_directive + "' directive; `" + m_line + "'";
			return error.c_str();
		}

		virtual ~InvalidDirectiveException() throw() {}
		InvalidDirectiveException(std::string directive, std::string line) : m_directive(directive), m_line(line) {}

	private:
		std::string m_directive;
		std::string m_line;
	};

	class TooMuchDirectiveException : public std::exception
	{
	public:
		const char* what() const throw()
		{
			static std::string error = "too much `" + m_directive + "' directive in `" + m_block.block_name + "'";
			return error.c_str();
		}

		virtual ~TooMuchDirectiveException() throw() {}
		TooMuchDirectiveException(std::string directive, Block block) : m_directive(directive), m_block(block) {}

	private:
		std::string m_directive;
		Block m_block;
	};
};

#endif // !PARSER_HPP
