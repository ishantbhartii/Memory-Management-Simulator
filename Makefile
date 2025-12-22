CXX=g++
CXXFLAGS=-std=c++17 -O2 -Iinclude -Wall -Wextra
SRC=src/main.cpp
OBJ=build/main.o
TARGET=memsim

all: $(TARGET)

$(TARGET): $(OBJ)
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(OBJ)

$(OBJ): $(SRC)
	mkdir -p build
	$(CXX) $(CXXFLAGS) -c $(SRC) -o $(OBJ)

.PHONY: clean
clean:
	rm -rf build $(TARGET)
