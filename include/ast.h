#ifndef AST_H
#define AST_H

#include <stdint.h>

#include "lexer.h"
#include "str.h"
#include "types.h"
#include "vec.h"

typedef struct ModCType ModCType;
typedef struct Scope Scope;

typedef enum {
  // postfix
  AST_UNOP_POST_INC = 1, // x++
  AST_UNOP_POST_DEC = 3, // x--

  // prefix
  AST_UNOP_PRE_INC = 5, // ++x
  AST_UNOP_PRE_DEC = 7, // --x

  // arithmetic sign
  AST_UNOP_PLUS = 9,   // +x
  AST_UNOP_MINUS = 11, // -x

  // logical & bitwise
  AST_UNOP_LOG_NOT = 13, // !x
  AST_UNOP_BIT_NOT = 15, // ~x

  // pointer & address
  AST_UNOP_ADDR_OF = 17, // &x
  AST_UNOP_DEREF = 19,   // *p
} AstUnOp;

AstUnOp ast_toktype_to_unop(TokenType type);
String *ast_unop_to_string(AstUnOp op);

typedef enum {
  // multiplicative
  AST_BIN_OP_MUL = 0, // a * b
  AST_BIN_OP_DIV = 2, // a / b
  AST_BIN_OP_MOD = 4, // a % b

  // additive
  AST_BIN_OP_ADD = 6, // a + b
  AST_BIN_OP_SUB = 8, // a - b

  // bit-shift
  AST_BIN_OP_SHL = 10, // a << b
  AST_BIN_OP_SHR = 12, // a >> b

  // relational
  AST_BIN_OP_LT = 14, // a  <   b
  AST_BIN_OP_LE = 16, // a  <=  b
  AST_BIN_OP_GT = 18, // a  >   b
  AST_BIN_OP_GE = 20, // a  >=  b

  // equality
  AST_BIN_OP_EQ = 22, // a == b
  AST_BIN_OP_NE = 24, // a != b

  // bitwise
  AST_BIN_OP_BIT_AND = 26, // a & b
  AST_BIN_OP_BIT_XOR = 28, // a ^ b
  AST_BIN_OP_BIT_OR = 30,  // a | b

  // logical
  AST_BIN_OP_LOG_AND = 32, // a && b
  AST_BIN_OP_LOG_OR = 34,  // a || b

  // assignment
  AST_BIN_OP_ASSIGN = 36,     // a   =  b
  AST_BIN_OP_ADD_ASSIGN = 38, // a  +=  b
  AST_BIN_OP_SUB_ASSIGN = 40, // a  -=  b
  AST_BIN_OP_MUL_ASSIGN = 42, // a  *=  b
  AST_BIN_OP_DIV_ASSIGN = 44, // a  /=  b
  AST_BIN_OP_MOD_ASSIGN = 46, // a  %=  b
  AST_BIN_OP_SHL_ASSIGN = 48, // a  <<= b
  AST_BIN_OP_SHR_ASSIGN = 50, // a  >>= b
  AST_BIN_OP_AND_ASSIGN = 52, // a  &=  b
  AST_BIN_OP_XOR_ASSIGN = 54, // a  ^=  b
  AST_BIN_OP_OR_ASSIGN = 56,  // a  |=  b
} AstBinOp;

AstBinOp ast_toktype_to_binop(TokenType type);
String *ast_binop_to_string(AstBinOp op);

typedef enum {
  // TODO: add more ast variants
  AST_INTEGER,
  AST_FLOAT,
  AST_STRING,
  AST_BOOL,
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
  AST_CAST,
} AstVariant;

typedef struct Ast Ast;

struct Ast {
  // TODO: add support for funptrs
  AstVariant variant;
  ModCType *type;

  int line;

  union {
    // literals
    struct {
      i64 value;
    } number;

    struct {
      f64 value;
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
    // e.g. int_64 x [= 10];
    struct {
      String *name; // TODO: Maybe change to String_View
      ModCType *type;
      Ast *initializer; // optional
    } var_decl;

    // function declaration
    struct {
      String *name; // TODO: Maybe change to String_View
      ModCType *func_type;
      Ast **params; // Its okay to use this fixed size array of type Ast **
                    // because its allocated in the Ast arena which simplifies
                    // memory handling, as compared to a separated allocated
                    // slice or vec. Of course I could provide vec / slice with
                    // an allocator argument that would determine where it
                    // decides to allocate its memory, e.g. into the arena via
                    // the arena_alloc or just to malloc
      size_t param_count;
      Ast *body; // required
    } func_decl;

    // function call
    struct {
      Ast *callee;
      Ast **args;
      size_t arg_count;
      Ast **values;
      size_t val_count;
    } func_call;

    // block
    struct {
      Scope *scope;
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
      size_t value;
      Ast *body;
    } case_stmt;

    // cast
    struct {
      Ast *expr;
      ModCType *type;
    } type_cast;
  };
};

void ast_memory_init(void);
void ast_memory_release(void);

Ast *ast_create(void);
Ast **ast_create_array(int len);

void ast_free(Ast *ast);

// Literals
Ast *ast_int64(i64 val);
Ast *ast_float64(f64 val);
Ast *ast_bool(b8 val);
Ast *ast_char(i64 ch);
Ast *ast_string(const char *str, size_t len);

Ast *ast_ident(const char *str, size_t len);

// Symbol operators i.e: +-*&^>
Ast *ast_binop(AstBinOp op, Ast *left, Ast *right, int *_is_err);
Ast *ast_unop(AstUnOp op, Ast *expr);

// Declarations
Ast *ast_vardecl(const char *name, size_t len, ModCType *type, Ast *init);
Ast *ast_funcdecl(const char *name, size_t len, ModCType *type, Ast **params,
                  size_t param_count, Ast *body);

Ast *ast_funccall(Ast *callee, Ast **args, size_t arg_count);

Ast *ast_block(Ast **statements, size_t stmt_count);
Ast *ast_if(Ast *cond, Ast *then, Ast *els);
Ast *ast_for(Ast *forinit, Ast *forcond, Ast *forstep, Ast *forbody);
Ast *ast_while(Ast *whilecond, Ast *whilebody);
Ast *ast_return(Ast *expr);
Ast *ast_switch(Ast *switch_expr, Ast **cases, size_t case_count,
                Ast *default_case);
Ast *ast_case(size_t value, Ast *body);
Ast *ast_cast(Ast *expr, ModCType *type);

void ast_print(Ast *ast, int depth);

extern VecType vec_ast_ptr_type;

#endif // !AST_H
