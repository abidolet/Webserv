#include "Webserv.hpp"
#include "Block.hpp"
#include "ParserUtils.hpp"
#include "Parser.hpp"

void assertLocation(const Location& location, const Block& block)
{
	if (location.path[0] != '/')
		throw std::runtime_error("`path' directive must be an absolute path");

	if (!location.cgi_pass.empty() && Utils::fileAccess(location.cgi_pass) == false)
		throw std::runtime_error("cannot open cgi pass: `" + location.cgi_pass + "'");

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
	block.loadSingleDirective("upload_dir", upload_dir);

	if (!cgi_extension.empty())
		is_cgi = true;

	path = Utils::processPath(path);
	root = is_cgi ? Utils::processPath(root, true) : Utils::processPath(root);
	cgi_pass = Utils::processPath(cgi_pass);
	upload_dir = Utils::processPath(upload_dir);
	index = Utils::processPath(index, true);

	{
		std::string tmp;
		block.loadSingleDirective("return", tmp);
		if (!tmp.empty())
		{
			std::vector<std::string> split = Utils::strsplit(tmp, ' ');
			if (split.size() != 2 || split[0] != "301")
				throw std::runtime_error("redirection error");
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

const Location*	getLocation(const std::string& path, const Server& server)
{
	Log(Log::DEBUG) << "Checking location for:" << path << Log::endl();
	const Location*	best_match = NULL;
	size_t	best_match_length = 0;

	for (size_t	i = 0; i < server.locations.size(); ++i)
	{
		const Location&	loc = server.locations[i];
		Log(Log::DEBUG) << "Checking location " << i << ": " << loc.root << Log::endl();

		if (path == loc.root
			|| (path.compare(0, loc.root.length(), loc.root) == 0
			&& (loc.root[loc.root.length() - 1] == '/'
				|| path[loc.root.length()] == '/'
				|| path[loc.root.length()] == '\0')))
		{
			Log(Log::DEBUG) << "Valid match found: " << loc.root << Log::endl();

			if (loc.root.length() > best_match_length)
			{
				best_match = &loc;
				best_match_length = loc.root.length();
				Log(Log::DEBUG) << "New best match: " << best_match->root << Log::endl();
			}
		}
	}
	return best_match;
}

