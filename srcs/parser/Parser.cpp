#include "Parser.hpp"
#include "Log.hpp"
#include "Webserv.hpp"

#include "ParserTools.hpp"

#include <cstdlib>

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
	m_file = Tools::read_file(m_stream);
	Log(Log::SUCCESS) << filepath << " is loaded!" << Log::endl();
	
	parseBlock();

	Log(Log::SUCCESS) << filepath << " was parsed successfuly" << Log::endl();

}

void Parser::parseBlock()
{
	std::vector<std::string>::iterator it = m_file.begin();
	m_block = loadBlock(it, "root");
}



Block Parser::loadBlock(std::vector<std::string>::iterator& it, std::string name)
{
	Block block(name);

	while (it != m_file.end())
	{
		if (it->find("{") != (size_t)-1)
		{
			Log(Log::DEBUG) << *it << Log::endl();
			std::string name = Tools::strtrim(*it, " \t{");
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
			if (Tools::should_add_line(*it))
				block.content.push_back(Tools::strtrim(*it, " \t"));
			Log(Log::DEBUG) << *it << Log::endl();
		}
		it++;
	}
	if (block.block_name != "root")
		throw std::runtime_error("missing `}'");
	return block;
}



bool blockAssert(std::vector<Block> blocks, std::vector<std::string> shouldBeFound, std::string blockName)
{
	std::vector<Block>::iterator block = blocks.begin();
	while (block != blocks.end())
	{
		if (block->block_name.substr(0, block->block_name.find(" ")) != blockName)
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
				throw std::runtime_error("invalid identifier: " + firstWord + "'; did you mean: `" + Tools::findClosest(firstWord, shouldBeFound) + "' ?");

			it++;
		}
		block++;
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

template <>
std::string fillEntry<std::string>(std::string* entry, std::string keyword, Block block)
{
	std::string result;
	std::stringstream ss;

	std::vector<std::string>::iterator it = block.content.begin();
	std::string	firstWord;
	while (it != block.content.end())
	{
		firstWord = it->substr(0, it->find(" "));

		if (firstWord == keyword)
		{
			ss << it->substr(it->find(" ") + 1, it->size() - it->find(" ") + 1);
			*entry = ss.str();
			return *entry;
		}

		it++;
	}
	return *entry;
}

void printServConfig(Server serv)
{
	Log(Log::DEBUG) << "server config:" << Log::endl();
	Log(Log::DEBUG) << GRAY << "\t|-> server_name:" << serv.server_name << Log::endl();
	Log(Log::DEBUG) << GRAY << "\t|-> host:" << serv.host << Log::endl();
	Log(Log::DEBUG) << GRAY << "\t|-> port:" << serv.port << Log::endl();
	Log(Log::DEBUG) << GRAY << "\t|-> client_max_body_size:" << serv.client_max_body_size << Log::endl();
	Log(Log::DEBUG) << GRAY << "\t|-> allowed methodes:" << Tools::strUnite(serv.allowed_methodes, ",") << Log::endl();
	Log(Log::DEBUG) << GRAY << "\t|-> redirection:" << serv.redirection.first << serv.redirection.second << Log::endl();
	Log(Log::DEBUG) << GRAY << "\t|-> is_default:" << serv.is_default << Log::endl();
	Log(Log::DEBUG) << GRAY << "\t|-- locations:" << serv.locations.size() << Log::endl();

	std::vector<Location>::iterator it = serv.locations.begin();
	while (it != serv.locations.end())
	{
		Log(Log::DEBUG) << GRAY << "\t\t|-> path:" << it->path << Log::endl();
		Log(Log::DEBUG) << GRAY << "\t\t|-> root:" << it->root << Log::endl();
		Log(Log::DEBUG) << GRAY << "\t\t|-> index:" << it->index << Log::endl();
		Log(Log::DEBUG) << GRAY << "\t\t|-> cgi pass:" << it->cgi_pass << Log::endl();
		Log(Log::DEBUG) << GRAY << "\t\t|-> cgi extansions:" << Tools::strUnite(it->cgi_extensions, ",") << Log::endl();
		it++;
	}
}

std::vector<Location> populateLocationInfos(Block serv_block)
{
	std::vector<Location> locations;

	const std::string locationOptions[] = {"index", "root"};
	blockAssert(serv_block.inners, std::vector<std::string>(locationOptions, locationOptions + 2), "location");

	std::vector<Block>::iterator it = serv_block.inners.begin();
	while (it != serv_block.inners.end())
	{
		Location tmp;

		fillEntry(&tmp.path, "path", *it);

		locations.push_back(tmp);
		it++;
	}
	return locations;
}


Server Parser::populateServerInfos()
{
	Server serv;
	const std::string serverOption[] = {"listen", "host", "server_name", "return", "client_max_body_size", "allowed_methodes", "error_page"};

	blockAssert(m_block.inners, std::vector<std::string>(serverOption, serverOption + 7), "server"); 
	std::vector<Block>::iterator it = m_block.inners.begin();

	//
	// parsing server blocks
	//
	while (it != m_block.inners.end())
	{
		fillEntry(&serv.server_name, "server_name", *it);

		// server host:port //
		std::string listen = "0.0.0.0:8080";
		fillEntry(&listen, "listen", *it);
		std::vector<std::string> split = Tools::strsplit(listen, ':');
		if (split.size() > 2)
			throw std::runtime_error("invalid expression in `listen'");
		if (split.size() == 1)
			serv.port = std::atoi(split[0].c_str());
		else
		{
			serv.host = split[0];	
			serv.port = std::atoi(split[1].c_str());
		}
		if (serv.port < 0)
			throw std::runtime_error("invalid port");
		// ============
		
		
		//
		// max body size
		//
		std::string max_size = "0";
		char* end;
		fillEntry(&max_size, "client_max_body_size", *it); 
		float tmp = std::strtod(max_size.c_str(), &end);
		if (tmp < 0)
			throw std::runtime_error("invalid client_max_body_size: `" + max_size + "'");
		if (end[0] != '\0')
		{
			if ((end[0] == 'm' || end[0] == 'M') && end[1] == '\0')
				serv.client_max_body_size *= tmp * 1000000;
			else
				throw std::runtime_error("invalid client_max_body_size: `" + max_size + "'");
		}
		else
			serv.client_max_body_size = tmp;
		
		std::string allowedMethodes = "POST GET DELETE";
		fillEntry(&allowedMethodes, "allowed_methodes", *it);
		serv.allowed_methodes = Tools::strsplit(allowedMethodes, ' ');
		//
		// parsing locations
		//
		serv.locations = populateLocationInfos(*it); 
		it++;
	}

	Log(Log::SUCCESS) << "server block is good" << Log::endl();
	printServConfig(serv);
	std::cout << std::endl;
	return serv;
}

//TODO parsing
// [x] servername 
// [x] host
// [x] port
// [x] client_max_body_size
// [~] allowed_methodes
// [ ] redirection
// [~] location
// [ ]  | path
// [ ]  | root
// [ ]  | index
// [ ]  | cgi pass
// [ ]  | cgi extension
