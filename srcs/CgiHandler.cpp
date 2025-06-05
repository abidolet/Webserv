/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   CgiHandler.cpp                                     :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: ygille <ygille@student.42lyon.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/06/05 11:58:35 by ygille            #+#    #+#             */
/*   Updated: 2025/06/05 16:10:10 by ygille           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "CgiHandler.hpp"

/* Canonical Form */
CgiHandler::CgiHandler(){}

CgiHandler::CgiHandler(const CgiHandler& other){}

CgiHandler& CgiHandler::operator=(const CgiHandler& other){return (*this);}

CgiHandler::~CgiHandler(){}
/* End-Of Canonical Form */

std::string	CgiHandler::handleCgi(std::string query, std::string script, std::string method, std::string addr)
{
	t_pipes	pipes = createPipes();
	int		pid = fork();

	if (pid < 0)
		throw std::runtime_error("fork failed");
	if (pid == 0)
		childProcess(pipes, query, script, method, addr);
	else
		return father(pipes, pid);
}

t_pipes	CgiHandler::createPipes()
{
	t_pipes	pipes;

	if (pipe(pipes.to_cgi) == -1 || pipe(pipes.from_cgi) == -1)
		throw	std::runtime_error("Pipes error");
	return (pipes);
}

void	CgiHandler::closePipes(t_pipes pipes)
{
	close(pipes.from_cgi[0]);
	close(pipes.from_cgi[1]);
	close(pipes.to_cgi[0]);
	close(pipes.to_cgi[1]);
}

char**	CgiHandler::cgiSetEnv(std::string query, std::string script, std::string method, std::string addr)
{
	std::string	envStr[5] = {"REQUEST_METHOD=", "QUERY_STRING=", "SCRIPT_NAME=", "SERVER_PROTOCOL=HTTP/1.1", "REMOTE_ADDR="};

	envStr[0].append(method);
	envStr[1].append(query);
	envStr[2].append(script);
	envStr[4].append(addr);
	for (int i = 0; i <=4; ++i)
		putenv(const_cast<char*>(envStr[i].c_str()));
}

void	CgiHandler::childProcess(t_pipes pipes, std::string query, std::string script, std::string method, std::string addr)
{
	char* args[] = { const_cast<char*>(script.c_str()), NULL };
    char* env[] = { NULL };

	close(pipes.to_cgi[1]);
	close(pipes.from_cgi[0]);

	dup2(pipes.to_cgi[0], STDIN_FILENO);
	dup2(pipes.from_cgi[1], STDOUT_FILENO);
	close(pipes.to_cgi[0]);
	close(pipes.from_cgi[1]);

	cgiSetEnv(query, script, method, addr);

	execve(script.c_str(), args, env);
	
	throw	std::runtime_error("execve failed");
}

std::string	CgiHandler::father(t_pipes pipes, int pid)
{
	int status;
    char buffer[1024];
	ssize_t bytes_read;
    std::string cgi_output;

	close(pipes.to_cgi[0]);
    close(pipes.from_cgi[1]);

    while ((bytes_read = read(pipes.from_cgi[0], buffer, sizeof(buffer) - 1)) > 0) 
	{
        buffer[bytes_read] = '\0';
        cgi_output += buffer;
    }
    close(pipes.from_cgi[0]);
    close(pipes.to_cgi[1]);

    waitpid(pid, &status, 0);

    return cgi_output;
}