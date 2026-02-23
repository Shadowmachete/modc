#ifndef SYMTAB_H
#define SYMTAB_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "../lexer/lexer.h"

typedef struct Scope {
  struct Scope **children;
  struct Scope *parent;
  size_t num_children;
  size_t capacity;
  char *name;
  size_t length;
} Scope;

typedef struct Symbol {
  const char *name;
  struct Symbol *next;

  size_t length;
  Scope *scope;

  uint32_t hash;

  TokenType type;
  enum Variant { FUNCTION, LABEL, VARIABLE, PARAMETER } variant;

  bool is_pointer;
  uint8_t pointer_depth;
} Symbol;

typedef struct {
  Scope *current_scope;

  Symbol **buckets;
  size_t capacity;
  size_t count;
} SymTable;

SymTable *symtab_create(void);
void symtab_resize(SymTable *symtab);
void symtab_destroy(SymTable *symtab);

void symtab_enter_scope(SymTable *symtab, char *name, size_t length);
void symtab_leave_scope(SymTable *symtab);

Symbol *symtab_insert(SymTable *symtab, const char *name, size_t len);
Symbol *symtab_lookup(SymTable *symtab, const char *name, size_t len);

void symtab_print(SymTable *symtab);
void symbol_print(Symbol *sym);

Scope *scope_create(char *name, size_t length, Scope *parent);
void scope_resize(Scope *scope);
void scope_tree_destroy(Scope *scope);

#endif
