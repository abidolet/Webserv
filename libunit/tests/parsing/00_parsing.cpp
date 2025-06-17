/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   00_parsing.cpp                                     :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: mjuncker <mjuncker@student.42lyon.fr>      +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/01/26 10:49:49 by mjuncker          #+#    #+#             */
/*   Updated: 2025/06/16 13:28:52 by mjuncker         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#define COLORS

#include "Routine.hpp"
#include "Test.hpp"
#include "tests.hpp"
#include "Webserv.hpp"

int routine_parsing_ok( void )
{
	Libunit::Routine	routine("parsing no error");

	Log::setFlags(F_ERROR);

	/* ================ ADD TESTS HERE ================ */
	routine.AddNewTest(Libunit::Test("PARSING",		"valid",		&ok_config));
	routine.AddNewTest(Libunit::Test("PARSING",		"mutliserv",	&multiserv));
	routine.AddNewTest(Libunit::Test("PARSING",		"whitespace",	&whitespace));
	routine.AddNewTest(Libunit::Test("PARSING",		"mutlLocation",	&multLocation));
	routine.AddNewTest(Libunit::Test("PARSING",		"page path",	&pagePath));
	routine.AddNewTest(Libunit::Test("PARSING",		"empty",		&empty));
	// ==================================================

	routine.Run();
	return (routine.TestsOK());
}


int routine_parsing_error( void )
{
	Libunit::Routine	routine("parsing error");

	Log::setFlags(F_ERROR);

	/* ================ ADD TESTS HERE ================ */
	routine.AddNewTest(Libunit::Test("LISTEN   ",  	"`8080:'",			&Err_listen1));
	routine.AddNewTest(Libunit::Test("LISTEN   ",  	"`:8080'",			&Err_listen3));
	routine.AddNewTest(Libunit::Test("LISTEN   ",  	"`0.0.0.0:'",		&Err_listen2));
	routine.AddNewTest(Libunit::Test("LISTEN   ",  	"`0.0.00:8080'", 	&Err_listen4));
	routine.AddNewTest(Libunit::Test("LISTEN   ",  	"`0.0.0.0.0:8080'",	&Err_listen5));
	routine.AddNewTest(Libunit::Test("LISTEN   ",  	"`:0.0.0.0:8080'",	&Err_listen6));
	routine.AddNewTest(Libunit::Test("PARSING  ", 	"`noBeginBrace'",	&Err_noBeginBrace));
	routine.AddNewTest(Libunit::Test("PARSING  ", 	"`noEndBrace'",		&Err_noEndBrace));
	routine.AddNewTest(Libunit::Test("Cgi      ", 	"`Cgi'",			&Err_Cgi));
	routine.AddNewTest(Libunit::Test("DIRECTIVE",	"`invalidDirName'", &Err_wrongLocation));
	routine.AddNewTest(Libunit::Test("COOKIES  ",	"`='",				&Err_Cookies1));
	routine.AddNewTest(Libunit::Test("COOKIES  ",	"`=test'",			&Err_Cookies2));
	routine.AddNewTest(Libunit::Test("COOKIES  ",	"`test='",			&Err_Cookies3));
	routine.AddNewTest(Libunit::Test("COOKIES  ",	"`test test'",		&Err_Cookies4));
	routine.AddNewTest(Libunit::Test("COOKIES  ",	"`=test='",			&Err_Cookies5));

	// ==================================================

	routine.Run();
	return (routine.TestsOK());
}
