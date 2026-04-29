#include <assert.h>
#include <stddef.h>
#include <string.h>
#include <time.h>

#include "arena.h"
#include "ast.h"
#include "hash_map.h"
#include "modc_types.h"
#include "scope.h"
#include "str.h"
#include "types.h"
#include "utils.h"
#include "vec.h"

static int vec_scope_equality(void *s1, void *s2) {
  return str_eq(((Scope *)s1)->name, ((Scope *)s2)->name);
}

VecType vec_scope_type = {
    .to_string = NULL,
    .equals = vec_scope_equality,
    .free = NULL,
    .type_str = "Scope",
};

static void map_str_to_string(String *buf, void *ptr) {
  str_cpy(buf, (String *)ptr, ((String *)ptr)->len);
}

static int map_str_equality(void *s1, void *s2) {
  return str_eq((String *)s1, (String *)s2);
}

static u64 map_str_hash(void *s) { return str_hash((String *)s); }

HashMapType map_str_symbol_type = {
    .key_to_string = map_str_to_string,
    .key_equals = map_str_equality,
    .key_free = NULL,
    .val_to_string = NULL,
    .val_free = NULL,
    .hash = map_str_hash,
    .type_str = "String->Symbol",
};

static Arena scope_arena;
static int scope_arena_init = 0;

void scope_memory_init(void) {
  if (!scope_arena_init) {
    arena_init(&scope_arena, sizeof(Scope) * 512);
    scope_arena_init = 1;
  }
}

static void scope_free_internals(Scope *scope) {
  for (u64 i = 0; i < scope->children->size; i++) {
    scope_free_internals(vec_get_at(scope->children, i));
  }
  vec_release(scope->children);
  hashmap_release(scope->symbols);
}

void scope_memory_release(Scope *global_scope) {
  // go through all scopes and free all the vectors and hashmaps since i havent
  // given them custom allocators, they currently just use malloc
  if (scope_arena_init) {

    scope_free_internals(global_scope);

    scope_arena_init = 0;
    arena_clear(&scope_arena);
  }
}

static Scope *scope_alloc(void) {
  return (Scope *)arena_alloc(&scope_arena, sizeof(Scope));
}

Scope *scope_create(void) {
  Scope *scope = scope_alloc();

  if (!scope)
    error(0, "arena alloc failed creating scope");

  memset(scope, 0, sizeof(Scope));

  scope->name = NULL;
  scope->parent = NULL;
  scope->children = vec_new(&vec_scope_type);
  scope->symbols = hashmap_new(&map_str_symbol_type);

  return scope;
}

static Symbol *symbol_alloc(void) {
  return (Symbol *)arena_alloc(&scope_arena, sizeof(Symbol));
}

Symbol *symbol_create(void) {
  Symbol *symbol = symbol_alloc();

  if (!symbol)
    error(0, "arena alloc failed creating symbol");

  memset(symbol, 0, sizeof(Symbol));

  symbol->name = NULL;
  symbol->type = NULL;
  symbol->scope = NULL;
  symbol->decl = NULL;

  return symbol;
}

Scope *make_global_scope(void) {
  Scope *scope = scope_create();

  scope->variant = SCOPE_GLOBAL;
  scope->name = str_dup_raw("global", 6);

  return scope;
}

Symbol *find_symbol(Scope *current_scope, String *symbol) {
  if (hashmap_has(current_scope->symbols, symbol))
    return hashmap_get(current_scope->symbols, symbol);

  if (current_scope->parent == NULL)
    return NULL;

  return find_symbol(current_scope->parent, symbol);
}

