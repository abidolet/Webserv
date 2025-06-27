/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   CgiHandler.hpp                                     :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: ygille <ygille@student.42lyon.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/06/05 11:58:29 by ygille            #+#    #+#             */
/*   Updated: 2025/06/27 11:14:41 by ygille           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#pragma once

#include <string>
#include <ctime>
#include <stdexcept>
#include <cstring>
#include <cstdlib>
#include <sys/wait.h>
#include <unistd.h>
#include <sys/socket.h>

#include "Webserv.hpp"

#define	INPUT					1
#define OUTPUT					0

#define TIMEOUT_DELAY			10000

#define	DEFAULT_SERVER_PROTOCOL	"HTTP/1.1"

#define HTTP_OK					"HTTP/1.1 200 OK\n"

/*	example URL
	http://example.com/cgi-bin/script.cgi/extra/path/file.txt?key=value&foo=bar

	KEY						VALUE														PRECISION

	REQUEST_METHOD		 	GET/POST
	SCRIPT_NAME			 	/cgi-bin/script.cgi
	PATH_INFO			 	/extra/path/file.txt
	PATH_TRANSLATED		 	/var/www/html/extra/path/file.txt							DOCUMENT_ROOT + PATH_INFO
	QUERY_STRING		 	key=value&foo=bar 											after ? in URL
	CONTENT_TYPE		 	application/x-www-form-urlencoded
	CONTENT_LENGTH		 	1024
	REQUEST_URI=		 	/cgi-bin/script.cgi/extra/path/file.txt?key=value&foo=bar
	SERVER_NAME			 	server.com
	SERVER_PORT			 	8080
	SERVER_PROTOCOL		 	HTTP/1.1
	SERVER_SOFTWARE		 	webserv/418.0
	DOCUMENT_ROOT		 	/var/www/html
	REMOTE_ADDR			 	10.2.56.36
	REMOTE_HOST			 	client.foo.fr
	REMOTE_PORT			 	35255
	HTTP_USER_AGENT		 	Mozilla/5.0 (Windows NT 10.0; Win64; x64)
	HTTP_ACCEPT			 	application/x-www-form-urlencoded
	HTTPS				 	on/off
	SERVER_ADDR			 	5.2.36.9
	SCRIPT_FILENAME		 	/var/www/html/cgi-bin/script.cgi							DOCUMENT_ROOT + SCRIPT_NAME
	REDIRECT_STATUS		 	200/CGI
	GATEWAY_INTERFACE	 	CGI/1.1
*/

enum
{
	REQUEST_METHOD = 0,
	SCRIPT_NAME,
	PATH_INFO,
	PATH_TRANSLATED,
	QUERY_STRING,
	CONTENT_TYPE,
	CONTENT_LENGTH,
	REQUEST_URI,
	SERVER_NAME,
	SERVER_PORT,
	SERVER_PROTOCOL,
	SERVER_SOFTWARE,
	DOCUMENT_ROOT,
	REMOTE_ADDR,
	REMOTE_HOST,
	REMOTE_PORT,
	HTTP_USER_AGENT,
	HTTP_ACCEPT,
	HTTPS,
	SERVER_ADDR,
	SCRIPT_FILENAME,
	REDIRECT_STATUS,
	HTTP_COOKIE,
	GATEWAY_INTERFACE,
	ENV_LAST_ELEM = GATEWAY_INTERFACE
};

enum
{
	AS_BODY = 0,
	BODY_SENT,
	PIPES_OPENED,
	EXECUTED,
	INFO_LAST_ELEM = EXECUTED
};

#define ENV_SIZE				ENV_LAST_ELEM + 2
#define	INFO_SIZE				INFO_LAST_ELEM + 1

const std::string	baseEnv[ENV_SIZE] = {	"REQUEST_METHOD=",
											"SCRIPT_NAME=",
											"PATH_INFO=",
											"PATH_TRANSLATED=",
											"QUERY_STRING=",
											"CONTENT_TYPE=",
											"CONTENT_LENGTH=",
											"REQUEST_URI=",
											"SERVER_NAME=",
											"SERVER_PORT=",
											"SERVER_PROTOCOL=",
											"SERVER_SOFTWARE=",
											"DOCUMENT_ROOT=",
											"REMOTE_ADDR=",
											"REMOTE_HOST=",
											"REMOTE_PORT=",
											"HTTP_USER_AGENT=",
											"HTTP_ACCEPT=",
											"HTTPS=",
											"SERVER_ADDR=",
											"SCRIPT_FILENAME=",
											"REDIRECT_STATUS=",
											"HTTP_COOKIE=",
											"GATEWAY_INTERFACE="};

const bool			baseInfos[INFO_SIZE] = {false, false, false, false};

typedef struct	s_pipes
{
	int	to_cgi[2];
	int	from_cgi[2];
}	t_pipes;

struct	HttpRequest;
struct	Location;

class	CgiHandler
{
public:

	CgiHandler(const std::string& method, const std::string& contentType, const std::string& contentLength);
	~CgiHandler();

	void		sendFd(int fd);

	std::string	launch();

	bool		cgiRequest(HttpRequest request, std::vector<Location> locations);

protected:

private:

	std::string	envConstruct[ENV_SIZE];
	char*		env[ENV_SIZE + 1];

	std::string method;
	std::string	contentType;
	std::string contentLength;

	std::string	cgi;
	std::string script;
	std::string	path;

	bool		info[INFO_SIZE];

	t_pipes		pipes;

	int			pid;
	int			fd;

	void		createPipes();
	void		constructEnv(HttpRequest request);
	void		closePipes();
	void		childProcess();
	std::string	father();
	void		addBody();

	CgiHandler(const CgiHandler& other);
	CgiHandler& operator=(const CgiHandler& other);
};
