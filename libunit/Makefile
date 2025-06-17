NAME = libunit.a
CFLAGS = $(INCLUDES_D) -g3
CC = g++
MAKEFLAGS += --no-print-directory

SRC =	Test.cpp		\
		Routine.cpp		\
		Utils.cpp		\

OBJ = $(SRC:.cpp=.o)

OBJ_D = obj/
SRCS_D = src/
BIN_D = bin/
LOG_D = log/
INCLUDES_D = -Iincludes/

OBJ := $(addprefix $(OBJ_D), $(OBJ))
SRCS := $(addprefix $(SRCS_D), $(SRCS))

# colors
RESET 			= \033[0m
RED 			= \033[31m
GREEN 			= \033[32m
YELLOW 			= \033[33m
BLUE 			= \033[34m
CURSOR_OFF 		= \e[?25l
CURSOR_ON 		= \e[?25h

RM = rm -fr
ARGS = 1

all: header $(BIN_D)$(NAME)
	$(MAKE) -C tests/

.PHONY: header
header:
	printf "$(YELLOW)"
	printf "\n ▗▖   ▗▄▄▄▖▗▄▄▖ ▗▖ ▗▖▗▖  ▗▖▗▄▄▄▖▗▄▄▄▖ "
	printf "\n ▐▌     █  ▐▌ ▐▌▐▌ ▐▌▐▛▚▖▐▌  █    █  "
	printf "\n ▐▌     █  ▐▛▀▚▖▐▌ ▐▌▐▌ ▝▜▌  █    █   "
	printf "\n ▐▙▄▄▖▗▄█▄▖▐▙▄▞▘▝▚▄▞▘▐▌  ▐▌▗▄█▄▖  █   "
	printf "\n\n---\n"
	printf "[Author]: $(GREEN)Maxime Juncker"
	printf "$(YELLOW)\t[Github]: $(GREEN)https://github.com/Maxime-juncker/better-libunit.git\n\n"

$(BIN_D)$(NAME): $(OBJ) | $(BIN_D)
	printf "$(BLUE)compiling: [$$(ls obj | wc -l)/$(shell ls src | wc -l)] [OK]\r\n"
	ar rcs $(BIN_D)$(NAME) $(OBJ)
	printf "$(GREEN)$(NAME): success\n"
	printf "\n---------------------$(CURSOR_ON)\n\n"

$(OBJ_D)%.o : $(SRCS_D)%.cpp includes/libunit.hpp | $(OBJ_D)
	printf "$(CURSOR_OFF)$(BLUE)"
	printf "compiling: [$$(ls obj | wc -l)/$(shell ls src | wc -l)]\r"
	$(CC) $(CFLAGS) -c $< -o $@

.PHONY: clean
clean:
	$(RM) $(OBJ_D)
	printf "$(RED)clean:\t$(NAME)\n"
	$(MAKE) fclean -C tests/
	printf "$(RED)\n---------------------\n\n$(RESET)"
	$(MAKE) clog

.PHONY: fclean
fclean:
	$(RM) $(BIN_D)
	printf "$(RED)fclean:\t$(NAME)\n"
	$(MAKE) clean

.PHONY: clog
clog:
	$(RM) $(LOG_D)

.PHONY: re
re:
	$(MAKE) fclean
	$(MAKE) all

.PHONY: test
test: all
	$(MAKE) test -C tests/

# Create directories
$(OBJ_D):
	mkdir -p $(OBJ_D)

$(LOG_D):
	mkdir -p $(LOG_D)

$(BIN_D):
	mkdir -p $(BIN_D)

.PHONY: debug
debug: all $(LOG_D)
	$(BIN_D)./push_swap $(ARGS) > $(LOG_D)$(shell date --iso=seconds).log
	cat $(LOG_D)$(shell date --iso=seconds).log
	echo "$(BLUE)[SAVED]: $(LOG_D)$(shell date --iso=seconds).log"

.SILENT: