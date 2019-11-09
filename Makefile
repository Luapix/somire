
CC     := g++
CFLAGS := -O3 -MD -MP -std=c++11 -Wall -Wextra -pedantic -Wno-unused \
	-Ic:/lib/utf8-cpp-2.3.4

SRC_FILES := $(wildcard src/*.cpp) src/uni_data.cpp
OBJ_FILES := $(patsubst src/%.cpp,build/%.o,$(SRC_FILES))
OUTPUT := somire.exe

PYTHON3_CMD := python
TIME_CMD := /c/msys64/usr/bin/time -p -q

build: $(OUTPUT)

test: $(OUTPUT) input.txt
	./$(OUTPUT) compile input.txt
	$(TIME_CMD) ./$(OUTPUT) run input.out

clean:
	rm -rf build
	mkdir build
	rm -f $(OUTPUT)

debug: CFLAGS := -g $(CFLAGS)
debug: $(OUTPUT)

debug-gc: CFLAGS := -DDEBUG_GC $(CFLAGS)
debug-gc: $(OUTPUT)

build/%.o: src/%.cpp src/*.hpp src/*.tpp
	$(CC) $(CFLAGS) -c $< -o $@

$(OUTPUT): $(OBJ_FILES)
	$(CC) build/*.o -o $(OUTPUT)

src/uni_data.cpp: tools/gen_uni_data.py tools/ppucd.txt
	cd tools; $(PYTHON3_CMD) gen_uni_data.py
	cp tools/uni_data.cpp src/uni_data.cpp
