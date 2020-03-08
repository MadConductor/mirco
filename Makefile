
default: 
	bison -o build/parser.cpp -d parser.ypp 
	flex -o build/lexer.cpp lexer.l 
	g++ src/lang.cpp build/parser.cpp build/lexer.cpp src/main.cpp -ggdb3 -O0
