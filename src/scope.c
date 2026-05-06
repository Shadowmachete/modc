#include <assert.h>
#include <stddef.h>
#include <string.h>
#include <time.h>

#include "arena.h"
#include "ast.h"
#include "hash_map.h"
#include "scope.h"
#include "str.h"
#include "types.h"
#include "utils.h"
#include "vec.h"

#define STR_FMT_UNWRAP(str) (int)(str)->len, (str)->data

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

Symbol *find_symbol(Scope *current_scope, String *symbol) {
  if (hashmap_has(current_scope->symbols, symbol))
    return hashmap_get(current_scope->symbols, symbol);

  if (current_scope->parent == NULL)
    return NULL;

  return find_symbol(current_scope->parent, symbol);
}

Scope *make_global_scope(void) {
  Scope *scope = scope_create();

  scope->variant = SCOPE_GLOBAL;
  scope->name = str_dup_raw("global", 6);

  return scope;
}

Scope *make_block_scope(Scope *current_scope) {
  Scope *new_scope = scope_create();
  new_scope->parent = current_scope;
  new_scope->variant = SCOPE_BLOCK;
  new_scope->name = str_new();
  str_catprintf(new_scope->name, "%.*s_%d", STR_FMT_UNWRAP(current_scope->name),
                current_scope->children->size);
  vec_push(current_scope->children, new_scope);

  return new_scope;
}

void populate_scopes(Ast *ast, Scope *current_scope) {
  switch (ast->variant) {
  case AST_BLOCK: {
    if (ast->block.scope == NULL) {
      ast->block.scope = make_block_scope(current_scope);
    }

    for (size_t i = 0; i < ast->block.stmt_count; i++) {
      populate_scopes(ast->block.statements[i], ast->block.scope);
    }
  } break;
  case AST_FUNC: {
    if (hashmap_has(current_scope->symbols, ast->func_decl.name)) {
      error(ast->line,
            "Cannot redeclare function %.*s() on line %d, function has already "
            "been declared on line %d",
            STR_FMT_UNWRAP(ast->func_decl.name),
            ((Symbol *)hashmap_get(current_scope->symbols, ast->func_decl.name))
                ->decl->line);
      return;
    }

    Symbol *func_symbol = symbol_create();
    func_symbol->name = ast->func_decl.name;
    func_symbol->variant = SYMBOL_FUNCTION;
    func_symbol->scope = current_scope;
    func_symbol->type = ast->func_decl.func_type;
    func_symbol->decl = ast;
    hashmap_add(current_scope->symbols, func_symbol->name, func_symbol);

    Scope *func_scope = scope_create();
    func_scope->parent = current_scope;
    func_scope->variant = SCOPE_FUNCTION;
    func_scope->name = ast->func_decl.name;
    vec_push(current_scope->children, func_scope);

    for (size_t i = 0; i < ast->func_decl.param_count; i++) {
      populate_scopes(ast->func_decl.params[i], func_scope);
    }

    ast->func_decl.body->block.scope = func_scope;

    populate_scopes(ast->func_decl.body, current_scope);
  } break;
  case AST_VAR: {
    if (hashmap_has(current_scope->symbols, ast->var_decl.name)) {
      error(ast->line,
            "Cannot redeclare variable %.*s on line %d, variable has already "
            "been declared in the same scope on line %d",
            STR_FMT_UNWRAP(ast->var_decl.name),
            ((Symbol *)hashmap_get(current_scope->symbols, ast->var_decl.name))
                ->decl->line);
      return;
    }

    Symbol *var_symbol = symbol_create();
    var_symbol->name = ast->var_decl.name;
    var_symbol->variant = SYMBOL_VARIABLE;
    var_symbol->scope = current_scope;
    var_symbol->type = ast->var_decl.type;
    var_symbol->decl = ast;
    hashmap_add(current_scope->symbols, var_symbol->name, var_symbol);
  } break;
  case AST_IF: {
    populate_scopes(ast->if_stmt.then, current_scope);

    if (ast->if_stmt.els != NULL) {
      populate_scopes(ast->if_stmt.els, current_scope);
    }
  } break;
  case AST_WHILE: {
    populate_scopes(ast->while_stmt.whilebody, current_scope);
  } break;
  case AST_FOR: {
    Scope *for_scope = make_block_scope(current_scope);
    ast->for_stmt.forbody->block.scope = for_scope;

    populate_scopes(ast->for_stmt.forinit, for_scope);
    populate_scopes(ast->for_stmt.forbody, current_scope);
  } break;
  default:
    break;
  }
}
