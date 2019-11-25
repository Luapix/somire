
CC     := g++
CFLAGS := -MD -MP -std=c++17 -Ic:/lib/utf8-cpp-2.3.4 -Isrc
LDFLAGS := 

SRC_FILES := $(wildcard src/*/*.cpp) src/main.cpp src/util/uni_data.cpp
OBJ_FILES := $(patsubst src/%.cpp,build/%.o,$(SRC_FILES))
OUTPUT := somire.exe

PYTHON3_CMD := python
TIME_CMD := /c/msys64/usr/bin/time -p -q

build: $(OUTPUT)

test: $(OUTPUT) test.smr
	./$(OUTPUT) compile test.smr
	$(TIME_CMD) ./$(OUTPUT) run test.sbf

clean:
	rm -rf build
	mkdir build
	mkdir build/util
	mkdir build/parser
	mkdir build/compiler
	mkdir build/vm
	rm -f $(OUTPUT)

release: CFLAGS := -O3 $(CFLAGS)
release: $(OUTPUT)

debug: CFLAGS := -g $(CFLAGS)
debug: $(OUTPUT)

debug-gc: CFLAGS := -DDEBUG_GC $(CFLAGS)
debug-gc: $(OUTPUT)

profile: CFLAGS := -O3 -pg $(CFLAGS)
profile: LDFLAGS := -O3 -pg $(CFLAGS)
profile: $(OUTPUT)

build/%.o: src/%.cpp src/*/*.hpp src/*/*.tpp
	$(CC) $(CFLAGS) -c $< -o $@

$(OUTPUT): $(OBJ_FILES)
	$(CC) $(LDFLAGS) build/*/*.o build/main.o -o $(OUTPUT)

src/uni_data.cpp: tools/gen_uni_data.py tools/ppucd.txt
	cd tools; $(PYTHON3_CMD) gen_uni_data.py
	cp tools/uni_data.cpp src/uni_data.cpp
