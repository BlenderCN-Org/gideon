%{
  
#include "compiler/ast/ast.hpp"
#include "compiler/parser.hpp"
#include "rtlparser.hpp"
#include "rtlscanner.hpp"
#include <iostream>
  
  using namespace raytrace;
  
  int yyerror(YYLTYPE *yylloc, yyscan_t scanner, ast::gideon_parser_data *gd_data, const char *msg) {
    std::cerr << "Parser Error: " << msg << std::endl;
    std::cerr << "Column: " << yylloc->first_column << std::endl;
    std::cerr << "Line Number: " << yylloc->last_line << std::endl;
  }
 %}

%locations

%define api.pure
%lex-param { void * scanner }
%parse-param { void *scanner }
%parse-param { ast::gideon_parser_data *gd_data }

%error-verbose

%code requires {
  #ifndef RT_PARSER_NODE_DEF
  #define RT_PARSER_NODE_DEF
  #include "compiler/ast/ast.hpp"
  
  typedef struct {
    int i;
    float f;
    std::string s;
    raytrace::type_spec tspec;

    std::vector<std::string> id_list;
    
    raytrace::ast::expression_ptr expr;
    std::vector<raytrace::ast::expression_ptr> expr_list;
    
    raytrace::ast::statement_ptr stmt;
    std::vector<raytrace::ast::statement_ptr> stmt_list;
    
    raytrace::function_argument arg;
    std::vector<raytrace::function_argument> arg_list;
    raytrace::ast::prototype_ptr ptype;
    
    raytrace::ast::function_ptr func;
    
    raytrace::ast::global_declaration_ptr global;
    std::vector<raytrace::ast::global_declaration_ptr> global_list;
  } YYSTYPE;

  #endif
 }

//Define terminal symbols
%token <s> IDENTIFIER STRING_LITERAL
%token <i> INTEGER_LITERAL BOOL_LITERAL
%token <f> FLOAT_LITERAL
%token <tspec> FLOAT_TYPE INT_TYPE BOOL_TYPE VOID_TYPE STRING_TYPE

%token <tspec> RAY_TYPE INTERSECTION_TYPE
%token <tspec> LIGHT_TYPE SCENE_PTR_TYPE DISTRIBUTION_FUNC_TYPE
%token <tspec> SHADER_HANDLE_TYPE

%token <tspec> FLOAT2_TYPE FLOAT3_TYPE FLOAT4_TYPE

%token <i> DISTRIBUTION FUNCTION

%token<i> MODULE

%token <i> EXTERN
token <i> OUTPUT

%token <i> IF ELSE
%token <i> FOR
%token <i> BREAK CONTINUE
%token <i> RETURN

%token <i> IMPORT LOAD

//Operators
%right <i> '='

%left <i> '<' '>'

%left <i> '+' '-'
%left <i> '*' '/'

%left <i> '(' ')' '.'

//Non-terminals
%type <global_list> rt_file
%type <global_list> global_declarations_opt global_declarations
%type <global> global_declaration

%type <expr> module_import_path
%type <global> import_declaration
%type <global> load_declaration
%type <global> module_declaration

%type <global> function_declaration
%type <func> function_definition
%type <ptype> function_prototype external_function_declaration
%type <arg_list> function_formal_params function_formal_params_opt
%type <arg> function_formal_param

%type <global> distribution_declaration

%type <arg> distribution_param
%type <arg_list> distribution_params distribution_params_opt
%type <global_list> distribution_content_opt

%type <i> outputspec
%type <tspec> typespec
%type <tspec> simple_typename

%type <stmt_list> statement_list
%type <stmt> statement

%type <stmt> local_declaration variable_declaration
%type <stmt> conditional_statement scoped_statement

%type <stmt> loop_statement
%type <stmt> loop_mod_statement
%type <stmt> for_init_statement

%type <stmt> return_statement

%type <expr> expression binary_expression
%type <expr> assignment_expression variable_ref
%type <expr> type_constructor
%type <expr> field_selection

%type <expr> function_call
%type <expr_list> function_args_opt function_args

%start rt_file

%%

/* Basic Structure */

rt_file : global_declarations_opt { *gd_data->globals = $1; } ;

global_declarations_opt
 : global_declarations
 | { $$ = std::vector<ast::global_declaration_ptr>(); } //empty
 ;

global_declarations
 : global_declaration { if ($1) $$ = std::vector<ast::global_declaration_ptr>(1, $1); }
 | global_declarations global_declaration { $$ = $1; if ($2) $$.push_back($2); } 
 ;

global_declaration
 : function_declaration
 | typespec IDENTIFIER ';' { $$ = ast::global_declaration_ptr(new ast::global_variable_decl(gd_data->state, $2, $1)); }
 | load_declaration
 | import_declaration
 | module_declaration
 | distribution_declaration
 ;

function_declaration
 : function_definition { $$ = $1; }
 | function_prototype ';' { $$ = $1; }
 | external_function_declaration { $$ = $1; }
 ;

