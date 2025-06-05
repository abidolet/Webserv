/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   CgiHandler.hpp                                     :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: ygille <ygille@student.42lyon.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/06/05 11:58:29 by ygille            #+#    #+#             */
/*   Updated: 2025/06/05 18:52:01 by ygille           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#pragma once

#include <string>
#include <stdexcept>
#include <cstring>
#include <cstdlib>
#include <sys/wait.h>
#include <unistd.h>

typedef struct	s_pipes
{
	int	to_cgi[2];
	int	from_cgi[2];
}	t_pipes;

class	CgiHandler
{
public:

	CgiHandler(std::string cgi, std::string script, std::string query, std::string method, std::string addr);
	~CgiHandler();

	std::string	launch();

protected:

private:

	std::string	envStr[6] = {"REQUEST_METHOD=", "QUERY_STRING=", "SCRIPT_NAME=", "SERVER_PROTOCOL=HTTP/1.1", "REMOTE_ADDR="};
	char**		env;

	std::string	cgi;
	std::string script;
	std::string query;
	std::string method;
	std::string addr;

	t_pipes		pipes;

	int			pid;

	void		createPipes();
	void		closePipes();
	void		cgiSetEnv();
	void		childProcess();
	std::string	father();

	CgiHandler(const CgiHandler& other);
	CgiHandler& operator=(const CgiHandler& other);
};
