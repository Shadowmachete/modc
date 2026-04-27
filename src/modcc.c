#include <stdio.h>
#include <stdlib.h>

#include "arena.h"
#include "ast.h"
#include "hash_map.h"
#include "lexer.h"
#include "modc_types.h"
#include "parser.h"
#include "scope.h"
#include "str.h"
#include "utils.h"
#include "vec.h"

#define ARENA_CAPACITY (1024 * 64)

Scope *make_block_scope(Scope *current_scope) {
  Scope *new_scope = scope_create();
  new_scope->parent = current_scope;
  new_scope->variant = SCOPE_BLOCK;
  new_scope->name = str_new();
  str_catprintf(new_scope->name, "%.*s_%d", (int)current_scope->name->len,
                current_scope->name->data, current_scope->children->size);
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
            (int)ast->var_decl.name->len, ast->var_decl.name->data, ast->line,
            ((Symbol *)hashmap_get(current_scope->symbols, ast->var_decl.name))
                ->decl->line);
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

int main(int argc, char *argv[]) {
  if (argc != 2) {
    fputs("usage: modcc file.modc\n", stderr);
    exit(1);
  }

  char *source = readin(argv[1]);
  printf("Source: \n%s\n", source);

  global_arena_init(ARENA_CAPACITY);
  ast_memory_init();
  scope_memory_init();
  modctype_memory_init();

  Lexer lexer;
  Parser *parser = parser_create(&lexer);

  parse(parser, source); // produce AST

  ast_print(parser->ast, 0);

  Scope *global_scope = make_global_scope();

  parser->ast->block.scope = global_scope;

  populate_scopes(parser->ast, global_scope);

  type_check(parser->ast, global_scope);

  // typecheck and identifier declared check

  free(source);
  parser_free(parser);
  global_arena_release();
  ast_memory_release();
  scope_memory_release(global_scope);
  modctype_memory_release();
}