module_import_path
 : IDENTIFIER { $$ = ast::expression_ptr(new ast::variable_ref(gd_data->state, $1)); }
 | module_import_path '.' IDENTIFIER { $$ = ast::expression_ptr(new ast::field_selection(gd_data->state, $3, $1, yylloc.first_line, yylloc.first_column)); }
 ;

import_declaration
 : IMPORT module_import_path ';' { $$ = ast::global_declaration_ptr(new ast::import_declaration(gd_data->state, $2,
												yylloc.first_line, yylloc.first_column)); }
 ;

load_declaration
 : LOAD STRING_LITERAL ';' {
   $$ = ast::global_declaration_ptr(new ast::load_declaration(gd_data->state, $2, yylloc.first_line, yylloc.first_column));
   gd_data->dependencies->push_back($2);
 }
 ;

module_declaration
 : MODULE IDENTIFIER '{' global_declarations_opt '}' { $$ = ast::global_declaration_ptr(new ast::module(gd_data->state,
													$2, $4, yylloc.first_line, yylloc.first_column)); }
 ;

/** Functions **/

function_prototype
 : FUNCTION IDENTIFIER '(' function_formal_params_opt ')' typespec { $$ = ast::prototype_ptr(new ast::prototype(gd_data->state, $2, $6, $4)); }
 ;

external_function_declaration
 : EXTERN function_prototype ':' IDENTIFIER ';' { $$ = $2; $$->set_external($4); }
 ;

function_definition
 : function_prototype '{' statement_list '}' { $$ = ast::function_ptr(new ast::function(gd_data->state, $1, ast::statement_list($3))); }
 ;

function_formal_params_opt
 : function_formal_params
 | { } //empty
 ;

function_formal_params
 : function_formal_param { $$ = std::vector<function_argument>(1, $1); }
 | function_formal_params ',' function_formal_param { $$ = $1; $$.push_back($3); }
 ;

function_formal_param
 : outputspec typespec IDENTIFIER { $$ = {$3, $2, ($1 ? true :false)}; }
 ;

outputspec
 : OUTPUT { $$ = 1; }
 | { $$ = 0; }
 ;

simple_typename
 : FLOAT_TYPE { $$ = gd_data->state->types["float"]; }
 | FLOAT2_TYPE { $$ = gd_data->state->types["vec2"]; }
 | FLOAT3_TYPE { $$ = gd_data->state->types["vec3"]; }
 | FLOAT4_TYPE { $$ = gd_data->state->types["vec4"]; }

 | LIGHT_TYPE { $$ = gd_data->state->types["light"]; }
 | SCENE_PTR_TYPE { $$ = gd_data->state->types["scene_ptr"]; }
 | DISTRIBUTION_FUNC_TYPE { $$ = gd_data->state->types["dfunc"]; }
 | SHADER_HANDLE_TYPE { $$ = gd_data->state->types["shader_handle"]; }
 
 | RAY_TYPE { $$ = gd_data->state->types["ray"]; }
 | INTERSECTION_TYPE { $$ = gd_data->state->types["isect"]; }

 | INT_TYPE { $$ = gd_data->state->types["int"]; }
 | BOOL_TYPE { $$ = gd_data->state->types["bool"]; }
 | STRING_TYPE { $$ = gd_data->state->types["string"]; }
 | VOID_TYPE { $$ = gd_data->state->types["void"]; }
;

typespec
 : simple_typename
 ;

/* Distributions */

distribution_param
 : typespec IDENTIFIER { $$ = {$2, $1, false}; }
 ;

distribution_params
 : distribution_param { $$ = std::vector<function_argument>(1, $1); }
 | distribution_params ',' distribution_param { $$ = $1; $$.push_back($3); }
;

distribution_params_opt
 : distribution_params
 | { $$ = std::vector<function_argument>(); } //empty
 ;

distribution_content_opt
 : distribution_content_opt function_declaration { $$ = $1; $$.push_back($2); }
 | { $$ = std::vector<ast::global_declaration_ptr>(); } //empty
 ;

distribution_declaration
 : DISTRIBUTION IDENTIFIER '(' distribution_params_opt ')' '{' distribution_content_opt '}' {
   $$ = ast::global_declaration_ptr(new ast::distribution(gd_data->state, $2,
							  $4, $7,
							  yylloc.first_line, yylloc.first_column));
 }
;

/* Statements */

statement_list
 : statement_list statement { $$ = $1; if ($2) $$.push_back($2); }
 | { }
 ;

statement
 : scoped_statement
 | conditional_statement
 | local_declaration
 | loop_statement
 | loop_mod_statement
 | return_statement
 | expression ';' { $$ = ast::statement_ptr(new ast::expression_statement(gd_data->state, $1)); }
 | ';' { $$ = nullptr; }
 ;

scoped_statement
 : '{' statement_list '}' { $$ = ast::statement_ptr(new ast::scoped_statement(gd_data->state, $2)); }
 ;

