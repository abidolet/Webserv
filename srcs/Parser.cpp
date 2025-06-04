#include "Parser.hpp"
#include <iostream>
#include "Log.hpp"

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
	loadBlock();
}

void Parser::loadBlock()
{
	std::vector<std::string>::iterator it = m_file.begin();

	while (it != m_file.end())
	{
		if (it->find("{") != it->size())
			Log() << "found block start";
		// if (it->find("}"))
		// 	Log() << "found block end";
		Log(Log::DEBUG) << *it;	
		it++;
	}
}
