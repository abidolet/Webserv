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
	m_stream.close();
	Log(Log::SUCCESS) << filepath << "is loaded!" << Log::endl();

#if 0
	Utils::printFile(m_file);
#endif

	parseBlock();
}

void Parser::parseBlock()
{
	std::vector<std::string>::iterator it = m_file.begin();
	m_block = loadBlock(it, "root");
}

std::string Parser::joinPath(Server& serv, const std::string &path, std::string locationName)
{
	Location* location = serv.searchLocationByName(Utils::findFileFolder(locationName));

	std::string to_add;
	if (location == NULL)
	{
		if (serv.root[serv.root.size() - 1] == '/')
			to_add = serv.root.substr(0, serv.root.size() - 1);
		else
			to_add = serv.root;
	}
	else
	{
		if (location->path[location->path.size() - 1] == '/')
			to_add = location->path.substr(0, location->path.size() - 1);
		else
			to_add = location->path;
	}
	return to_add + path;
}

Block Parser::loadBlock(std::vector<std::string>::iterator& it, const std::string &_name)
{
	Block block(_name);
	for ( ; it != m_file.end(); ++it)
	{
		if (it->find("{") != (size_t)-1)
		{
			if (it->find("{") != it->size() - 1)
				throw std::runtime_error("{ must be at end of line");
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

std::vector<std::string> setupAllowedMethods(Block& block)
{
	std::string	defaults[] = {"GET", "POST", "DELETE"};
	std::vector<std::string> result(defaults, defaults + 3);

	std::string found;
	block.loadSingleDirective("allowed_methods", found);
	if (found.empty())
		return result;

	const std::string& allowedMethods = found;
	result = Utils::strsplit(allowedMethods, ' ');

	// check if entry is correct
	std::vector<std::string>::iterator it = result.begin();
	while (it != result.end())
	{
		int i = 0;
		for ( ; i < 3; i++)
		{
			if (*it == defaults[i])
				break;
		}
		if (i == 3)
			throw Parser::InvalidArgumentException(*it, Utils::findClosest(*it, std::vector<std::string>(defaults, defaults + 3)));
		++it;
	}
	return result;
}

void populateServerSubInfos(Block serv_block, Server& serv)
{
	std::vector<Location> locations;

	const std::string locationOptions[] = {"index", "path", "allowed_methods", "cgi_pass", "return", "directory_listing", "upload_dir"};
	const std::string cookiesOptions[] = {"set"};
	const std::string names[] = { "location", "cookies" };

	serv_block.nameAssert(std::vector<std::string>(names, names + 2));

	std::vector<Block>::iterator it = serv_block.inners.begin();
	for ( ; it != serv_block.inners.end(); ++it)
	{

		if (it->getName() == "location")
		{
			it->dirAssert(std::vector<std::string>(locationOptions, locationOptions + 7));
			Location tmp(*it);
			tmp.allowed_methods = setupAllowedMethods(*it);
			locations.push_back(tmp);
		}
		if (it->getName() == "cookies")
		{
			it->dirAssert(std::vector<std::string>(cookiesOptions, cookiesOptions + 1));
			serv.cookies = it->loadDirectives("set");
			serv.cookiesAssert();
		}
	}
	serv.locations = locations;
}

void removeExcessListen(std::vector<Server>& servs)
{
	std::vector<Listen> pastListen;

	for (size_t i = 0; i < servs.size(); i++)
	{
		for (size_t j = 0; j < servs[i].listen.size(); j++)
		{
			size_t idx = find(pastListen, servs[i].listen[j]);
			if (idx != (size_t)-1)
			{
				servs[i].listen.erase(servs[i].listen.begin() + idx);
				Log(Log::WARNING) << "duplicate listen directive found, the duplicate will be ignored" << Log::endl();
			}
			else
			{
				pastListen.push_back(servs[i].listen[j]);
			}
		}
	}
}

std::vector<Server> Parser::populateServerInfos()
{
	std::vector<Server> servs;

	const std::string names[] = { "server" };
	const std::string options[] = {
		"listen", "host", "server_name", "root",
		"client_max_body_size", "allowed_methods", "error_page"
	};

	m_block.nameAssert(std::vector<std::string>(names, names + 1));
	Log(Log::SUCCESS) << "server block is good" << Log::endl();

	//
	// parsing server blocks
	//
	std::vector<Block>::iterator it = m_block.inners.begin();
	for ( ; it != m_block.inners.end(); ++it) // loop on each server block
	{
		it->dirAssert(std::vector<std::string>(options, options + 7));

		Server serv;

		serv.init(*it);
		serv.allowed_methods = setupAllowedMethods(*it);
		populateServerSubInfos(*it, serv); // will populate locations and cookies

		std::map<int, std::string>::iterator page = serv.error_pages.begin();
		for ( ; page != serv.error_pages.end(); ++page)
		{
			page->second = joinPath(serv, page->second, page->second);
		}

		if (serv.locations.size() == 0)
			serv.locations.push_back(Location());

		serv.runSelfCheck();
		servs.push_back(serv);
	}

	if (m_block.inners.size() == 0) // empty file
		servs.push_back(Server());

	removeExcessListen(servs);

	for (size_t i = 0; i < servs.size(); i++)
	{
		Utils::printServConfig(servs[i]);
	}


	Log(Log::SUCCESS) << m_filepath << " was parsed successfuly" << Log::endl();
	return servs;
}