conditional_statement
 : IF '(' expression ')' statement { $$ = ast::statement_ptr(new ast::conditional_statement(gd_data->state, $3, $5, nullptr)); }
 | IF '(' expression ')' statement ELSE statement { $$ = ast::statement_ptr(new ast::conditional_statement(gd_data->state, $3, $5, $7)); }
 ;

loop_mod_statement
 : BREAK ';' { $$ = ast::statement_ptr(new ast::break_statement(gd_data->state)); }
 | CONTINUE ';' { $$ = ast::statement_ptr(new ast::continue_statement(gd_data->state)); }
 ;

loop_statement
 : FOR '(' for_init_statement expression ';' expression ')' statement { $$ = ast::statement_ptr(new ast::for_loop_statement(gd_data->state, $3, $4, $6, $8)); }
 ;

for_init_statement
 : variable_declaration
 | expression ';' { $$ = ast::statement_ptr(new ast::expression_statement(gd_data->state, $1)); }
 | ';' { $$ = nullptr; }
 ;

local_declaration
 : variable_declaration
 ;

variable_declaration
 : typespec IDENTIFIER '=' expression ';' { $$ = ast::statement_ptr(new ast::variable_decl(gd_data->state, $2, $1, $4, yylloc.first_line, yylloc.first_column)); }
 | typespec IDENTIFIER ';' { $$ = ast::statement_ptr(new ast::variable_decl(gd_data->state, $2, $1, nullptr, yylloc.first_line, yylloc.first_column)); }
 ;


return_statement
 : RETURN expression ';' { $$ = ast::statement_ptr(new ast::return_statement(gd_data->state, $2)); }
 | RETURN ';' { $$ = ast::statement_ptr(new ast::return_statement(gd_data->state, nullptr)); }
 ;

expression
 : INTEGER_LITERAL { $$ = ast::expression_ptr(new ast::literal<int>(gd_data->state, $1)); }
 | FLOAT_LITERAL { $$ = ast::expression_ptr(new ast::literal<float>(gd_data->state, $1)); }
 | BOOL_LITERAL { $$ = ast::expression_ptr(new ast::literal<bool>(gd_data->state, $1)); }
 | STRING_LITERAL { $$ = ast::expression_ptr(new ast::literal<std::string>(gd_data->state, $1)); }
 | variable_ref
 | field_selection
 | assignment_expression
 | binary_expression
 | type_constructor
 | function_call
 | '(' expression ')' { $$ = $2; }
 ;

assignment_expression
 : expression '=' expression { $$ = ast::expression_ptr(new ast::assignment(gd_data->state, $1, $3)); }
 ;

type_constructor
 : typespec '(' function_args_opt ')' { $$ = ast::expression_ptr(new ast::type_constructor(gd_data->state, $1, $3)); }
 ;

variable_ref
 : IDENTIFIER { $$ = ast::expression_ptr(new ast::variable_ref(gd_data->state, $1)); }
 ;

field_selection
 : expression '.' IDENTIFIER { $$ = ast::expression_ptr(new ast::field_selection(gd_data->state, $3, $1, yylloc.first_line, yylloc.first_column)); }
 ;

function_call
 : IDENTIFIER '(' function_args_opt ')' { $$ = ast::expression_ptr(new ast::func_call(gd_data->state, nullptr, $1, $3)); }
 | expression '.' IDENTIFIER '(' function_args_opt ')' { $$ = ast::expression_ptr(new ast::func_call(gd_data->state, $1, $3, $5)); }
 ;

function_args_opt
 : function_args
 | { $$ = std::vector<ast::expression_ptr>(); }
 ;

function_args
 : function_args ',' expression { $$ = $1; $$.push_back($3); }
 | expression { $$ = std::vector<ast::expression_ptr>(1, $1); }
 ;

binary_expression
 : expression '+' expression { $$ = ast::expression_ptr(new ast::binary_expression(gd_data->state, "+", $1, $3, yylloc.first_line, yylloc.first_column)); }
 | expression '-' expression { $$ = ast::expression_ptr(new ast::binary_expression(gd_data->state, "-", $1, $3, yylloc.first_line, yylloc.first_column)); }
 | expression '*' expression { $$ = ast::expression_ptr(new ast::binary_expression(gd_data->state, "*", $1, $3, yylloc.first_line, yylloc.first_column)); }
 | expression '/' expression { $$ = ast::expression_ptr(new ast::binary_expression(gd_data->state, "/", $1, $3, yylloc.first_line, yylloc.first_column)); }
 | expression '<' expression { $$ = ast::expression_ptr(new ast::binary_expression(gd_data->state, "<", $1, $3, yylloc.first_line, yylloc.first_column)); }
 | expression '>' expression { $$ = ast::expression_ptr(new ast::binary_expression(gd_data->state, ">", $1, $3, yylloc.first_line, yylloc.first_column)); }
 ;

%%

