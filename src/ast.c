#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#include "arena.h"
#include "ast.h"
#include "modc_types.h"
#include "str.h"
#include "utils.h"
#include "vec.h"

static Arena ast_arena;
static int ast_arena_init = 0;

void ast_memory_init(void) {
  if (!ast_arena_init) {
    arena_init(&ast_arena, sizeof(Ast) * 512);
    ast_arena_init = 1;
  }
}

void ast_memory_release(void) {
  if (ast_arena_init) {
    ast_arena_init = 0;
    arena_clear(&ast_arena);
  }
}

static Ast *ast_alloc(void) {
  return (Ast *)arena_alloc(&ast_arena, sizeof(Ast));
}

Ast *ast_create(void) {
  Ast *ast = ast_alloc();

  if (!ast)
    error(0, "arena alloc failed creating ast");

  memset(ast, 0, sizeof(Ast));

  return ast;
}

Ast **ast_create_array(int len) {
  Ast **ast_array = (Ast **)arena_alloc(&ast_arena, sizeof(Ast *) * len);

  if (!ast_array)
    error(0, "arena alloc failed creating ast array");

  memset(ast_array, 0, sizeof(Ast *) * len);

  return ast_array;
}

void ast_free(Ast *ast) { (void)ast; }

int vec_ast_free(void *ast) {
  ast_free((Ast *)ast);
  return 1;
}

VecType vec_ast_ptr_type = {
    .to_string = NULL,
    .equals = NULL,
    .free = vec_ast_free,
    .type_str = "Ast *", // vec stores pointers to the asts
};

String *ast_unop_to_string(AstUnOp op) {
  switch (op) {
  case AST_UNOP_POST_INC:
    return str_dup_raw("x++", 3);
  case AST_UNOP_POST_DEC:
    return str_dup_raw("x--", 3);
  case AST_UNOP_PRE_INC:
    return str_dup_raw("++x", 3);
  case AST_UNOP_PRE_DEC:
    return str_dup_raw("--x", 3);
  case AST_UNOP_PLUS:
    return str_dup_raw("+x", 2);
  case AST_UNOP_MINUS:
    return str_dup_raw("-x", 2);
  case AST_UNOP_LOG_NOT:
    return str_dup_raw("!x", 2);
  case AST_UNOP_BIT_NOT:
    return str_dup_raw("~x", 2);
  case AST_UNOP_ADDR_OF:
    return str_dup_raw("&x", 2);
  case AST_UNOP_DEREF:
    return str_dup_raw("*p", 2);
  }
  return NULL;
}

String *ast_binop_to_string(AstBinOp op) {
  switch (op) {
  case AST_BIN_OP_MUL:
    return str_dup_raw("*", 1);
  case AST_BIN_OP_DIV:
    return str_dup_raw("/", 1);
  case AST_BIN_OP_MOD:
    return str_dup_raw("%", 1);
  case AST_BIN_OP_ADD:
    return str_dup_raw("+", 1);
  case AST_BIN_OP_SUB:
    return str_dup_raw("-", 1);
  case AST_BIN_OP_SHL:
    return str_dup_raw("<<", 2);
  case AST_BIN_OP_SHR:
    return str_dup_raw(">>", 2);
  case AST_BIN_OP_LT:
    return str_dup_raw("< ", 1);
  case AST_BIN_OP_LE:
    return str_dup_raw("<=", 2);
  case AST_BIN_OP_GT:
    return str_dup_raw("> ", 1);
  case AST_BIN_OP_GE:
    return str_dup_raw(">=", 2);
  case AST_BIN_OP_EQ:
    return str_dup_raw("==", 2);
  case AST_BIN_OP_NE:
    return str_dup_raw("!=", 2);
  case AST_BIN_OP_BIT_AND:
    return str_dup_raw("&", 1);
  case AST_BIN_OP_BIT_XOR:
    return str_dup_raw("^", 1);
  case AST_BIN_OP_BIT_OR:
    return str_dup_raw("|", 1);
  case AST_BIN_OP_LOG_AND:
    return str_dup_raw("&&", 2);
  case AST_BIN_OP_LOG_OR:
    return str_dup_raw("||", 2);
  case AST_BIN_OP_ASSIGN:
    return str_dup_raw("=", 1);
  case AST_BIN_OP_ADD_ASSIGN:
    return str_dup_raw("+=", 2);
  case AST_BIN_OP_SUB_ASSIGN:
    return str_dup_raw("-=", 2);
  case AST_BIN_OP_MUL_ASSIGN:
    return str_dup_raw("*=", 2);
  case AST_BIN_OP_DIV_ASSIGN:
    return str_dup_raw("/=", 2);
  case AST_BIN_OP_MOD_ASSIGN:
    return str_dup_raw("%=", 2);
  case AST_BIN_OP_SHL_ASSIGN:
    return str_dup_raw("<<=", 3);
  case AST_BIN_OP_SHR_ASSIGN:
    return str_dup_raw(">>=", 3);
  case AST_BIN_OP_AND_ASSIGN:
    return str_dup_raw("&=", 2);
  case AST_BIN_OP_XOR_ASSIGN:
    return str_dup_raw("^=", 2);
  case AST_BIN_OP_OR_ASSIGN:
    return str_dup_raw("|=", 2);
  }
  return NULL;
}

