#ifndef SCOPE_H
#define SCOPE_H

#include "ast.h"
#include "hash_map.h"
#include "str.h"
#include "utils.h"
#include "vec.h"

typedef struct Scope Scope;
typedef struct Symbol Symbol;

struct Scope {
  String *name;
  enum {
    SCOPE_GLOBAL,
    SCOPE_FUNCTION,
    SCOPE_BLOCK,
  } variant;

  Scope *parent;
  Vec *children;
  HashMap *symbols;
  File *f;
};

struct Symbol {
  String *name;
  enum {
    SYMBOL_VARIABLE,
    SYMBOL_FUNCTION,
    SYMBOL_ENUM,
  } variant;

  ModCType *type;
  Scope *scope;

  Ast *decl;
};

void scope_memory_init(void);
void scope_memory_release(Scope *global_scope);
Scope *scope_create(Scope *parent, String *name);
Symbol *symbol_create(Scope *scope, String *name, ModCType *type, Ast *decl);
Symbol *find_symbol(Scope *current_scope, String *symbol);

Scope *make_global_scope(File *f);
void populate_scopes(Ast *ast, Scope *current_scope);

#endif // !SCOPE_H
