/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   CgiHandler.cpp                                     :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: ygille <ygille@student.42lyon.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/06/05 11:58:35 by ygille            #+#    #+#             */
/*   Updated: 2025/06/05 18:52:25 by ygille           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "CgiHandler.hpp"

/* Canonical Form */
CgiHandler::CgiHandler(std::string cgi, std::string script, std::string query, std::string method, std::string addr) : cgi(cgi), script(script), query(query), method(method), addr(addr){}

CgiHandler::CgiHandler(const CgiHandler& other){}

CgiHandler& CgiHandler::operator=(const CgiHandler& other){return (*this);}

CgiHandler::~CgiHandler(){}
/* End-Of Canonical Form */

std::string	CgiHandler::launch()
{
	this->createPipes();
	this->pid = fork();

	if (pid < 0)
		throw std::runtime_error("fork failed");
	if (pid == 0)
		this->childProcess();
	return this->father();
}

void	CgiHandler::createPipes()
{
	if (pipe(this->pipes.to_cgi) == -1 || pipe(this->pipes.from_cgi) == -1)
		throw	std::runtime_error("Pipes error");
}

void	CgiHandler::closePipes()
{
	close(this->pipes.from_cgi[0]);
	close(this->pipes.from_cgi[1]);
	close(this->pipes.to_cgi[0]);
	close(this->pipes.to_cgi[1]);
}

void	CgiHandler::cgiSetEnv()
{
	this->envStr[0].append(method);
	this->envStr[1].append(query);
	this->envStr[2].append(script);
	this->envStr[4].append(addr);

	for (int i = 0 ; i < 5 ; ++i)
		this->env[i] = const_cast<char*>(this->envStr[i].c_str());
	this->env[5] = NULL;
}

void	CgiHandler::childProcess()
{
	char* args[] = { const_cast<char*>(this->cgi.c_str()), const_cast<char*>(this->script.c_str()), NULL };

	close(pipes.to_cgi[1]);
	close(pipes.from_cgi[0]);

	dup2(pipes.to_cgi[0], STDIN_FILENO);
	dup2(pipes.from_cgi[1], STDOUT_FILENO);
	close(pipes.to_cgi[0]);
	close(pipes.from_cgi[1]);

	this->cgiSetEnv();
	execve(this->cgi.c_str(), args, this->env);
	
	throw	std::runtime_error("execve failed");
}

std::string	CgiHandler::father()
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

	if (WIFEXITED(status)) 
	{
        if (WEXITSTATUS(status) == 0) 
			return cgi_output;
        else 
            throw std::runtime_error("PHP CGI script failed");
    }
	else
    	throw std::runtime_error("PHP CGI script terminated abnormally\n");
}