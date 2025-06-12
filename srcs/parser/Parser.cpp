#include "Parser.hpp"
#include "Log.hpp"
#include "Webserv.hpp"
#include "ParserUtils.hpp"

#include <cstdlib>
#include <list>

Parser::Parser(const std::string& filepath)
	: m_block("root"), m_filepath(filepath), default_config_path("./conf/default.conf")
{
	m_stream.open(filepath.c_str());
	if (!m_stream.good() || !m_stream.is_open())
		throw std::runtime_error("cannot open " + filepath);
	m_file = Utils::read_file(m_stream);
	Log(Log::SUCCESS) << filepath << "is loaded!" << Log::endl();
	
	parseBlock();
}

void Parser::parseBlock()
{
	std::vector<std::string>::iterator it = m_file.begin();
	m_block = loadBlock(it, "root");
}

Block Parser::loadBlock(std::vector<std::string>::iterator& it, const std::string &_name)
{
	Block block(_name);
	for ( ; it != m_file.end(); ++it)
	{
		if (it->find("{") != (size_t)-1)
		{
			std::string name = Utils::strtrim(*it, " \t{");
			++it;
			block.inners.push_back(loadBlock(it, name));
		}
		else if (it->find("}") != (size_t)-1)
		{
			return block;
		}
		else
		{
			if (block.block_name == "root")
				throw std::runtime_error("missing `{' on line `" + *it + "'");
			if (Utils::should_add_line(*it))
				block.directives.push_back(Utils::strtrim(*it, " \t"));
		}
	}
	if (block.block_name != "root")
		throw std::runtime_error("missing `}'");
	return block;
}

void setupAllowedMethods(Server& serv, Block& block)
{
	std::vector<std::string> found = block.loadDirectives("allowed_methods");
	if (found.empty())
		return;

	const std::string& allowedMethods = found[0];
	serv.allowed_methods = Utils::strsplit(allowedMethods, ' ');

	// check if entry is correct
	const std::string avaibleStrs[3] = {"POST", "GET", "DELETE"};
	std::vector<std::string>::iterator it = serv.allowed_methods.begin();
	while (it != serv.allowed_methods.end())
	{
		int i = 0;
		for ( ; i < 3; i++)
		{
			if (*it == avaibleStrs[i])
				break;
		}
		if (i == 3)
			throw Parser::InvalidArgumentException(*it, Utils::findClosest(*it, std::vector<std::string>(avaibleStrs, avaibleStrs + 3)));
		++it;
	}
}

std::vector<Location> populateLocationInfos(Block serv_block)
{
	std::vector<Location> locations;

	const std::string locationOptions[] = {"index", "path", "allowed_methods", "cgi_pass"};
	serv_block.blockAssert(std::vector<std::string>(locationOptions, locationOptions + 4), "location");

	std::vector<Block>::iterator it = serv_block.inners.begin();
	for ( ; it != serv_block.inners.end(); ++it)
	{
		Location tmp(*it);
		locations.push_back(tmp);
	}
	return locations;
}

Server Parser::populateServerInfos()
{
	Server serv;
	const std::string serverOption[] = {"listen", "host", "server_name", "return", "client_max_body_size", "allowed_methods", "error_page"};

	m_block.blockAssert(std::vector<std::string>(serverOption, serverOption + 7), "server");
	Log(Log::SUCCESS) << "server block is good" << Log::endl();

	//
	// parsing server blocks
	// TODO: this should return a vector of server
	//
	std::vector<Block>::iterator it = m_block.inners.begin();
	for ( ; it != m_block.inners.end(); ++it) // loop on each server block
	{
		serv.init(*it);
		setupAllowedMethods(serv, *it);
		serv.locations = populateLocationInfos(*it);
	}

	Utils::printServConfig(serv);
	std::cout << std::endl;

	Log(Log::SUCCESS) << m_filepath << " was parsed successfuly" << Log::endl();
	return serv;
}


//TODO check le parsing:
// [ ] mettre des blocs sans le `{'
// [ ] mettre des str la ou y'a besoin de nombre
