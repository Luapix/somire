
CC     := g++
CFLAGS := -std=c++11 -Wall -Wextra -pedantic -Ic:/lib/utf8-cpp-2.3.4

SRC_FILES := $(wildcard src/*.cpp)
OBJ_FILES := $(patsubst src/%.cpp,build/%.o,$(SRC_FILES))
OUTPUT := somire.exe

build: $(OUTPUT)

test: $(OUTPUT) input.txt
	./$(OUTPUT) compile input.txt
	./$(OUTPUT) run input.out

clean:
	rm -f build/*
	rm -f $(OUTPUT)

debug: CFLAGS := -g $(CFLAGS)
debug: $(OUTPUT)

debug-gc: CFLAGS := -DDEBUG_GC $(CFLAGS)
debug-gc: $(OUTPUT)

$(OBJ_FILES): build/%.o : src/%.cpp src/*.hpp src/*.tpp
	$(CC) $(CFLAGS) -c $< -o $@

$(OUTPUT): $(OBJ_FILES)
	$(CC) build/*.o -o $(OUTPUT)

src/uni_data.cpp: tools/gen_uni_data.py tools/ppucd.txt
	cd tools; python gen_uni_data.py
	cp tools/uni_data.cpp src/uni_data.cpp
