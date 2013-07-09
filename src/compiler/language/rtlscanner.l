/*

  Copyright 2013 Curtis Andrus

  This file is part of Gideon.

  Gideon is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.
  
  Gideon is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.
  
  You should have received a copy of the GNU General Public License
  along with Gideon.  If not, see <http://www.gnu.org/licenses/>.

*/

%{
#include <string>
#include "rtlparser.hpp"
#include <boost/lexical_cast.hpp>
  
#define TOKEN_STRING (std::string(yytext, yyleng))
  
#define YY_USER_ACTION yylloc->first_line = yylloc->last_line = yylineno; \
  yylloc->first_column = yycolumn; yylloc->last_column = yycolumn+yyleng-1; \
  yycolumn += yyleng;

  void save_float(YYSTYPE *yylval, const std::string &str) {
    yylval->f = boost::lexical_cast<float>(str);
  }

  void save_int(YYSTYPE *yylval, const std::string &str) {
    yylval->i = boost::lexical_cast<int>(str);
  }

  void save_string(YYSTYPE *yylval) {
    yylval->strbuf.push_back('\0'); //null-terminator
    yylval->s = std::string(yylval->strbuf.data());
  }

  void reset_strbuf(YYSTYPE *yylval) {
    yylval->strbuf.clear();
  }

%}

%option reentrant noyywrap
%option bison-bridge
%option bison-locations
%option yylineno

%x COMMENT
%x STRING

%%

<INITIAL>{
  "/*" BEGIN(COMMENT);
 }
<COMMENT>{
  "*/" BEGIN(INITIAL);
  [^*\n]+   // eat comment in chunks
  "*"       // eat the lone star
  \n
}

\/\/.*\n     ; //comments

  
<INITIAL>{
  "\"" { BEGIN(STRING); reset_strbuf(yylval); }
}

<STRING>"\"" {
  BEGIN(INITIAL);
  save_string(yylval);
  return STRING_LITERAL;
}
<STRING>"\\\"" { yylval->strbuf.push_back('"'); }
<STRING>"\\'" { yylval->strbuf.push_back('\''); }
<STRING>"\\a" { yylval->strbuf.push_back('\a'); }
<STRING>"\\b" { yylval->strbuf.push_back('\b'); }
<STRING>"\\f" { yylval->strbuf.push_back('\f'); }
<STRING>"\\n" { yylval->strbuf.push_back('\n'); }
<STRING>"\\r" { yylval->strbuf.push_back('\r'); }
<STRING>"\\t" { yylval->strbuf.push_back('\t'); }
<STRING>"\\v" { yylval->strbuf.push_back('\v'); }
<STRING>"\\\\" { yylval->strbuf.push_back('\\'); }
<STRING>\\. { yylval->strbuf.push_back(yytext[1]); }
<STRING>. { yylval->strbuf.push_back(yytext[0]); }


"void" { return VOID_TYPE; }
"bool" { return BOOL_TYPE; }
"int" { return INT_TYPE; }
"float" { return FLOAT_TYPE; }
"string" { return STRING_TYPE; }

"vec2" { return FLOAT2_TYPE; }
"vec3" { return FLOAT3_TYPE; }
"vec4" { return FLOAT4_TYPE; }

"ray" { return RAY_TYPE; }
"isect" { return INTERSECTION_TYPE; }

"light" { return LIGHT_TYPE; }
"scene" { return SCENE_PTR_TYPE; }
"dfunc" { return DISTRIBUTION_FUNC_TYPE; }
"shader_handle" { return SHADER_HANDLE_TYPE; }
"shader_flag" { return SHADER_FLAG_TYPE; }

"distribution" { return DISTRIBUTION; }
"function" { return FUNCTION; }
"module" { return MODULE; }

"extern" { return EXTERN; }
"output" { return OUTPUT; }
"entry" { return ENTRY; }
"material" { return MATERIAL; }

"return" { return RETURN; }
"if" { return IF; }
"else" { return ELSE; }
"for" { return FOR; }
"break" { return BREAK; }
"continue" { return CONTINUE; }

"import" { return IMPORT; }
"load" { return LOAD; }

"true" { yylval->i = 1; return BOOL_LITERAL; }
"false" { yylval->i = 0; return BOOL_LITERAL; }

"&&" { return AND_OP; }

"==" { return CMP_EQ; }
"+=" { return ADD_ASSIGN; }
"-=" { return SUB_ASSIGN; }
"*=" { return MUL_ASSIGN; }
"/=" { return DIV_ASSIGN; }

"++" { return INCR_OP; }
"--" { return DECR_OP; }

[ \t\r]      ;
\n           ;

[a-zA-Z_][a-zA-Z0-9_:]*  yylval->s = TOKEN_STRING; return IDENTIFIER;
-?[0-9]+\.[0-9]*          save_float(yylval, TOKEN_STRING); return FLOAT_LITERAL;
[0-9]+                  save_int(yylval, TOKEN_STRING); return INTEGER_LITERAL;

.                       { return yytext[0]; }

%%



#include <iostream>

int yyerror(const char *msg) { std::cerr << "Lexer Error: " << msg << std::endl; }

