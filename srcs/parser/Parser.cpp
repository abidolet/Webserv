#include "Parser.hpp"
#include "Log.hpp"
#include "Webserv.hpp"
#include "ParserTools.hpp"

#include <cstdlib>
#include <list>

void print_block(Block block, int depth)
{
	std::vector<std::string>::iterator it = block.directives.begin();

	Log(Log::DEBUG) << "block name:" << block.block_name << Log::endl();
	while (it != block.directives.end())
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
	: m_block("root"), m_filepath(filepath), default_config_path("./conf/default.conf")
{
	m_stream.open(filepath.c_str());
	if (!m_stream.good() || !m_stream.is_open())
		throw std::runtime_error("cannot open " + filepath);
	m_file = Tools::read_file(m_stream);
	Log(Log::SUCCESS) << filepath << "is loaded!" << Log::endl();
	
	parseBlock();
}

void Parser::parseBlock()
{
	std::vector<std::string>::iterator it = m_file.begin();
	m_block = loadBlock(it, "root");
}

Block Parser::loadBlock(std::vector<std::string>::iterator& it, std::string _name)
{
	Block block(_name);
	while (it != m_file.end())
	{
		if (it->find("{") != (size_t)-1)
		{
			Log(Log::DEBUG) << *it << Log::endl();
			std::string name = Tools::strtrim(*it, " \t{");
			++it;
			block.inners.push_back(loadBlock(it, name));
		}
		else if (it->find("}") != (size_t)-1)
		{
			Log(Log::DEBUG) << *it << Log::endl();
			return block;
		}
		else
		{
			if (block.block_name == "root")
				throw std::runtime_error("missing `{' on line `" + *it + "'");
			if (Tools::should_add_line(*it))
				block.directives.push_back(Tools::strtrim(*it, " \t"));
			Log(Log::DEBUG) << *it << Log::endl();
		}
		it++;
	}
	if (block.block_name != "root")
		throw std::runtime_error("missing `}'");
	return block;
}

void printServConfig(Server serv)
{
	Log(Log::DEBUG) << "server config:" << Log::endl();
	Log(Log::DEBUG) << "\t|-> server_name:" << serv.server_name << Log::endl();
	Log(Log::DEBUG) << "\t|-> client_max_body_size:" << serv.client_max_body_size << Log::endl();
	Log(Log::DEBUG) << "\t|-> allowed methods:" << Tools::strUnite(serv.allowed_methods, ",") << Log::endl();

	// printing redirections
	{
		Log(Log::DEBUG) << "\t|->" << serv.error_pages.size() << "error_pages" << Log::endl();
		std::map<int, std::string>::iterator it = serv.error_pages.begin();
		while (it != serv.error_pages.end())
		{
			Log(Log::DEBUG) << "\t|\t|-> {" << it->first << "=>" << it->second  << "}" << Log::endl();
			it++;
		}
	}

	// printing listens
	{
		Log(Log::DEBUG) << "\t|->" << serv.listen.size() << "listens" << Log::endl();
		std::map<std::string, int>::iterator it = serv.listen.begin();
		while (it != serv.listen.end())
		{
			Log(Log::DEBUG) << "\t|\t|-> {" << it->first << "=>" << it->second  << "}" << Log::endl();
			++it;
		}
	}

	// printing each location
	{
		Log(Log::DEBUG) << "\t|" << Log::endl();
		Log(Log::DEBUG) << "\t|->" << serv.locations.size() << "locations" << Log::endl();
		std::vector<Location>::iterator it = serv.locations.begin();
		while (it != serv.locations.end())
		{
			Log(Log::DEBUG) << "\t|---- locations:" << it->root << Log::endl();
			Log(Log::DEBUG) << "\t|\t|-> path:" << it->path << Log::endl();
			Log(Log::DEBUG) << "\t|\t|-> root:" << it->root << Log::endl();
			Log(Log::DEBUG) << "\t|\t|-> index:" << it->index << Log::endl();
			Log(Log::DEBUG) << "\t|\t|-> type:" << it->type << Log::endl();

			Log(Log::DEBUG) << "\t|\t|-> cgi pass:" << it->cgi_pass << Log::endl();
			Log(Log::DEBUG) << "\t|\t|-> cgi extension:" << it->cgi_extension << Log::endl();

			it++;
		}
	}
}

void setupLocationPath(Location& location, Block& block)
{
	block.loadSingleDirective("path", location.path);

	if (location.path[0] != '/')
		throw std::runtime_error("`path' directive must be an absolute path");
}

std::vector<Location> populateLocationInfos(Block serv_block)
{
	std::vector<Location> locations;

	const std::string locationOptions[] = {"index", "path"};
	serv_block.blockAssert(std::vector<std::string>(locationOptions, locationOptions + 2), "location");

	std::vector<Block>::iterator it = serv_block.inners.begin();
	while (it != serv_block.inners.end())
	{
		Location tmp;

		// root
		std::vector<std::string> split = Tools::strsplit(it->block_name, ' ');
		if (split.size() == 1)
			throw Parser::InvalidDirectiveException("location", it->block_name);

		tmp.root = split[1];
		if (split[1] == "~") // for cgi
		{
			Log() << "found cgi location block" << Log::endl();
			if (split.size() != 3)
				throw std::runtime_error("invalid option in `" + it->block_name + "'");
			if (split[2] == ".php")
			{
				Log() << "found cgi type: `php'";
				tmp.type = PHP;
			}
			else
				throw Parser::InvalidArgumentException(split[1], ".php");
			tmp.root = split[2];
		}
		// ---

		setupLocationPath(tmp, *it);
		it->loadSingleDirective("index", tmp.index);

		locations.push_back(tmp);
		++it;
	}
	return locations;
}

// TODO check for format error in the listen directive (e.g: az.by.cvwc.fg323:3243233)
void	setupListen(Server& serv, Block &block)
{
	std::map<std::string, int>	result;

	std::vector<std::string> found = block.loadDirectives("listen");
	if (found.empty())
		return;

	std::vector<std::string>::iterator it = found.begin();
	for ( ; it != found.end(); ++it)
	{
		char *endl;
		int port;

		std::vector<std::string> split = Tools::strsplit(*it, ':');
		if (split.size() == 1)
		{
			port = std::strtol(split[0].c_str(), &endl, 10);
			if (endl[0] != '\0')
				throw Parser::InvalidDirectiveException("listen", *it);
			result.insert(std::pair<std::string, int>("0.0.0.0", port));
		}
		else
		{
			if (split.empty() || split.size() > 2)
				throw Parser::InvalidDirectiveException("listen", *it);

			port = std::strtol(split[1].c_str(), &endl, 10);
			if (endl[0] != '\0')
				throw Parser::InvalidDirectiveException("listen", *it);

			result.insert(std::pair<std::string, int>(split[0], port));
		}

	}
	serv.listen = result;
}

void setupMaxBodySize(Server& serv, Block& block)
{
	std::vector<std::string> found = block.loadDirectives("client_max_body_size");
	if (found.empty())
		return;

	char* end;
	std::string max_size = found[0];

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
}

void setupAllowedMethods(Server& serv, Block& block)
{
	std::vector<std::string> found = block.loadDirectives("allowed_methods");
	if (found.empty())
		return;

	std::string allowedMethods = found[0];
	serv.allowed_methods = Tools::strsplit(allowedMethods, ' ');

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
			throw Parser::InvalidArgumentException(*it, Tools::findClosest(*it, std::vector<std::string>(avaibleStrs, avaibleStrs + 3)));
		++it;
	}
}

