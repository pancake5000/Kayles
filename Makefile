CXX = g++
CXXFLAGS = -Wall -Wextra -std=c++17 -O2 -MMD -MP

.PHONY: all clean kayles_client kayles_server

all: kayles_client kayles_server

CLIENT_OBJS = Client.o ArgParser.o GameState.o

kayles_client: $(CLIENT_OBJS)
	$(CXX) $(CXXFLAGS) -o kayles_client $(CLIENT_OBJS)

kayles_server:
	# TODO: server is not implemented yet, compile it here later

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -f *.o kayles_client kayles_server