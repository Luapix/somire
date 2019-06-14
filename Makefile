
CFLAGS := -std=c++11 -Wall -Wextra -pedantic -Ic:/lib/utf8-cpp-2.3.4/src

FILES  := main uni_data ast
OBJ    := $(addprefix build/, $(addsuffix .o, $(FILES)))

run: build/parser.exe input.txt
	build/parser.exe input.txt

$(OBJ): build/%.o : src/%.cpp src/*.hpp
	g++ $(CFLAGS) -c $< -o $@

build/parser.exe: $(OBJ)
	g++ build/*.o -o build/parser.exe

src/uni_data.cpp: tools/gen_uni_data.py tools/ppucd.txt
	cd tools; python gen_uni_data.py
	cp tools/uni_data.cpp src/uni_data.cpp
