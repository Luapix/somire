
CC     := g++
CFLAGS := -std=c++11 -Wall -Wextra -pedantic -Ic:/lib/utf8-cpp-2.3.4

SRC_FILES := $(wildcard src/*.cpp)
OBJ_FILES := $(patsubst src/%.cpp,build/%.o,$(SRC_FILES))

run: parser.exe input.txt
	./parser.exe input.txt

clean:
	rm -f build/*
	rm -f parser.exe

debug: CFLAGS := -g $(CFLAGS)
debug: parser.exe

$(OBJ_FILES): build/%.o : src/%.cpp src/*.hpp
	$(CC) $(CFLAGS) -c $< -o $@

parser.exe: $(OBJ_FILES)
	$(CC) build/*.o -o parser.exe

src/uni_data.cpp: tools/gen_uni_data.py tools/ppucd.txt
	cd tools; python gen_uni_data.py
	cp tools/uni_data.cpp src/uni_data.cpp
