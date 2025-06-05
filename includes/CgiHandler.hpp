/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   CgiHandler.hpp                                     :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: ygille <ygille@student.42lyon.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/06/05 11:58:29 by ygille            #+#    #+#             */
/*   Updated: 2025/06/05 13:45:04 by ygille           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#pragma once

#include <string>
#include <stdexcept>
#include <cstring>
#include <cstdlib>
#include "unistd.h"

typedef struct	s_pipes
{
	int	to_cgi[2];
	int	from_cgi[2];
}	t_pipes;

class	CgiHandler
{
public:

	std::string	handleCgi(std::string query, std::string script, std::string method, std::string addr, int client);

protected:

private:

	t_pipes		createPipes();
	void		closePipes(t_pipes pipes);
	char**		cgiSetEnv(std::string query, std::string script, std::string method, std::string addr);
	void		childProcess(t_pipes pipes, std::string query, std::string script, std::string method, std::string addr);
	std::string	father(t_pipes pipes);

/* Canonical Form */
CgiHandler();
CgiHandler(const CgiHandler& other);
CgiHandler& operator=(const CgiHandler& other);
~CgiHandler();
/* End-Of Canonical Form */
};
