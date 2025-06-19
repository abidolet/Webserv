#include "Webserv.hpp"
#include "Block.hpp"
#include "ParserUtils.hpp"
#include "Parser.hpp"

void assertLocation(const Location& location, const Block& block)
{
	if (location.path[0] != '/')
		throw std::runtime_error("`path' directive must be an absolute path");

	// if (!location.cgi_pass.empty() && Utils::fileAccess(location.cgi_pass) == false)
	// 	throw std::runtime_error("cannot open cgi pass: `" + location.cgi_pass + "'");

	//? check that cgi_extension and cgi_pass are BOTH empty or filled
	if (!location.cgi_extension.empty() != !location.cgi_pass.empty())
		throw std::runtime_error("cannot have only one cgi directive; " + block.block_name);
}

Location::Location(Block &block)
	:	path("/"), root("/"), index(""),
		is_cgi(false), directoryListing(true), redirection(-1, "")
{
	setupLocationRoot(block);
	block.loadSingleDirective("path", path);
	block.loadSingleDirective("index", index);
	block.loadSingleDirective("cgi_pass", cgi_pass);

	// TODO faire une rediretion assertion
	{
		std::string tmp;
		block.loadSingleDirective("return", tmp);
		if (!tmp.empty())
		{
			std::vector<std::string> split = Utils::strsplit(tmp, ' ');
			redirection.first = std::atoi(split[0].c_str());
			redirection.second = split[1];
		}
	}

	{
		std::string tmp;
		block.loadSingleDirective("directory_listing", tmp);
		if (tmp == "on")
			directoryListing = true;
		else if (tmp == "off")
			directoryListing = false;
		else if (!tmp.empty())
			throw Parser::InvalidDirectiveException("directory_listing", tmp);
	}

	assertLocation(*this, block);
	if (!cgi_extension.empty())
		is_cgi = true;
}

void Location::setupLocationRoot(const Block& block)
{
	std::vector<std::string> split = Utils::strsplit(block.block_name, ' ');
	if (split.size() == 1)
		throw Parser::InvalidDirectiveException("location", block.block_name);

	root = split[1];
	if (split[1] == "~") // for cgi
	{
		if (split.size() != 3)
			throw std::runtime_error("invalid option in `" + block.block_name + "'");

		const std::string options[] = {".php", ".bla" ,".py"};
		for (size_t i = 0; i < options->size(); i++)
		{
			if (split[2] == options[i])
			{
				Log() << "cgi extension found:" << options[i] << Log::endl();
				cgi_extension = options[i];
				break;
			}
		}
		if (cgi_extension.empty())
			throw Parser::InvalidArgumentException(split[2], Utils::findClosest(split[2], std::vector<std::string>(options, options + 1)));
		root = split[2];
	}
}
