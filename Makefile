
FLAGS=-I lib/ -Llib/rtmidi -lrtmidi -fconcepts -pthread
DEBUG_FLAGS=-ggdb3 -O0
FILES=src/lang.cpp src/rtseq.cpp build/parser.cpp build/lexer.cpp src/main.cpp

default: build build/parser.cpp build/lexer.cpp
	g++ $(FLAGS) $(DEBUG_FLAGS) $(FILES) -o mirco

build:
	mkdir build

build/parser.cpp: src/parser.ypp
	bison -o $@ -d $^

build/lexer.cpp: src/lexer.l
	flex -o $@ $^
