#ifndef AST_H
#define AST_H

#include <stdint.h>

#include "modc_types.h"

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

const char *ast_unop_to_string(AstUnOp op);

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
  AST_BIN_OP_LT, // a <  b
  AST_BIN_OP_LE, // a <= b
  AST_BIN_OP_GT, // a >  b
  AST_BIN_OP_GE, // a >= b

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
  AST_BIN_OP_ASSIGN,     // a  =  b
  AST_BIN_OP_ADD_ASSIGN, // a += b
  AST_BIN_OP_SUB_ASSIGN, // a -= b
  AST_BIN_OP_MUL_ASSIGN, // a *= b
  AST_BIN_OP_DIV_ASSIGN, // a /= b
  AST_BIN_OP_MOD_ASSIGN, // a %= b
  AST_BIN_OP_SHL_ASSIGN, // a <<= b
  AST_BIN_OP_SHR_ASSIGN, // a >>= b
  AST_BIN_OP_AND_ASSIGN, // a &= b
  AST_BIN_OP_XOR_ASSIGN, // a ^= b
  AST_BIN_OP_OR_ASSIGN,  // a |= b
} AstBinOp;

const char *ast_binop_to_string(AstBinOp op);

typedef enum {
  // TODO: add more ast variants
  AST_INTEGER,
  AST_FLOAT,
  AST_STRING,
  AST_IDENTIFIER,

  AST_UNOP,
  AST_BINOP,

  AST_LVAR,
  AST_GVAR,
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
  // TODO: tidy up all the ast union structs, stuff like gvar, lvar,
  // declarations and what not figure out if i want to handle variables together
  // and have a flag for global and local use a String struct instead of const
  // char * and length add support for functions and declarations and funptrs
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
      const char *data;
      size_t length;
    } string;

    // identifier
    struct {
      const char *name;
      size_t length;
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
    struct {
      const char *name;
      ModCType *decl_type;
      Ast *initializer;
      int is_global;
    } var_decl;

    // function declaration
    struct {
      const char *name;
      ModCType *func_type;
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
      Ast *expr;
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

// TODO: add and implement more to make a base with returns, function calls, etc

// Literals
Ast *ast_int64(int64_t val);
Ast *ast_float64(double val);
Ast *ast_char(int64_t ch);
Ast *ast_string(char *str, size_t len);

// Declarations
Ast *ast_decl(Ast *var, Ast *init);

// Symbol operators i.e: +-*&^>
Ast *ast_binary_op(AstBinOp operation, Ast *left, Ast *right, int *_is_err);
Ast *ast_unar_op(ModCType *type, AstUnOp operation, Ast *operand);

// Variable definitions
Ast *ast_l_var(ModCType *type, char *name, int len);
Ast *ast_g_var(ModCType *type, char *name, int len, int is_static);

#endif // !AST_H
