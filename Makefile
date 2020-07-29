CXXFLAGS = -Wall -Wpedantic -Wextra -g 
CXX = g++

all: bin server client

server: src/server.cpp src/auxlib.cpp
	$(CXX) $(CFLAGS) -pthread $^ -o bin/$@ -lpthread

client : src/client.cpp src/auxlib.cpp
	$(CXX) $(CFLAGS) $^ -o bin/$@ -lreadline

bin:
	mkdir bin

kill:
	killall client
	killall server

clean:
	rm -r bin

