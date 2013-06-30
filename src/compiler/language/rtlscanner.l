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

  void save_string(YYSTYPE *yylval, const std::string &str) {
    yylval->s = str.substr(1, str.length()-2);
  }

%}

%option reentrant noyywrap
%option bison-bridge
%option bison-locations
%option yylineno

%x COMMENT

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

\"(\\.|[^\\"])*\" { save_string(yylval, TOKEN_STRING); return STRING_LITERAL; }
\"(\\.|[^\\"])*\" { /* dummy rule */ }
  
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

[ \t\r]      ;
\n           ;

[a-zA-Z_][a-zA-Z0-9_:]*  yylval->s = TOKEN_STRING; return IDENTIFIER;
-?[0-9]+\.[0-9]*          save_float(yylval, TOKEN_STRING); return FLOAT_LITERAL;
[0-9]+                  save_int(yylval, TOKEN_STRING); return INTEGER_LITERAL;

.                       { return yytext[0]; }

%%



#include <iostream>

int yyerror(const char *msg) { std::cerr << "Lexer Error: " << msg << std::endl; }

