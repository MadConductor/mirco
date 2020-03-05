%{
  #include <iostream>
  #include <map>
  #include "parser.hpp"
  using namespace std;

  std::map<string, int> noteMap = {
		{"C",0},
		{"D",2},
		{"E",4},
		{"F",5},
		{"G",7},
		{"A",9},
		{"B",11},
		{"H",11}
  };

  int parseNote(string s) {
    int noteVal = noteMap[s.substr(0,1)];
    int secIdx = 1;
    string secString = s.substr(secIdx,1);

    if (secString == "#") {
      noteVal += 1;
      secIdx++;
    }
    if (secString == "b") {
      noteVal -= 1;
      secIdx++;
    }

    int octave = 1 + std::stoi(s.substr(secIdx, 1));
    int octaveOffset = 12 * octave;
    return noteVal + octaveOffset;
  }
%}

%option yylineno
%option noyywrap

DIGIT [0-9]
NOTE [ABCDEFGHabcdefgh](#|b)?[0-9]

%%

[ \t\n]         ;

sequence        { return SEQUENCE; }
default           { return DEFAULT; }
auto            { return AUTO; }

{DIGIT}*\.{DIGIT}+  { 
  yylval.fval = atof(yytext);
  return FLOAT;
}

[1-9]{DIGIT}+          { 
  yylval.ival = atoi(yytext); 
  return INT;
}

{NOTE}    {
  yylval.nval = parseNote(yytext); 
  return NOTE;
}

[a-zA-Z][a-zA-Z0-9]*    { 
  yylval.idval = strdup(yytext);
  return IDENTIFIER;
}

\"[a-zA-Z0-9]*\"    { 
  yylval.sval = strdup(yytext);
  return STRING;
}

[\(\)\{\}:;=,]    { return yytext[0]; }

%%