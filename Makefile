CXX = g++
CXXFLAGS = -Wall -Wextra -std=c++17
SRC = $(shell find src -name '*.cpp')
BIN = bin/Sutbnet

all:
	$(CXX) $(CXXFLAGS) $(SRC) -o $(BIN)

run: all
	./$(BIN)

clean:
	rm -f $(BIN)
