
FLAGS=-I lib/ -Llib/rtmidi -lrtmidi -fconcepts -pthread
DEBUG_FLAGS=-ggdb3 -O0
FILES=src/lang.cpp src/rtseq.cpp build/parser.cpp build/lexer.cpp src/main.cpp

default: 
	bison -o build/parser.cpp -d src/parser.ypp 
	flex -o build/lexer.cpp src/lexer.l 
	g++ $(FLAGS) $(DEBUG_FLAGS) $(FILES) -o mirco
		
