NAME = webserv
MODE ?= release

OBJ_DIR = obj-$(MODE)
INCLUDES = -Iincludes

CXX = c++
CXXFLAGS = -Wall -Werror -Wextra -MD $(INCLUDES) -D PRETTY_LOGGER -std=c++98 -fPIE

ifeq ($(MODE), debug)
	CXX = g++
	CXXFLAGS = -Wall -Wextra -MD $(INCLUDES) -D PRETTY_LOGGER -g3 -std=c++98
endif

VPATH = srcs:srcs/parser:srcs/server:srcs/cgi

SRCS =	main.cpp		\
		Webserv.cpp		\
		Parser.cpp		\
		Log.cpp			\
		ParserUtils.cpp	\
		Block.cpp		\
		CgiHandler.cpp	\
		Server.cpp		\
		Location.cpp	\
		Session.cpp		\


OBJS = $(addprefix $(OBJ_DIR)/, $(SRCS:.cpp=.o))
DEPS = $(OBJS:.o=.d)
BIN = $(NAME)

RESET			= \033[0m
GRAY			= \033[90m
RED 			= \033[31m
GREEN 			= \033[32m
YELLOW 			= \033[33m
BLUE 			= \033[34m

all:
	$(MAKE) $(BIN)
	printf "$(RESET)"

debug:
	$(MAKE) MODE=debug all
	./webserv ./conf/default.conf

debuger:
	$(MAKE) MODE=debug all

$(OBJ_DIR):
	mkdir -p $(OBJ_DIR)

$(BIN): $(OBJS)
	$(CXX) $(CXXFLAGS) $(OBJS) -o $@

$(OBJ_DIR)/%.o: %.cpp Makefile |  $(OBJ_DIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@
	printf "$(GRAY)compiling: $(BLUE)%-40s $(GRAY)[%d/%d]\n" "$<" "$$(ls $(OBJ_DIR) | grep -c '\.o')" "$(words $(SRCS))"

units: $(OBJS)
	ar rcs libwebserv.a $(OBJS)
	$(MAKE) test -C libunit

leaks:
	$(MAKE) MODE=debug all
	valgrind --leak-check=full --show-leak-kinds=all --track-origins=yes --track-fds=yes ./webserv

clog:
	rm -rf log_webserv_*

clean: clog
	rm -rf obj-*

fclean: clean
	rm -f $(NAME)

re: fclean all

.PHONY: all clean fclean re debug leaks clog

-include $(DEPS)
.SILENT:
MAKEFLAGS=--no-print-directory
