#include "Parser.hpp"
#include "Log.hpp"
#include "Webserv.hpp"

#include <math.h>

bool	should_add_line(std::string line)
{
	for (size_t i = 0; i < line.size(); i++)
	{
		if (line[i] != ' ' && line[i] != '\t')
			return true;
	}
	return false;
}

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

void print_block(Block block, int depth)
{
	std::vector<std::string>::iterator it = block.content.begin();

	Log(Log::DEBUG) << "block name:" << block.block_name << Log::endl();
	while (it != block.content.end())
	{
		Log(Log::LOG) << "depth:" << depth << *it << Log::endl();
		it++;
	}

	for (size_t i = 0; i < block.inners.size(); i++)
	{
		std::cout << std::endl;
		print_block(block.inners[i], depth + 1);
	}
	
}

Parser::Parser(const std::string& filepath)
	: m_block("root"), default_config_path("./conf/default.conf")
{
	m_stream.open(filepath.c_str());
	if (!m_stream.good() || !m_stream.is_open())
		throw std::runtime_error("cannot open " + filepath);
	m_file = read_file(m_stream);
	Log(Log::SUCCESS) << filepath << " is loaded!" << Log::endl();
	
	parseBlock();

	Log(Log::SUCCESS) << filepath << " was parsed successfuly" << Log::endl();

}

void Parser::parseBlock()
{
	std::vector<std::string>::iterator it = m_file.begin();
	m_block = loadBlock(it, "root");
}

std::string strtrim(std::string str, const std::string set)
{
	str = str.substr(str.find_first_not_of(set));
	str = str.substr(0, str.find_last_not_of(set) + 1);
	return str;
}

Block Parser::loadBlock(std::vector<std::string>::iterator& it, std::string name)
{
	Block block(name);

	while (it != m_file.end())
	{
		if (it->find("{") != (size_t)-1)
		{
			Log(Log::DEBUG) << *it << Log::endl();
			std::string name = strtrim(*it, " \t{");
			it++;
			block.inners.push_back(loadBlock(it, name));
		}
		else if (it->find("}") != (size_t)-1)
		{
			Log(Log::DEBUG) << *it << Log::endl();
			return block;
		}
		else
		{
			if (should_add_line(*it))
				block.content.push_back(strtrim(*it, " \t"));
			Log(Log::DEBUG) << *it << Log::endl();
		}
		it++;
	}
	if (block.block_name != "root")
		throw std::runtime_error("missing `}'");
	return block;
}

int getStrValue(std::string str)
{
	int		value = 0;

	for (size_t i = 0; i < str.size(); i++)
	{
		value += str[i];
	}
	return value;
}

std::string findClosest(std::string str, std::vector<std::string> options)
{
	int idx = 0;
	int	str_val = getStrValue(str);

	for (size_t i = 0; i < options.size(); i++)
	{
		if (std::abs(getStrValue(options[i]) - str_val) < std::abs(getStrValue(options[idx]) - str_val))
		{
			idx = i;
		}
	}
	return options[idx];
}

bool blockAssert(std::vector<Block> blocks, std::vector<std::string> shouldBeFound, std::string blockName)
{
	std::vector<Block>::iterator block = blocks.begin();
	while (block != blocks.end())
	{
		if (block->block_name != blockName)
			throw std::runtime_error("invalid identifier: `" + block->block_name + "'; did you mean: `" + blockName + "' ?");

		std::vector<std::string>::iterator it = block->content.begin();
		std::string	firstWord;
		while (it != block->content.end())
		{
			firstWord = it->substr(0, it->find(" "));

			size_t i = 0;
			for ( ; i < shouldBeFound.size() ; i++)
			{
				if (firstWord == shouldBeFound[i])
					break;
			}
			if (i == shouldBeFound.size())
				throw std::runtime_error("invalid identifier: `" + firstWord + "'; did you mean: `" + findClosest(firstWord, shouldBeFound) + "' ?");

			it++;
		}

	}

	return true;
}

template <typename T>
T fillEntry(T* entry, std::string keyword, Block block)
{
	T result;
	std::stringstream ss;

	std::vector<std::string>::iterator it = block.content.begin();
	std::string	firstWord;
	while (it != block.content.end())
	{
		firstWord = it->substr(0, it->find(" "));

		if (firstWord == keyword)
		{
			ss << it->substr(it->find(" ") + 1, it->size() - it->find(" ") + 1);
			ss >> result;
			*entry = result;
			return result;
		}

		it++;
	}
	return *entry;
}

void printServConfig(Server serv)
{
	Log(Log::DEBUG) << "server config:" << "\n"
	<< GRAY << "\t\t\tserver_name" << serv.server_name << "\n"
	<< GRAY << "\t\t\thost" << serv.host << "\n"
	<< GRAY << "\t\t\tport" << serv.port << "\n"
	<< GRAY << "\t\t\tfd" << serv.fd << "\n"
	<< GRAY << "\t\t\tclient_max_body_size" << serv.client_max_body_size << "\n"
	<< GRAY << "\t\t\tis_default" << serv.is_default << Log::endl();
}

std::vector<Location> populateLocationInfos(Block serv_block)
{
	std::vector<Location> locations;

	(void)serv_block;
#ifdef TODO
	const std::string locationOptions[] = {"index", "allowed_methodes",};
	blockAssert(serv_block.inners, std::vector<std::string>(locationOptions, locationOptions + 4), "location");

	std::vector<Block>::iterator it = serv_block.inners.begin();
#endif
	return locations;
}


Server Parser::populateServerInfos()
{
	Server serv;
	const std::string serverOption[] = {"listen", "server_name", "return", "client_max_body_size"};

	blockAssert(m_block.inners, std::vector<std::string>(serverOption, serverOption + 4), "server"); 
	std::vector<Block>::iterator it = m_block.inners.begin();
	//
	// parsing server blocks
	//
	while (it != m_block.inners.end())
	{
		fillEntry(&serv.server_name, "server_name", *it);
		fillEntry(&serv.host, "listen", *it);
		fillEntry(&serv.port, "listen", *it);
		fillEntry(&serv.client_max_body_size, "client_max_body_size", *it); // TODO: body size peut etre 10M <= 10 megabytes et ca c'est pas gerer 
		it++;

		//
		// parsing locations
		//
		serv.locations = populateLocationInfos(*it); 
	}

	Log(Log::SUCCESS) << "server block is good" << Log::endl();
	printServConfig(serv);
	std::cout << std::endl;
	return serv;
}


