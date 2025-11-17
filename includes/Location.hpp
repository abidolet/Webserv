#ifndef LOCATION_HPP
#define LOCATION_HPP

#include <string>
#include <vector>

#include "Block.hpp"

struct 	Server;
struct 	Location
{
	Location()
	:	path("/"), root("/"), index(""),
		is_cgi(false), directoryListing(false), redirection(-1, "")
		{
			std::string	defaults[] = {"GET", "POST", "DELETE"};
			allowed_methods = std::vector<std::string>(defaults, defaults + 3);
		}

	Location(Block& block);

	std::string		path;
	std::string		root;
	std::string		index;
	std::string		cgi_pass;
	std::string		cgi_extension;
	bool			is_cgi;

	bool			directoryListing;
	std::string		upload_dir;

	std::pair<int, std::string> redirection;

	std::vector<std::string>	allowed_methods;
private:
	void	setupLocationRoot(const Block& block);

};
const Location*	getLocation(const std::string& path, const Server& server);

#endif