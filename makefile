CXX := clang++
CXXFLAGS := -std=c++17 -Wall -Wextra -O2 -MMD

.PHONY: all clean

all: server client

server: server.cpp SerialPort.h
	$(CXX) $(CXXFLAGS) $< -o $@

client: client.cpp SerialPort.h
	$(CXX) $(CXXFLAGS) $< -o $@

clean:
	rm -f server client *.d   

-include *.d