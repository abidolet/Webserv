/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   CgiHandler.cpp                                     :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: ygille <ygille@student.42lyon.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: Invalid date        by                   #+#    #+#             */
/*   Updated: 2025/06/27 11:18:57 by ygille           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */


#include "CgiHandler.hpp"

/* Canonical Form */
CgiHandler::CgiHandler(const std::string& method, const std::string& contentType, const std::string& contentLength, const Server& server)
: server(server), method(method), contentType(contentType), contentLength(contentLength), fd(0)
{
	for (int i = 0; i < ENV_SIZE; ++i)
		this->envConstruct[i] = baseEnv[i];
	for (int i = 0; i < INFO_SIZE; ++i)
		this->info[i] = baseInfos[i];
}

CgiHandler::~CgiHandler()
{
	this->closePipes();
}
/* End-Of Canonical Form */

void		CgiHandler::addBody()
{
	char		buffer[4096];
	ssize_t		bytes_read;

	while ((bytes_read = recv(this->fd, buffer, sizeof(buffer), 0)) > 0)
	{
		Log(Log::DEBUG) << "Writing" << Log::endl();
		write (this->pipes.to_cgi[INPUT], buffer, bytes_read);
	}
	Log(Log::DEBUG) << "Done receiving file" << Log::endl();
	this->info[BODY_SENT] = true;
}

std::string	CgiHandler::launch()
{
	if (this->info[EXECUTED])
	{
		Log(Log::ERROR) << "This CGI request has been already executed" << Log::endl();
		return NULL;
	}
	if (!this->info[PIPES_OPENED])
	{
		Log(Log::ERROR) << "Pipes not opened, can't executed" << Log::endl();
		return NULL;
	}
	if (this->info[AS_BODY] && !this->fd)
		Log(Log::ERROR) << "This request need body" << Log::endl();

	Log(Log::LOG) << this->cgi << Log::endl();
	this->pid = fork();

	if (pid < 0)
	{
		Log(Log::ERROR) << "Fork failed" << Log::endl();
		return NULL;
	}
	if (pid == 0)
		this->childProcess();
	else
	{
		this->addBody();
		return this->father();
	}
	return generatePage(500, "CGI FAILED");
}

void	CgiHandler::createPipes()
{
	if (pipe(this->pipes.to_cgi) == -1 || pipe(this->pipes.from_cgi) == -1)
	{
		Log(Log::ERROR) << "Pipes not opened" << Log::endl();
		return;
	}
	this->info[PIPES_OPENED] = true;
}

void	CgiHandler::constructEnv(HttpRequest request)
{
	if (contentLength.length() > 1)
	{
		this->envConstruct[CONTENT_LENGTH].append(this->contentLength);
		this->envConstruct[CONTENT_TYPE].append(this->contentType);
		this->info[AS_BODY] = true;
	}

	this->envConstruct[REQUEST_METHOD].append(this->method);
	this->envConstruct[SCRIPT_FILENAME].append(this->script);
	this->envConstruct[SCRIPT_NAME].append(this->script);

	this->envConstruct[REQUEST_URI].append(this->script);

	this->path.append(this->script);

	this->envConstruct[SERVER_PROTOCOL].append(DEFAULT_SERVER_PROTOCOL);

	this->envConstruct[PATH_INFO].append("/");
	this->envConstruct[HTTP_COOKIE].append(request.server.getCookiesCgi());

	for (int i = 0; i < ENV_SIZE; ++i)
		this->env[i] = const_cast<char*>(this->envConstruct[i].c_str());
	this->env[ENV_SIZE] = NULL;
}

void	CgiHandler::closePipes()
{
	if (this->info[PIPES_OPENED])
	{
		CLOSE(this->pipes.from_cgi[OUTPUT]);
		CLOSE(this->pipes.from_cgi[INPUT]);
		CLOSE(this->pipes.to_cgi[OUTPUT]);
		CLOSE(this->pipes.to_cgi[INPUT]);
		this->info[PIPES_OPENED] = false;
	}
}

void	CgiHandler::childProcess()
{
	char* args[] = {const_cast<char*>(this->path.c_str()), const_cast<char*>(this->path.c_str()), NULL};

	CLOSE(pipes.to_cgi[INPUT]);
	CLOSE(pipes.from_cgi[OUTPUT]);

	dup2(pipes.to_cgi[OUTPUT], STDIN_FILENO);
	dup2(pipes.from_cgi[INPUT], STDOUT_FILENO);
	CLOSE(pipes.to_cgi[OUTPUT]);
	CLOSE(pipes.from_cgi[INPUT]);

	execve(this->cgi.c_str(), args, this->env);

	Log(Log::ERROR) << "Execve failed" << Log::endl();
}

std::string	CgiHandler::father()
{
	int 				status = 0;
	int					wait = 0;
    char 				buffer[1024];
	ssize_t 			bytes_read;
    std::string 		cgi_output = HTTP_OK;
	const clock_t		start = clock();

	CLOSE(pipes.to_cgi[OUTPUT]);
    CLOSE(pipes.from_cgi[INPUT]);

    while ((bytes_read = read(pipes.from_cgi[0], buffer, sizeof(buffer) - 1)) > 0)
	{
        buffer[bytes_read] = '\0';
        cgi_output += buffer;
    }
    CLOSE(pipes.from_cgi[OUTPUT]);
    CLOSE(pipes.to_cgi[INPUT]);

	this->info[EXECUTED] = true;

	while (!wait)
	{
    	wait = waitpid(pid, &status, WNOHANG);
		if (start + TIMEOUT_DELAY < clock())
			break;
	}
	if (wait <= 0)
	{
		kill(pid, SIGTERM);
		return getErrorPage(504, this->server);
	}

	Log(Log::LOG) << "CGI Executed" << Log::endl();

	if (WIFEXITED(status))
	{
        if (WEXITSTATUS(status) != 0)
            return getErrorPage(500, this->server);
    }
	else
    	return getErrorPage(500, this->server);
	return cgi_output;
}

bool	CgiHandler::cgiRequest(HttpRequest request, std::vector<Location> locations)
{
	std::string	extension;
	size_t		extPos = request.path.find_last_of('.');

	if (extPos == request.path.npos)
		return false;
	extension.append(const_cast<char*>(request.path.c_str()), extPos, request.path.npos);
	for (std::vector<Location>::iterator it = locations.begin(); it != locations.end(); ++it)
	{
		if (it->cgi_extension == extension)
		{
			this->cgi = it->cgi_pass;
			this->script = request.path;
			this->constructEnv(request);
			this->createPipes();
			return true;
		}
	}
	return false;
}

void	CgiHandler::sendFd(int fd)
{
	this->fd = fd;
}