void setupRedirections(Server& serv, Block& block)
{
	std::vector<std::string> found = block.loadDirectives("error_page");
	if (found.empty())
		return;

	for (size_t j = 0; j < found.size(); j++)
	{
		std::string str = found[j];
		std::vector<std::string> redirs = Tools::strsplit(str, ' ');
		if (redirs.size() <= 1)
			throw std::runtime_error("invalid directive: `error_page'");

		// isolating the error page
		std::string error_page = redirs[redirs.size() - 1];
		redirs.pop_back();

		if (	Tools::is_number(redirs) == false)
			if (error_page[0] != '/')
				throw std::runtime_error("invalid page path: `error_page'");

		for (size_t i = 0; i < redirs.size(); i++)
		{
			int code = std::atoi(redirs[i].c_str());
			if (serv.error_pages.find(code) != serv.error_pages.end())
				serv.error_pages.find(code)->second = error_page;
			else
				serv.error_pages.insert(std::pair<int, std::string>(code, error_page));
		}
	}
}

Server Parser::populateServerInfos()
{
	Server serv;
	const std::string serverOption[] = {"listen", "host", "server_name", "return", "client_max_body_size", "allowed_methods", "error_page"};

	m_block.blockAssert(std::vector<std::string>(serverOption, serverOption + 7), "server");
	Log(Log::SUCCESS) << "server block is good" << Log::endl();
	std::vector<Block>::iterator it = m_block.inners.begin();

	//
	// parsing server blocks
	//
	while (it != m_block.inners.end())
	{
		it->loadSingleDirective("server_name", serv.server_name);
		setupListen(serv, *it);
		setupMaxBodySize(serv, *it);
		setupAllowedMethods(serv, *it);
		setupRedirections(serv, *it);

		serv.locations = populateLocationInfos(*it);
		it++;
	}

	printServConfig(serv);
	std::cout << std::endl;

	Log(Log::SUCCESS) << m_filepath << " was parsed successfuly" << Log::endl();
	return serv;
}

//TODO parsing
// [x] servername 
// [x] host
// [x] port
// [x] client_max_body_size
// [x] allowed_methods
// [x] redirection
// [~] location
// [x]  | path
// [x]  | root
// [x]  | index
// [ ]  | cgi pass
// [ ]  | cgi extension
//

//TODO check le parsing:
// [ ] mettre des blocs sans le `{'
// [ ] mettre des str la ou y'a besoin de nombre
