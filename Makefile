
FLAGS=-I lib/ -Llib/rtmidi -lrtmidi	
DEBUG_FLAGS=-ggdb3 -O0
FILES=src/lang.cpp src/rtseq.cpp build/parser.cpp build/lexer.cpp src/main.cpp

default: 
	bison -o build/parser.cpp -d parser.ypp 
	flex -o build/lexer.cpp lexer.l 
	g++ $(FLAGS) $(DEBUG_FLAGS) $(FILES)
		
