CXX = g++
CXXFLAGS = -Wall -Wextra -std=c++17 -O2 -MMD -MP

.PHONY: all clean kayles_client kayles_server

all: kayles_client kayles_server

CLIENT_OBJS = Client.o ArgParser.o GameState.o
SERVER_OBJS = Server.o ArgParser.o GameState.o
DEPS = $(CLIENT_OBJS:.o=.d) $(SERVER_OBJS:.o=.d)

kayles_client: $(CLIENT_OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $(CLIENT_OBJS)

kayles_server: $(SERVER_OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $(SERVER_OBJS)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -f *.o *.d kayles_client kayles_server

-include $(DEPS)