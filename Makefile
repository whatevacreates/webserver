NAME = webserv

CXX = c++
CXXFLAGS = -Wall -Wextra -Werror -std=c++98 -Iinclude -Itests/include -g

SRCS = $(wildcard src/**/*.cpp src/*.cpp)
TEST_SRCS = $(wildcard tests/*.cpp)
OBJS = $(SRCS:.cpp=.o) $(TEST_SRCS:.cpp=.o)

all: $(NAME)

$(NAME): bin $(OBJS)
	$(CXX) $(CXXFLAGS) -o $(NAME) $(OBJS)

bin:
	mkdir -p bin

clean:
	rm -f $(OBJS)

fclean: clean
	rm -f $(NAME)

re: fclean all