void type_check(Ast *ast, Scope *current_scope) {
  switch (ast->variant) {
  case AST_BLOCK: {
    for (size_t i = 0; i < ast->block.stmt_count; i++) {
      type_check(ast->block.statements[i], ast->block.scope);
    }
  } break;
  case AST_FUNC: {
    // TODO: check that parameters with initializers are placed after those
    // without, i.e. default params must be after positional params
    type_check(ast->func_decl.body, NULL);
    break;
  }
  case AST_IDENTIFIER: {
    Symbol *sym = find_symbol(current_scope, ast->ident.name);

    if (sym == NULL) {
      error(ast->line, "Identifier \"%.*s\" used before declaration",
            (int)ast->ident.name->len, ast->ident.name->data);
      return;
    }

    ast->type = sym->type;
  } break;
  case AST_BINOP: {
    Ast *left = ast->binary.left;
    Ast *right = ast->binary.right;
    type_check(left, current_scope);
    type_check(right, current_scope);

    if (left->type == NULL)
      return;
    if (right->type == NULL)
      return;

    /*
     * Handle other types:
     *
     * all defined in overload table?
     */

    String *binop_str = ast_binop_to_string(ast->binary.binop);
    String *left_type_str = modctype_to_string(ast->binary.left->type);
    String *right_type_str = modctype_to_string(ast->binary.right->type);

    if (!t_is_numeric(left->type) || !t_is_numeric(right->type)) {
      // check overloads table if operation is defined

      if (ast->binary.binop == AST_BIN_OP_ASSIGN) {
        if (left->type != right->type) {
          error(ast->line, "Cannot assign value of type %.*s to type %.*s",
                (int)right_type_str->len, right_type_str->data,
                (int)left_type_str->len, left_type_str->data);
        }
        ast->type = left->type;
        return;
      }

      error(ast->line,
            "Binary operation \"%.*s\" is not defined between types %.*s and "
            "%.*s",
            (int)binop_str->len, binop_str->data, (int)left_type_str->len,
            left_type_str->data, (int)right_type_str->len,
            right_type_str->data);
      return;
    }

    /*
     * Handle booleans
     *
     * bool only defines && and ||
     */

    b8 left_is_bool = (left->type->variant == TYPE_BOOL);
    b8 right_is_bool = (right->type->variant == TYPE_BOOL);

    if (left_is_bool || right_is_bool) {
      if (ast->binary.binop != AST_BIN_OP_LOG_AND &&
          ast->binary.binop != AST_BIN_OP_LOG_OR) {
        error(ast->line,
              "Binary operation \"%.*s\" is not defined for type boolean",
              (int)binop_str->len, binop_str->data);
        return;
      }

      ast->type = bool_type;
      return;
    }

    /*
     * TODO: Handle pointers
     */

    /*
     * Handle floats and integers
     *
     * mul, div, add, sub, log and, log or, comparison takes int/float for both
     * mod, shl, shr, bit and, bit xor, bit or take int for both
     * same for the ones with assignment
     *
     */

    switch (ast->binary.binop) {
    case AST_BIN_OP_ASSIGN: {
      // TODO: type check rhs type can fit in lhs
      // check that lhs is a var
      ast->type = left->type;
    } break;
    case AST_BIN_OP_MUL:
    case AST_BIN_OP_DIV:
    case AST_BIN_OP_ADD:
    case AST_BIN_OP_SUB: {
      ast->type = make_combined_type(ast);
    } break;
    case AST_BIN_OP_ADD_ASSIGN:
    case AST_BIN_OP_SUB_ASSIGN:
    case AST_BIN_OP_MUL_ASSIGN:
    case AST_BIN_OP_DIV_ASSIGN: {
      // TODO: type check whether the combined_type can be put into the original
      // check that lhs is a var
      ModCType *combined_type = make_combined_type(ast);
      (void)combined_type; // unused
      ast->type = void_type;
    } break;
    case AST_BIN_OP_LOG_AND:
    case AST_BIN_OP_LOG_OR:
    case AST_BIN_OP_LT:
    case AST_BIN_OP_LE:
    case AST_BIN_OP_GT:
    case AST_BIN_OP_GE:
    case AST_BIN_OP_EQ:
    case AST_BIN_OP_NE: {
      ast->type = bool_type;
    } break;
    case AST_BIN_OP_MOD:
    case AST_BIN_OP_SHL:
    case AST_BIN_OP_SHR:
    case AST_BIN_OP_BIT_AND:
    case AST_BIN_OP_BIT_XOR:
    case AST_BIN_OP_BIT_OR: {
      if (left->type->variant == TYPE_FLOAT ||
          right->type->variant == TYPE_FLOAT) {
        error(ast->line, "Binary operation %.*s is not defined for type float",
              (int)binop_str->len, binop_str->data);
        return;
      }

      ast->type = make_combined_type(ast);
    } break;
    case AST_BIN_OP_MOD_ASSIGN:
    case AST_BIN_OP_SHL_ASSIGN:
    case AST_BIN_OP_SHR_ASSIGN:
    case AST_BIN_OP_AND_ASSIGN:
    case AST_BIN_OP_XOR_ASSIGN:
    case AST_BIN_OP_OR_ASSIGN: {
      if (left->type->variant == TYPE_FLOAT ||
          right->type->variant == TYPE_FLOAT) {
        error(ast->line, "Binary operation %.*s is not defined for type float",
              (int)binop_str->len, binop_str->data);
        return;
      }

      // TODO: check that lhs is a var

      ModCType *combined_type = make_combined_type(ast);
      (void)combined_type; // unused
      ast->type = void_type;
    } break;
    default:
      break;
    }
  } break;
  case AST_UNOP: {
    type_check(ast->unary.expr, current_scope);

    switch (ast->unary.unop) {
    case AST_UNOP_ADDR_OF: {
      ast->type = make_pointer_type(ast->unary.expr->type, 1);
    } break;
    case AST_UNOP_DEREF: {
      if (ast->unary.expr->type->variant != TYPE_POINTER) {
        error(ast->line, "Attempt to deference a non-pointer type");

        ast->type = void_type;
        return;
      }

      ast->type = ast->unary.expr->type->pointer.base;
    } break;
    case AST_UNOP_BIT_NOT:
    case AST_UNOP_MINUS: {
      if (ast->unary.expr->type->is_signed == false) {
        ast->type = signed_type_wider_than(ast->unary.expr->type->size);
      } else {
        ast->type = ast->unary.expr->type;
      }
    } break;
    case AST_UNOP_LOG_NOT: {
      ast->type = bool_type;
    } break;
    case AST_UNOP_POST_DEC:
    case AST_UNOP_PRE_DEC: {
      if (!t_is_numeric(ast->unary.expr->type)) {
        String *type_str = modctype_to_string(ast->unary.expr->type);
        error(ast->line, "Attempt to decrement non-numeric type %.*s",
              (int)type_str->len, type_str->data);
        return;
      }

      ast->type = ast->unary.expr->type;
    } break;
    case AST_UNOP_POST_INC:
    case AST_UNOP_PRE_INC: {
      if (!t_is_numeric(ast->unary.expr->type)) {
        String *type_str = modctype_to_string(ast->unary.expr->type);
        error(ast->line, "Attempt to increment non-numeric type %.*s",
              (int)type_str->len, type_str->data);
        return;
      }

      ast->type = ast->unary.expr->type;
    } break;
    default:
      ast->type = ast->unary.expr->type;
      break;
    }
  } break;
  case AST_VAR: {
    if (ast->var_decl.initializer == NULL) {
      ast->type = void_type;
      break;
    }

    type_check(ast->var_decl.initializer, current_scope);

    String *var_type_str = modctype_to_string(ast->var_decl.type);
    String *init_type_str = modctype_to_string(ast->var_decl.initializer->type);

    if (ast->var_decl.type->variant !=
        ast->var_decl.initializer->type->variant) {
      error(ast->line,
            "Attempted to assign expression of type %.*s to variable with "
            "defined type %.*s",
            (int)init_type_str->len, init_type_str->data,
            (int)var_type_str->len, var_type_str->data);
      return;
    }

    ast->type = void_type;
  } break;
  case AST_FUNCALL: {
    //  TODO: check that all the parameters are declared and the types match the
    // types of the function. update the ast->type to the return type
    // check number of parameters line up
  } break;
  default:
    break;
  }
}
