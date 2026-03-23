#ifndef AST_H
#define AST_H

#include <stdint.h>

#include "modc_types.h"
#include "str.h"

typedef enum {
  // postfix
  AST_UNOP_POST_INC, // x++
  AST_UNOP_POST_DEC, // x--

  // prefix
  AST_UNOP_PRE_INC, // ++x
  AST_UNOP_PRE_DEC, // --x

  // arithmetic sign
  AST_UNOP_PLUS,  // +x
  AST_UNOP_MINUS, // -x

  // logical & bitwise
  AST_UNOP_LOG_NOT, // !x
  AST_UNOP_BIT_NOT, // ~x

  // pointer & address
  AST_UNOP_ADDR_OF, // &x
  AST_UNOP_DEREF,   // *p
} AstUnOp;

String *ast_unop_to_string(AstUnOp op);

typedef enum {
  // multiplicative
  AST_BIN_OP_MUL, // a * b
  AST_BIN_OP_DIV, // a / b
  AST_BIN_OP_MOD, // a % b

  // additive
  AST_BIN_OP_ADD, // a + b
  AST_BIN_OP_SUB, // a - b

  // bit-shift
  AST_BIN_OP_SHL, // a << b
  AST_BIN_OP_SHR, // a >> b

  // relational
  AST_BIN_OP_LT, // a  <   b
  AST_BIN_OP_LE, // a  <=  b
  AST_BIN_OP_GT, // a  >   b
  AST_BIN_OP_GE, // a  >=  b

  // equality
  AST_BIN_OP_EQ, // a == b
  AST_BIN_OP_NE, // a != b

  // bitwise
  AST_BIN_OP_BIT_AND, // a & b
  AST_BIN_OP_BIT_XOR, // a ^ b
  AST_BIN_OP_BIT_OR,  // a | b

  // logical
  AST_BIN_OP_LOG_AND, // a && b
  AST_BIN_OP_LOG_OR,  // a || b

  // assignment (simple + compound)
  AST_BIN_OP_ASSIGN,     // a   =  b
  AST_BIN_OP_ADD_ASSIGN, // a  +=  b
  AST_BIN_OP_SUB_ASSIGN, // a  -=  b
  AST_BIN_OP_MUL_ASSIGN, // a  *=  b
  AST_BIN_OP_DIV_ASSIGN, // a  /=  b
  AST_BIN_OP_MOD_ASSIGN, // a  %=  b
  AST_BIN_OP_SHL_ASSIGN, // a  <<= b
  AST_BIN_OP_SHR_ASSIGN, // a  >>= b
  AST_BIN_OP_AND_ASSIGN, // a  &=  b
  AST_BIN_OP_XOR_ASSIGN, // a  ^=  b
  AST_BIN_OP_OR_ASSIGN,  // a  |=  b
} AstBinOp;

String *ast_binop_to_string(AstBinOp op);

typedef enum {
  // TODO: add more ast variants
  AST_INTEGER,
  AST_FLOAT,
  AST_STRING,
  AST_IDENTIFIER,

  AST_UNOP,
  AST_BINOP,

  AST_VAR,
  AST_FUNC,
  AST_FUNCALL,
  AST_FUNPTR,
  AST_FUNPTR_CALL,

  AST_BLOCK,
  AST_IF,
  AST_FOR,
  AST_WHILE,
  AST_RETURN,
  AST_BREAK,
  AST_CONTINUE,

  AST_SWITCH,
  AST_CASE,
  AST_JUMP,
} AstVariant;

typedef struct Ast Ast;

struct Ast {
  // TODO: add support for functions and declarations and funptrs
  // and calling functions
  AstVariant variant;
  ModCType *type;
  void *symbol;

  union {
    // literals
    struct {
      int64_t value;
    } number;

    struct {
      double value;
    } fvalue;

    struct {
      String *value; // TODO: Maybe change to String_View
    } string;

    // identifier
    struct {
      String *name; // TODO: Maybe change to String_View
    } ident;

    // unary
    struct {
      AstUnOp unop;
      Ast *expr;
    } unary;

    // binary
    struct {
      AstBinOp binop;
      Ast *left;
      Ast *right;
    } binary;

    // variable declaration
    // e.g. int x = 10;
    struct {
      String *name; // TODO: Maybe change to String_View
      ModCType *type;
      Ast *initializer;
    } var_decl;

    // function declaration
    struct {
      String *name; // TODO: Maybe change to String_View
      ModCType *return_type;
      Ast **params;
      size_t param_count;
      Ast *body;
    } func_decl;

    // function call
    struct {
      Ast *callee;
      Ast **args;
      size_t arg_count;
    } func_call;

    // block
    struct {
      Ast **statements;
      size_t stmt_count;
    } block;

    // if
    struct {
      Ast *cond;
      Ast *then;
      Ast *els;
    } if_stmt;

    // for
    struct {
      Ast *forinit;
      Ast *forcond;
      Ast *forstep;
      Ast *forbody;
    } for_stmt;

    // while
    struct {
      Ast *whilecond;
      Ast *whilebody;
    } while_stmt;

    // return
    struct {
      Ast *expr;
    } return_stmt;

    // switch
    struct {
      Ast *switch_expr;
      Ast **cases;
      size_t case_count;
      Ast *default_case;
    } switch_stmt;

    // case
    struct {
      long long value;
      Ast *body;
    } case_stmt;
  };
};

Ast *ast_create(void);

void ast_free(Ast *ast);

// TODO: add and implement more to make a base with returns, function calls, etc

// Literals
Ast *ast_int64(int64_t val);
Ast *ast_float64(double val);
Ast *ast_char(int64_t ch);
Ast *ast_string(char *str, size_t len);

Ast *ast_ident(char *str, size_t len);

// Symbol operators i.e: +-*&^>
Ast *ast_binop(AstBinOp op, Ast *left, Ast *right, int *_is_err);
Ast *ast_unop(AstUnOp op, Ast *expr);

// Declarations
Ast *ast_vardecl(char *name, size_t len, ModCType *type, Ast *init);
Ast *ast_funcdecl(char *name, size_t len, ModCType *type, Ast **params,
                  size_t param_count, Ast *body);

Ast *ast_funccall(Ast *callee, Ast **args, size_t arg_count);

Ast *ast_block(Ast **statements, size_t stmt_count);
Ast *ast_if(Ast *cond, Ast *then, Ast *els);
Ast *ast_for(Ast *forinit, Ast *forcond, Ast *forstep, Ast *forbody);
Ast *ast_while(Ast *whilecond, Ast *whilebody);
Ast *ast_return(Ast *expr);
Ast *ast_switch(Ast *switch_expr, Ast **cases, size_t case_count,
                Ast *default_case);
Ast *ast_case(long long value, Ast *body);

#endif // !AST_H
