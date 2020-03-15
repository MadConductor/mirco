%{
  #include <iostream>
  #include <map>
  #include "../src/lang.hpp"
  #include "parser.hpp"
%}

%option yylineno
%option noyywrap

DIGIT [0-9]
INT [1-9]{DIGIT}*
NOTE [ABCDEFGHabcdefgh](#|b)?[0-9](\:[0-9]{1,3})?
OPERATOR [\+\-\*\\\=]
SEMITONE {INT}[s]
OCTAVE {INT}[o]
TONE_LITERAL {OCTAVE}|{SEMITONE}|{OCTAVE}{SEMITONE}

%%

[ \t\n]         ;

sequence        { return SEQUENCE; }
default           { return DEFAULT; }
auto            { return AUTO; }

{TONE_LITERAL} {
  Tone *tone = new Tone(yytext);
  yylval.toneval = tone; 
  return TONE_LITERAL;
}

{NOTE}    {
  Note *note = new Note(yytext);
  yylval.noteval = note;
  return NOTE;
}

[a-zA-Z][a-zA-Z0-9]*    { 
  Identifier *id = new Identifier(yytext);
  yylval.idval = id;
  return IDENTIFIER;
}

\"[a-zA-Z0-9]*\"    { 
  yylval.sval = strdup(yytext);
  return STRING;
}

{INT}?\.{DIGIT}+  { 
  yylval.fval = atof(yytext);
  return FLOAT;
}

{INT}          { 
  yylval.ival = atoi(yytext); 
  return INT;
}

[\(\)\{\}:;,]    { return yytext[0]; }

{OPERATOR} { 
  yylval.sval = strdup(yytext); 
  return OPERATOR;
}

%%