.PHONY: compile

server_OBJS := server.o
client_OBJS := client.o

compile: server client

.SECONDEXPANSION:
server client: $$($$@_OBJS)
	$(CXX) -O3 -Wall -Wextra -pedantic $< -o $@ `pkg-config --cflags --libs opencv4` 

%.o:%.cpp
	$(CXX) -O3 -Wall -Wextra -pedantic -c $< `pkg-config --cflags --libs opencv4` 

clean:
	rm $(server_OBJS) $(client_OBJS) server client
