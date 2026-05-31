#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "arena.h"
#include "ast.h"
#include "lexer.h"
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

  if (!ast) {
    error(NULL, (LineInfo){0}, "arena alloc failed creating ast");
    exit(1);
  }

  memset(ast, 0, sizeof(Ast));

  ast->type = NULL;

  return ast;
}

Ast **ast_create_array(int len) {
  Ast **ast_array = (Ast **)arena_alloc(&ast_arena, sizeof(Ast *) * len);

  if (!ast_array) {
    error(NULL, (LineInfo){0}, "arena alloc failed creating ast array");
    exit(1);
  }

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

AstUnOp ast_toktype_to_unop(TokenType type) {
  switch (type) {
  case TOKEN_PLUS_PLUS:
    return AST_UNOP_PRE_INC;
  case TOKEN_MINUS_MINUS:
    return AST_UNOP_PRE_DEC;
  case TOKEN_PLUS:
    return AST_UNOP_PLUS;
  case TOKEN_MINUS:
    return AST_UNOP_MINUS;
  case TOKEN_NOT:
    return AST_UNOP_LOG_NOT;
  case TOKEN_TILDE:
    return AST_UNOP_BIT_NOT;
  case TOKEN_AND:
    return AST_UNOP_ADDR_OF;
  case TOKEN_ASTERISK:
    return AST_UNOP_DEREF;
  default:
    return 0; // unreachable
  }
}

String *ast_unop_to_string(AstUnOp op) {
  switch (op) {
  case AST_UNOP_POST_INC:
    return str_dup_raw("++", 3);
  case AST_UNOP_POST_DEC:
    return str_dup_raw("--", 3);
  case AST_UNOP_PRE_INC:
    return str_dup_raw("++", 3);
  case AST_UNOP_PRE_DEC:
    return str_dup_raw("--", 3);
  case AST_UNOP_PLUS:
    return str_dup_raw("+", 2);
  case AST_UNOP_MINUS:
    return str_dup_raw("-", 2);
  case AST_UNOP_LOG_NOT:
    return str_dup_raw("!", 2);
  case AST_UNOP_BIT_NOT:
    return str_dup_raw("~", 2);
  case AST_UNOP_ADDR_OF:
    return str_dup_raw("&", 2);
  case AST_UNOP_DEREF:
    return str_dup_raw("*", 2);
  }
  return NULL;
}

AstBinOp ast_toktype_to_binop(TokenType type) {
  switch (type) {
  case TOKEN_ASTERISK:
    return AST_BIN_OP_MUL;
  case TOKEN_SLASH:
    return AST_BIN_OP_DIV;
  case TOKEN_MOD:
    return AST_BIN_OP_MOD;
  case TOKEN_PLUS:
    return AST_BIN_OP_ADD;
  case TOKEN_MINUS:
    return AST_BIN_OP_SUB;
  case TOKEN_LESS_LESS:
    return AST_BIN_OP_SHL;
  case TOKEN_GREATER_GREATER:
    return AST_BIN_OP_SHR;
  case TOKEN_LESS:
    return AST_BIN_OP_LT;
  case TOKEN_LESS_ASSIGN:
    return AST_BIN_OP_LE;
  case TOKEN_GREATER:
    return AST_BIN_OP_GT;
  case TOKEN_GREATER_ASSIGN:
    return AST_BIN_OP_GE;
  case TOKEN_EQ:
    return AST_BIN_OP_EQ;
  case TOKEN_NOT_EQUALS:
    return AST_BIN_OP_NE;
  case TOKEN_AND:
    return AST_BIN_OP_BIT_AND;
  case TOKEN_XOR:
    return AST_BIN_OP_BIT_XOR;
  case TOKEN_OR:
    return AST_BIN_OP_BIT_OR;
  case TOKEN_AND_AND:
    return AST_BIN_OP_LOG_AND;
  case TOKEN_OR_OR:
    return AST_BIN_OP_LOG_OR;
  case TOKEN_ASSIGN:
    return AST_BIN_OP_ASSIGN;
  case TOKEN_PLUS_ASSIGN:
    return AST_BIN_OP_ADD_ASSIGN;
  case TOKEN_MINUS_ASSIGN:
    return AST_BIN_OP_SUB_ASSIGN;
  case TOKEN_ASTERISK_ASSIGN:
    return AST_BIN_OP_MUL_ASSIGN;
  case TOKEN_SLASH_ASSIGN:
    return AST_BIN_OP_DIV_ASSIGN;
  case TOKEN_MOD_ASSIGN:
    return AST_BIN_OP_MOD_ASSIGN;
  case TOKEN_LESS_LESS_ASSIGN:
    return AST_BIN_OP_SHL_ASSIGN;
  case TOKEN_GREATER_GREATER_ASSIGN:
    return AST_BIN_OP_SHR_ASSIGN;
  case TOKEN_AND_ASSIGN:
    return AST_BIN_OP_AND_ASSIGN;
  case TOKEN_XOR_ASSIGN:
    return AST_BIN_OP_XOR_ASSIGN;
  case TOKEN_OR_ASSIGN:
    return AST_BIN_OP_OR_ASSIGN;
  default:
    return 0; // unreachable
  }
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

b8 binop_is_assignment(AstBinOp binop) {
  return binop / 2 >= 18 && binop / 2 <= 28;
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

  ast->type = make_array_type(char_8_type, len + 1);
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

  *_is_err = 0;

  return ast;
}

Ast *ast_vardecl(const char *name, size_t len, ModCType *type, Ast *init) {
  Ast *ast = ast_create();

  ast->variant = AST_VAR;
  ast->var_decl.name = str_dup_raw(name, len);
  ast->var_decl.type = type;
  ast->var_decl.initializer = init;
  ast->var_decl.is_global = 0;

  return ast;
}

Ast *ast_funcdecl(const char *name, size_t len, ModCType *type, Ast **params,
                  size_t param_count, Ast *body) {
  Ast *ast = ast_create();

  ast->variant = AST_FUNC;
  ast->func_decl.name = str_dup_raw(name, len);
  ast->func_decl.func_type = make_function_type(type, params, param_count);

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
  ast->func_call.values = NULL;
  ast->func_call.val_count = 0;

  return ast;
}

Ast *ast_block(Ast **statements, size_t stmt_count) {
  Ast *ast = ast_create();

  ast->variant = AST_BLOCK;
  ast->block.scope = NULL;
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

Ast *ast_cast(Ast *expr, ModCType *type) {
  Ast *ast = ast_create();

  ast->variant = AST_CAST;
  ast->type = type;
  ast->type_cast.type = type;
  ast->type_cast.expr = expr;

  return ast;
}

static void print_string(String *str) {
  printf("%.*s\n", (int)str->len, str->data);
}

static void print_depth(int depth) {
  if (depth <= 0)
    return;
  int spaces = (depth - 1) * 4;
  printf("%*s└──►", spaces, "");
}

void ast_print(Ast *ast, int depth) {
  switch (ast->variant) {
  case AST_INTEGER:
    print_depth(depth);
    printf("%ld\n", ast->number.value);
    break;
  case AST_FLOAT:
    print_depth(depth);
    printf("%lf\n", ast->fvalue.value);
    break;
  case AST_STRING:
    print_depth(depth);
    print_string(ast->string.value);
    break;
  case AST_BOOL:
    print_depth(depth);
    printf("%ld\n", ast->number.value);
    break;
  case AST_IDENTIFIER:
    print_depth(depth);
    print_string(ast->ident.name);
    break;
  case AST_UNOP:
    if (ast->unary.unop == AST_UNOP_POST_INC ||
        ast->unary.unop == AST_UNOP_POST_DEC) {
      ast_print(ast->unary.expr, depth);
      print_depth(depth + 1);
      print_string(ast_unop_to_string(ast->unary.unop));
    } else {
      print_depth(depth);
      print_string(ast_unop_to_string(ast->unary.unop));
      ast_print(ast->unary.expr, depth + 1);
    }
    break;
  case AST_BINOP:
    print_depth(depth);
    print_string(ast_binop_to_string(ast->binary.binop));
    ast_print(ast->binary.left, depth + 1);
    ast_print(ast->binary.right, depth + 1);
    break;
  case AST_VAR:
    print_depth(depth);
    print_string(ast->var_decl.name);
    // print type
    if (ast->var_decl.initializer) {
      printf("%*s=\n", depth * 4, "");
      ast_print(ast->var_decl.initializer, depth + 1);
    }
    break;
  case AST_FUNC:
    print_depth(depth);
    print_string(ast->func_decl.name);
    // print type

    if (ast->func_decl.param_count > 0) {
      printf("%*sparams:\n", depth * 4, "");
      for (size_t i = 0; i < ast->func_decl.param_count; i++) {
        ast_print(ast->func_decl.params[i], depth + 1);
      }
    }

    if (ast->func_decl.body) {
      printf("%*sbody:\n", depth * 4, "");
      ast_print(ast->func_decl.body, depth + 1);
    }
    break;
  case AST_FUNCALL:
    ast_print(ast->func_call.callee, depth);

    if (ast->func_call.arg_count > 0) {
      printf("%*sargs:\n", depth * 4, "");
      for (size_t i = 0; i < ast->func_call.arg_count; i++) {
        ast_print(ast->func_call.args[i], depth + 1);
      }
    }
    break;
  case AST_FUNPTR:
  case AST_FUNPTR_CALL:
    break;
  case AST_BLOCK:
    for (size_t i = 0; i < ast->block.stmt_count; i++) {
      ast_print(ast->block.statements[i], depth);
    }
    break;
  case AST_IF:
    print_depth(depth);
    printf("if\n");
    ast_print(ast->if_stmt.cond, depth + 1);
    printf("%*sthen\n", depth * 4, "");
    ast_print(ast->if_stmt.then, depth + 1);

    if (ast->if_stmt.els) {
      printf("%*selse\n", depth * 4, "");
      ast_print(ast->if_stmt.els, depth + 1);
    }
    break;
  case AST_FOR:
    print_depth(depth);
    printf("for\n");
    ast_print(ast->for_stmt.forinit, depth + 1);
    ast_print(ast->for_stmt.forcond, depth + 1);
    ast_print(ast->for_stmt.forstep, depth + 1);
    ast_print(ast->for_stmt.forbody, depth + 1);
    break;
  case AST_WHILE:
    print_depth(depth);
    printf("while\n");
    ast_print(ast->while_stmt.whilecond, depth + 1);
    ast_print(ast->while_stmt.whilebody, depth + 1);
    break;
  case AST_RETURN:
    printf("return\n");
    if (ast->return_stmt.expr != NULL) {
      ast_print(ast->return_stmt.expr, depth);
    }
    break;
  case AST_BREAK:
  case AST_CONTINUE:
  case AST_SWITCH:
  case AST_CASE:
  case AST_JUMP:
    break;
  case AST_CAST:
    print_depth(depth);
    printf("(");
    print_string(modctype_to_string(ast->type_cast.type));
    printf(")");
    ast_print(ast->type_cast.expr, depth);
    break;
  default:
    break; // unreachable
  }
}

b8 ast_expr_contains_ident(Ast *ast) {
  switch (ast->variant) {
  case AST_BINOP: {
    return (ast->binary.binop != AST_BIN_OP_ASSIGN &&
            ast->binary.left->variant != AST_BINOP &&
            ast_expr_contains_ident(ast->binary.left)) ||
           (ast->binary.right->variant != AST_BINOP &&
            ast_expr_contains_ident(ast->binary.right));
  } break;
  case AST_IDENTIFIER:
    return 1;
  case AST_CAST: {
    return ast_expr_contains_ident(ast->type_cast.expr);
  }
  case AST_RETURN: {
    return ast_expr_contains_ident(ast->return_stmt.expr);
  }
  default:
    break;
  }

  return 0;
}
