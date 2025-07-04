#include <string>
#include <vector>
#include <map>

#include "Block.hpp"

struct 	Location;

struct Listen
{
	std::string	addr;
	int 		port;

	Listen(std::string _addr, int _port) : addr(_addr), port(_port) { };
	bool operator==(const Listen& other);
};
size_t find(std::vector<Listen> vec, Listen toFind);

struct	Server
{
	Server()
		: root("/"), client_max_body_size(0)
	{
		server_names.push_back("localhost");
		listen.push_back(Listen("0.0.0.0", 8080));
	}

	void		init(Block& block);
	void		runSelfCheck();

	void		cookiesAssert();
	std::string	getCookies() const;
	std::string	getCookiesCgi() const;

	Location*	searchLocationByName(const std::string &name);
	static void	registerSession(const uint uid);

	std::string					root;
	std::vector<std::string>	server_names;
	size_t						client_max_body_size;

	std::vector<std::string>	allowed_methods;
	std::vector<Location>		locations;

	std::map<int, std::string>	error_pages;
	std::vector<Listen>			listen;

	bool						is_default;

	std::vector<std::string>	cookies;
	uint						lastUID;

private:
	void	setupMaxBodySize(Block& block);
	void	setupRedirections(Block& block);
	void	setupListen(Block &block);

};