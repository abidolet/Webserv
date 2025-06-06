/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   CgiHandler.cpp                                     :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: ygille <ygille@student.42lyon.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/06/05 11:58:35 by ygille            #+#    #+#             */
/*   Updated: 2025/06/06 13:42:40 by ygille           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "CgiHandler.hpp"

/* Canonical Form */
CgiHandler::CgiHandler(std::string cgi, std::string script, std::string method, std::string query) : cgi(cgi), script(script), method(method), query(query), bodySent(false), pipesOpened(false), executed(false)
{
	this->createPipes();
}

CgiHandler::CgiHandler(const CgiHandler& other){}

CgiHandler& CgiHandler::operator=(const CgiHandler& other){return (*this);}

CgiHandler::~CgiHandler()
{
	if (!this->executed)
		this->closePipes();
}
/* End-Of Canonical Form */

void		CgiHandler::addBody(std::string body)
{
	write (this->pipes.to_cgi[INPUT], body.c_str(), body.length());
	this->bodySent = true;
}

std::string	CgiHandler::launch()
{
	if (this->executed)
	{
		Log(Log::ERROR) << "This CGI request has been already executed" << Log::endl();
		return NULL;
	}
	if (!this->pipesOpened)
	{
		Log(Log::ERROR) << "Pipes not opened, can't executed" << Log::endl();
		return NULL;
	}
	if ((this->method == "POST" || this->method == "DELETE") && !this->bodySent)
		Log(Log::ERROR) << "This request need body before executing" << Log::endl();

	this->pid = fork();

	if (pid < 0)
	{
		Log(Log::ERROR) << "Fork failed" << Log::endl();
		return NULL;
	}
	if (pid == 0)
		this->childProcess();
	return this->father();
}

void	CgiHandler::createPipes()
{
	if (pipe(this->pipes.to_cgi) == -1 || pipe(this->pipes.from_cgi) == -1)
	{
		Log(Log::ERROR) << "Pipes not opened" << Log::endl();
		return;
	}
	this->pipesOpened = true;
}

void	CgiHandler::closePipes()
{
	close(this->pipes.from_cgi[OUTPUT]);
	close(this->pipes.from_cgi[INPUT]);
	close(this->pipes.to_cgi[OUTPUT]);
	close(this->pipes.to_cgi[INPUT]);
	this->pipesOpened = false;
}

void	CgiHandler::cgiSetEnv()
{
	this->envStr[0].append(method);
	this->envStr[1].append(query);
	this->envStr[2].append(script);

	for (int i = 0 ; i < 5 ; ++i)
		this->env[i] = const_cast<char*>(this->envStr[i].c_str());
	this->env[5] = NULL;
}

void	CgiHandler::childProcess()
{
	char* args[] = { const_cast<char*>(this->cgi.c_str()), const_cast<char*>(this->script.c_str()), NULL };

	close(pipes.to_cgi[INPUT]);
	close(pipes.from_cgi[OUTPUT]);

	dup2(pipes.to_cgi[OUTPUT], STDIN_FILENO);
	dup2(pipes.from_cgi[INPUT], STDOUT_FILENO);
	close(pipes.to_cgi[OUTPUT]);
	close(pipes.from_cgi[INPUT]);

	this->cgiSetEnv();
	execve(this->cgi.c_str(), args, this->env);
	
	Log(Log::ERROR) << "Execve failed" << Log::endl();
}

std::string	CgiHandler::father()
{
	int status;
    char buffer[1024];
	ssize_t bytes_read;
    std::string cgi_output;

	close(pipes.to_cgi[OUTPUT]);
    close(pipes.from_cgi[INPUT]);

    while ((bytes_read = read(pipes.from_cgi[0], buffer, sizeof(buffer) - 1)) > 0) 
	{
        buffer[bytes_read] = '\0';
        cgi_output += buffer;
    }
    close(pipes.from_cgi[OUTPUT]);
    close(pipes.to_cgi[INPUT]);

	this->executed = true;

    waitpid(pid, &status, 0);

	if (WIFEXITED(status)) 
	{
        if (WEXITSTATUS(status) != 0) 
            Log(Log::ERROR) << "CGI script failed" << Log::endl();
    }
	else
    	Log(Log::ERROR) << "CGI script terminated abnormally" << Log::endl();
	return cgi_output;
}