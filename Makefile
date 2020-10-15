CXXFLAGS = -Wall -Wpedantic -Wextra -g 
CXX = g++

all: bin server client

server: server.cpp auxlib.cpp
	$(CXX) $(CFLAGS) -pthread $^ -o bin/$@ -lpthread

client : client.cpp auxlib.cpp
	$(CXX) $(CFLAGS) $^ -o bin/$@ -lreadline

bin:
	mkdir bin

kill:
	killall client
	killall server

clean:
	rm -r bin

