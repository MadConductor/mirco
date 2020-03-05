
#include "stdio.h"
#include <map>
#include <vector>
#include "engine.hpp"

void yyparse();
extern FILE *yyin;

std::map<int, Sequence> midiMap;
std::map<std::string, Sequence*> scope;

int main(int, char**) {
  // open a file handle to a particular file:
  FILE *myfile = fopen("syntax.txt", "r");
  // make sure it's valid:
  if (!myfile) {
    printf("I can't open syntax.txt!");
    return -1;
  }
  // Set lex to read from it instead of defaulting to STDIN:
  yyin = myfile;

  // Parse through the input:
  yyparse();
}

