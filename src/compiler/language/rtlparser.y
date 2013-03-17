%{
  
#include "compiler/ast/ast.hpp"
#include "rtlparser.hpp"
#include "rtlscanner.hpp"
#include <iostream>
  
  using namespace raytrace;

  struct rtl_parse_data {
    var_symbol_table *variables;
    func_symbol_table *functions;
    control_state *control;
    std::vector<ast::global_declaration_ptr> *ast;
  };
  
  int yyerror(YYLTYPE *yylloc, yyscan_t scanner, rtl_parse_data *rtl_data, const char *msg) {
    std::cerr << "Parser Error: " << msg << std::endl;
    std::cerr << "Column: " << yylloc->first_column << std::endl;
  }
 %}

%locations

%define api.pure
%lex-param { void * scanner }
%parse-param { void *scanner }
%parse-param { rtl_parse_data *rtl_data }

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
    
    raytrace::ast::expression_ptr expr;
    std::vector<raytrace::ast::expression_ptr> expr_list;
    
    raytrace::ast::lvalue_ptr lval;
    
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
%token <tspec> FLOAT_TYPE INT_TYPE BOOL_TYPE RAY_TYPE VOID_TYPE
%token <tspec> FLOAT4_TYPE

%token <i> EXTERN
token <i> OUTPUT

%token <i> IF ELSE
%token <i> FOR
%token <i> BREAK CONTINUE
%token <i> RETURN

//Operators
%right <i> '='
%left <i> '+'
%left <i> '<'
%left <i> '(' ')'

//Non-terminals
%type <global_list> rt_file
%type <global_list> global_declarations_opt global_declarations
%type <global> global_declaration

%type <func> function_definition
%type <ptype> function_prototype external_function_declaration
%type <arg_list> function_formal_params function_formal_params_opt
%type <arg> function_formal_param

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

%type <expr> function_call
%type <expr_list> function_args_opt function_args

%type <lval> variable_lvalue

%start rt_file

%%

/* Basic Structure */

rt_file : global_declarations_opt { *rtl_data->ast = $1; } ;

global_declarations_opt
 : global_declarations
 | { } //empty
 ;

global_declarations
 : global_declaration { $$ = std::vector<ast::global_declaration_ptr>(1, $1); }
 | global_declarations global_declaration { $$ = $1; $$.push_back($2); } 
 ;

global_declaration
 : function_definition { $$ = $1; }
 | function_prototype ';' { $$ = $1; }
 | external_function_declaration { $$ = $1; }
 ;


/** Functions **/

function_prototype
 : typespec IDENTIFIER '(' function_formal_params_opt ')' { $$ = ast::prototype_ptr(new ast::prototype($2, $1, $4, rtl_data->functions)); }
;

external_function_declaration
 : EXTERN function_prototype ':' IDENTIFIER ';' { $$ = $2; $$->set_external($4); }
 ;

function_definition
 : function_prototype '{' statement_list '}' { $$ = ast::function_ptr(new ast::function($1, ast::statement_list($3), rtl_data->variables, rtl_data->control)); }
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
 : FLOAT_TYPE { $$ = { type_code::FLOAT }; }
 | FLOAT4_TYPE { $$ = { type_code::FLOAT4 }; }
 | INT_TYPE { $$ = { type_code::INT }; }
 | BOOL_TYPE { $$ = { type_code::BOOL }; }
 | VOID_TYPE { $$ = { type_code::VOID }; }
;

typespec
 : simple_typename
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
 | expression ';' { $$ = ast::statement_ptr(new ast::expression_statement($1)); }
 | ';' { $$ = nullptr; }
 ;

scoped_statement
 : '{' statement_list '}' { $$ = ast::statement_ptr(new ast::scoped_statement($2, rtl_data->variables, rtl_data->functions)); }
 ;

conditional_statement
 : IF '(' expression ')' statement { $$ = ast::statement_ptr(new ast::conditional_statement($3, $5, nullptr)); }
 | IF '(' expression ')' statement ELSE statement { $$ = ast::statement_ptr(new ast::conditional_statement($3, $5, $7)); }
 ;

loop_mod_statement
 : BREAK ';' { $$ = ast::statement_ptr(new ast::break_statement(rtl_data->control)); }
 | CONTINUE ';' { $$ = ast::statement_ptr(new ast::continue_statement(rtl_data->control)); }
 ;

loop_statement
 : FOR '(' for_init_statement expression ';' expression ')' statement { $$ = ast::statement_ptr(new ast::for_loop_statement($3, $4, $6, $8,
															    rtl_data->variables, rtl_data->functions,
															    rtl_data->control)); }
 ;

for_init_statement
 : variable_declaration
 | expression ';' { $$ = ast::statement_ptr(new ast::expression_statement($1)); }
 | ';' { $$ = nullptr; }
 ;

local_declaration
 : variable_declaration
 ;

variable_declaration
 : typespec IDENTIFIER '=' expression ';' { $$ = ast::statement_ptr(new ast::variable_decl(rtl_data->variables, $2, $1, $4)); }
 | typespec IDENTIFIER ';' { $$ = ast::statement_ptr(new ast::variable_decl(rtl_data->variables, $2, $1, nullptr)); }
 ;


return_statement
 : RETURN expression ';' { $$ = ast::statement_ptr(new ast::return_statement($2, rtl_data->control)); }
 | RETURN ';' { $$ = ast::statement_ptr(new ast::return_statement(nullptr, rtl_data->control)); }
 ;

expression
 : INTEGER_LITERAL { $$ = ast::expression_ptr(new ast::literal<int>($1)); }
 | FLOAT_LITERAL { $$ = ast::expression_ptr(new ast::literal<float>($1)); }
 | BOOL_LITERAL { $$ = ast::expression_ptr(new ast::literal<bool>($1)); }
 | assignment_expression
 | binary_expression
 | type_constructor
 | function_call
 | '(' expression ')' { $$ = $2; }
 | variable_ref
 ;

assignment_expression
 : variable_lvalue '=' expression { $$ = ast::expression_ptr(new ast::assignment($1, $3)); }
 ;

type_constructor
 : typespec '(' function_args_opt ')' { $$ = ast::expression_ptr(new ast::type_constructor($1, $3)); }
 ;

variable_lvalue
 : IDENTIFIER { $$ = ast::lvalue_ptr(new ast::variable_lvalue(rtl_data->variables, $1)); }
 ;

variable_ref
 : variable_lvalue { $$ = ast::expression_ptr(new ast::variable_ref($1)); }
 ;

function_call
 : IDENTIFIER '(' function_args_opt ')' { $$ = ast::expression_ptr(new ast::func_call($1, $3, rtl_data->functions)); }
 ;

function_args_opt
 : function_args
 | { }
 ;

function_args
 : function_args ',' expression { $$ = $1; $$.push_back($3); }
 | expression { $$ = std::vector<ast::expression_ptr>(1, $1); }
 ;

binary_expression
 : expression '+' expression { $$ = ast::expression_ptr(new ast::binary_expression("+", $1, $3)); }
 | expression '<' expression { $$ = ast::expression_ptr(new ast::binary_expression("<", $1, $3)); }
 ;

%%

