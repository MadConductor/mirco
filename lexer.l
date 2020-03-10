%{
  #include <iostream>
  #include <map>
  #include "parser.hpp"
  using namespace std;

%}

%option yylineno
%option noyywrap

DIGIT [0-9]
NOTE [ABCDEFGHabcdefgh](#|b)?[0-9](\:[0-9]{1,3})?

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
  yylval.nval = yytext; 
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