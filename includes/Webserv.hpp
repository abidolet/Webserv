#ifndef WEBSERV_HPP
# define WEBSERV_HPP

# include <iostream>
# include <vector>
# include <map>

struct	HttpRequest
{
	std::string							method;
	std::string							body;
	std::string							path;
	std::map<std::string, std::string>	headers;
};

struct	Location
{
	std::string					path;
	std::string					root;
	std::string					redirect;
	std::string					cgi_pass;
	std::vector<std::string>	cgi_extensions;
	bool						autoindex;
	std::vector<std::string>	allowed_methods;
};

struct	Server
{
	int							port;
	std::string					host;
	std::string					server_name;
	bool						is_default;
	std::map<int, std::string>	error_pages;
	size_t						client_max_body_size;
	std::vector<Location>		locations;
	int							fd;
};

class	Webserv
{
	private:
		std::vector<Server>	_servers;

		int					_epoll_fd;
		int					_listener_fd;

		// HttpRequest	parseRequest(const std::string& rawRequest);
		// std::string	handleGetRequest(const std::string& request);
		// std::string	handlePostRequest(void);
		// std::string	handleDeleteRequest(const std::string& request);

	public:
		Webserv();
		Webserv(const std::string& file);
		~Webserv();

		void	run();
};

#endif
