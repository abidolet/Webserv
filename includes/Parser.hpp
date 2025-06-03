#ifndef PARSER_HPP
#define PARSER_HPP

#include <string>
#include <vector>
#include <fstream>

struct Server;

#define DEFAULT_CONF_PATH "./conf/default.conf"

struct Block
{
	std::string block_name;
	std::vector<std::string>::iterator begin_it; 
	std::vector<std::string>::iterator end_it; 
};

class Parser
{
public:
	Parser(const std::string& filepath);
	~Parser() {};


	void	loadBlock();
	void	loadFile();

private:
	std::vector<std::string>	m_file;
	std::vector<Block>			m_blocks;

	std::ifstream m_stream;	

	std::string default_config_path;
};

#endif // !PARSER_HPP
