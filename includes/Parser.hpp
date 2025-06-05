#ifndef PARSER_HPP
#define PARSER_HPP

#include <string>
#include <vector>
#include <fstream>

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
	std::string					default_config_path;
};

#endif // !PARSER_HPP