Ast *ast_int64(i64 val) {
  Ast *ast = ast_create();

  ast->variant = AST_INTEGER;
  ast->type = int_64_type;
  ast->number.value = val;

  return ast;
}

Ast *ast_float64(f64 val) {
  Ast *ast = ast_create();

  ast->variant = AST_FLOAT;
  ast->type = float_64_type;
  ast->fvalue.value = val;

  return ast;
}

Ast *ast_bool(b8 val) {
  Ast *ast = ast_create();

  ast->variant = AST_BOOL;
  ast->type = bool_type;
  ast->number.value = val;

  return ast;
}

Ast *ast_char(i64 val) {
  Ast *ast = ast_create();

  ast->variant = AST_INTEGER;
  ast->type = char_8_type; // default string encoding is utf-8
  ast->number.value = val;

  return ast;
}

Ast *ast_string(const char *str, size_t len) {
  Ast *ast = ast_create();

  ast->variant = AST_STRING;
  ast->string.value = str_dup_raw(str, len);

  // TODO: implement make_array_type()

  /* ast->type = make_array_type(char_8_type, len); */
  return ast;
}

Ast *ast_ident(const char *str, size_t len) {
  Ast *ast = ast_create();

  ast->variant = AST_IDENTIFIER;
  ast->ident.name = str_dup_raw(str, len);

  return ast;
}

Ast *ast_unop(AstUnOp op, Ast *expr) {
  Ast *ast = ast_create();

  ast->variant = AST_UNOP;
  ast->unary.unop = op;
  ast->unary.expr = expr;

  return ast;
}

Ast *ast_binop(AstBinOp op, Ast *left, Ast *right, int *_is_err) {
  Ast *ast = ast_create();

  ast->variant = AST_BINOP;
  ast->binary.binop = op;
  ast->binary.left = left;
  ast->binary.right = right;

  if (op == AST_BIN_OP_EQ || op == AST_BIN_OP_NE) {
    ast->type = bool_type;
  } else {
    // TODO: implement make_combined_type()

    /* ast->type = make_combined_type(left, right) */
    // calculate the return type of the operation
  }

  *_is_err = 0;
  // forgot what _is_err is for but ill figure it out in the future

  return ast;
}

Ast *ast_vardecl(const char *name, size_t len, ModCType *type, Ast *init) {
  Ast *ast = ast_create();

  ast->variant = AST_VAR;
  ast->var_decl.name = str_dup_raw(name, len);
  ast->var_decl.type = type;
  ast->var_decl.initializer = init;

  return ast;
}

Ast *ast_funcdecl(const char *name, size_t len, ModCType *type, Ast **params,
                  size_t param_count, Ast *body) {
  Ast *ast = ast_create();

  ast->variant = AST_FUNC;
  ast->func_decl.name = str_dup_raw(name, len);
  (void)type;
  // TODO: implement make_function_type()

  /* ast->func_decl.func_type = make_function_type(type, params,
   * param_count); */

  ast->func_decl.params = params;
  ast->func_decl.param_count = param_count;
  ast->func_decl.body = body;

  return ast;
}

Ast *ast_funccall(Ast *callee, Ast **args, size_t arg_count) {
  Ast *ast = ast_create();

  ast->variant = AST_FUNCALL;
  ast->func_call.callee = callee;
  ast->func_call.args = args;
  ast->func_call.arg_count = arg_count;

  return ast;
}

Ast *ast_block(Ast **statements, size_t stmt_count) {
  Ast *ast = ast_create();

  ast->variant = AST_BLOCK;
  ast->block.statements = statements;
  ast->block.stmt_count = stmt_count;

  return ast;
}

Ast *ast_if(Ast *cond, Ast *then, Ast *els) {
  Ast *ast = ast_create();

  ast->variant = AST_IF;
  ast->if_stmt.cond = cond;
  ast->if_stmt.then = then;
  ast->if_stmt.els = els;

  return ast;
}

Ast *ast_for(Ast *forinit, Ast *forcond, Ast *forstep, Ast *forbody) {
  Ast *ast = ast_create();

  ast->variant = AST_FOR;
  ast->for_stmt.forinit = forinit;
  ast->for_stmt.forcond = forcond;
  ast->for_stmt.forstep = forstep;
  ast->for_stmt.forbody = forbody;

  return ast;
}

Ast *ast_while(Ast *whilecond, Ast *whilebody) {
  Ast *ast = ast_create();

  ast->variant = AST_WHILE;
  ast->while_stmt.whilecond = whilecond;
  ast->while_stmt.whilebody = whilebody;

  return ast;
}

Ast *ast_return(Ast *expr) {
  Ast *ast = ast_create();

  ast->variant = AST_RETURN;
  ast->return_stmt.expr = expr;

  return ast;
}

Ast *ast_switch(Ast *switch_expr, Ast **cases, size_t case_count,
                Ast *default_case) {
  Ast *ast = ast_create();

  ast->variant = AST_SWITCH;
  ast->switch_stmt.switch_expr = switch_expr;
  ast->switch_stmt.cases = cases;
  ast->switch_stmt.case_count = case_count;
  ast->switch_stmt.default_case = default_case;

  return ast;
}

Ast *ast_case(size_t value, Ast *body) {
  Ast *ast = ast_create();

  ast->variant = AST_CASE;
  ast->case_stmt.value = value;
  ast->case_stmt.body = body;

  return ast;
}
