%{
#include <string>
#include "rtlparser.hpp"
#include <boost/lexical_cast.hpp>
  
#define TOKEN_STRING (std::string(yytext, yyleng))

  void save_float(YYSTYPE *yylval, const std::string &str) {
    yylval->f = boost::lexical_cast<float>(str);
  }

  void save_int(YYSTYPE *yylval, const std::string &str) {
    yylval->i = boost::lexical_cast<int>(str);
  }

%}

%option reentrant noyywrap
%option bison-bridge
%option bison-locations

%%

"void" { return VOID_TYPE; }
"bool" { return BOOL_TYPE; }
"int" { return INT_TYPE; }
"float" { return FLOAT_TYPE; }
"vec4" { return FLOAT4_TYPE; }
"ray" { return RAY_TYPE; }

"extern" { return EXTERN; }
"output" { return OUTPUT; }
"return" { return RETURN; }
"if" { return IF; }
"else" { return ELSE; }
"for" { return FOR; }
"break" { return BREAK; }
"continue" { return CONTINUE; }

"true" { yylval->i = 1; return BOOL_LITERAL; }
"false" { yylval->i = 0; return BOOL_LITERAL; }



[ \t\n]      ;
[a-zA-Z_][a-zA-Z0-9_]*  yylval->s = TOKEN_STRING; return IDENTIFIER;
-?[0-9]+\.[0-9]*          save_float(yylval, TOKEN_STRING); return FLOAT_LITERAL;
[0-9]+                  save_int(yylval, TOKEN_STRING); return INTEGER_LITERAL;
.                       { return yytext[0]; }

%%

#include <iostream>

int yyerror(const char *msg) { std::cerr << "Lexer Error: " << msg << std::endl; }

