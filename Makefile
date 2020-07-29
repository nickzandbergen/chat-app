CXXFLAGS = -Wall -Wpedantic -Wextra -g 
CXX = g++

all: bin server client

server: src/server.cpp src/nzandber_lib.cpp
	$(CXX) $(CFLAGS) -pthread $^ -o bin/$@ -lpthread

client : src/client.cpp src/nzandber_lib.cpp
	$(CXX) $(CFLAGS) $^ -o bin/$@ -lreadline

bin:
	mkdir bin

kill:
	killall client
	killall server

clean:
	rm -r bin
tar:
	tar -cvzf asg5.tar.gz $(git ls-files)

