
FLAGS=-I lib/ -fconcepts -pthread
LDFLAGS=-Llib/rtmidi -lrtmidi -pthread
DEBUG_FLAGS=-ggdb3 -O0 -DDEBUG
FILES=src/lang.cpp src/rtseq.cpp src/param.cpp build/parser.cpp build/lexer.cpp src/main.cpp
OBJECTS=$(subst src/,build/,$(FILES:.cpp=.o))

default: mirco

release: DEBUG_FLAGS =
release: mirco

clean:
	rm -r build

build/%.o: src/%.cpp
	g++ $(FLAGS) $(DEBUG_FLAGS) $^ -c -o $@

mirco: build build/parser.cpp build/lexer.cpp $(OBJECTS)
	g++ $(LDFLAGS) $(OBJECTS) -o $@

build:
	mkdir build

build/parser.cpp: src/parser.ypp
	bison -o $@ -d $^

build/lexer.cpp: src/lexer.l
	flex -o $@ $^

.PHONY: default release
