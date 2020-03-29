%{
  #include <iostream>
  #include <map>
  #include "../src/lang.hpp"
  #include "parser.hpp"
  #include "yytype.hpp"

  #define YY_EXTRA_TYPE struct scanner_extra_data*

  #define YY_INPUT(buf, result, max_size)           \
    do {                                            \
      auto c = yyextra->stream->get();              \
      int n = 0;                                    \
      if (c)                                        \
        buf[n++] = (char)c;                         \
      if(!yyextra->stream->good()) break;           \
      result = n;                                   \
    } while (0)
%}

%option yylineno
%option noyywrap

DIGIT [0-9]
INT [1-9]{DIGIT}*
IDENTIFIER [a-zA-Z][a-zA-Z0-9_\-]* 
NOTE [ABCDEFGHabcdefgh](#|b)?[0-9]
PAUSE [_]
OPERATOR [\+\-\*\/\=]
SEMITONE {INT}[s]
OCTAVE {INT}[o]
TONE_LITERAL {OCTAVE}|{SEMITONE}|{OCTAVE}{SEMITONE}

%%

[ \t\n]         ;
"//".*  ;
[/][*][^*]*[*]+([^*/][^*]*[*]+)*[/] ;

sequence { return SEQUENCE; }
default  { return DEFAULT; }
auto     { return AUTO; }

{PAUSE} {
  yylval.sval = strdup(yytext);
  return PAUSE;
}

{TONE_LITERAL} {
  Tone *tone = new Tone(yytext);
  yylval.toneval = tone; 
  return TONE_LITERAL;
}

{NOTE}    {
  yylval.sval = strdup(yytext);
  return NOTE;
}

\${IDENTIFIER}   { 
  RtResource *rr = new RtResource(strdup(yytext));
  yylval.rtresourceval = rr;
  return RTRESOURCE;
}

{IDENTIFIER}   { 
  Identifier *id = new Identifier(strdup(yytext));
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

[\(\)\{\}\|:;,<>]    { return yytext[0]; }

{OPERATOR} { 
  yylval.sval = strdup(yytext); 
  return OPERATOR;
}

%%
