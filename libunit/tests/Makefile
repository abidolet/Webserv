MAKEFLAGS += --no-print-directory

NAME = tests.out
CC = g++
CFLAGS = -Wall -Wextra -Werror -g3
INCLUDES =	-I../includes/	-I../../includes/			\

TEST_SRCS = parsing/00_parsing.cpp	\
			parsing/01_parsing_ok.cpp	\
			parsing/02_parsing_error.cpp	\


TEST_OBJS = $(TEST_SRCS:.cpp=.o)
MAIN_OBJ = main.o

FRAMEWORK_LIB = ../bin/libunit.a ../../libwebserv.a

# colors
RED = 			\033[31m
GREEN = 		\033[32m
YELLOW = 		\033[33m
RESET = 		\033[0m
BLUE = 			\033[34m
CURSOR_OFF =	\e[?25l
CURSOR_ON = 	\e[?25h

all: $(NAME)

$(NAME): $(TEST_OBJS) $(MAIN_OBJ)
	printf "$(BLUE)compiling: [$$(find . -type f -name "*.o" | wc -l)/$(shell find . -type f -name "*.cpp" | wc -l)] [OK]\r\n"
	$(CC) $(CFLAGS) $(MAIN_OBJ) $(TEST_OBJS) $(FRAMEWORK_LIB) $(INCLUDES) -o $(NAME)
	printf "$(GREEN)$(NAME): success\n"
	printf "\n---------------------$(CURSOR_ON)\n\n"

%.o: %.cpp $(FRAMEWORK_LIB)
	printf "$(CURSOR_OFF)$(BLUE)"
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@
	printf "compiling: [$$(find . -type f -name "*.o" | wc -l)/$(shell find . -type f -name "*.cpp" | wc -l)]\r"

clean:
	printf "$(RED)clean:  tests.out\n"
	rm -f $(MAIN_OBJ) $(TEST_OBJS)

fclean: clean
	rm -f $(NAME)

re: fclean all

test: all
	./$(NAME)

.PHONY: all clean fclean re bonus
.SILENT: