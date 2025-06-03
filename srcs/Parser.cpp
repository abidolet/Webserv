#include "Parser.hpp"
#include <iostream>

std::vector<std::string> read_file(std::ifstream& stream)
{
	std::vector<std::string> file;
	std::string	line;

	while (std::getline(stream, line))
	{
		file.push_back(line);
	}

	return file;
}

Parser::Parser(const std::string& filepath)
	: default_config_path("./conf/default.conf")
{
	m_stream.open(filepath.c_str());
	if (!m_stream.good() || !m_stream.is_open())
		throw std::runtime_error("cannot open " + filepath);

	m_file = read_file(m_stream); 

	for (size_t i = 0; i < m_file.size(); i++)
	{
		std::cout << m_file[i] << std::endl;
	}
}

// std::vector<std::string> read_block()
// {
//
// }
