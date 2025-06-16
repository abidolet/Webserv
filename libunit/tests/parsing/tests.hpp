#pragma once

#include <libunit.hpp>

//
// routines
//
int routine_parsing_ok( void );
int routine_parsing_error( void );

//
// valid configs
//
int ok_config();
int	whitespace();
int	multiserv();
int	multLocation();
int	pagePath();
int	empty();

//
// errors config
//
int	Err_listen1();
int	Err_listen2();
int	Err_listen3();
int	Err_listen4();
int	Err_listen5();
int	Err_listen6();
int	Err_noBeginBrace();
int	Err_noEndBrace();
int	Err_Cgi();
int	Err_wrongLocation();
int	Err_Cookies1();
int	Err_Cookies2();
int	Err_Cookies3();
int	Err_Cookies4();
int	Err_Cookies5();
