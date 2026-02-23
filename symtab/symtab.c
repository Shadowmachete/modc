#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../utils.h"
#include "symtab.h"

#define INITIAL_CAPACITY 128
#define MAX_LOAD 0.75
#define HASH_SEED 0x9747b28c

#define INITIAL_SCOPE_CAPACITY 8

Scope *GLOBAL_SCOPE;

SymTable *symtab_create(void) {
  SymTable *symtab = malloc(sizeof(SymTable));

  if (!symtab)
    error(0, "malloc failed when creating symtab");

  symtab->capacity = INITIAL_CAPACITY;
  symtab->count = 0;
  symtab->buckets = calloc(INITIAL_CAPACITY, sizeof(Symbol *));
  if (!symtab->buckets)
    error(0, "calloc failed when creating buckets");

  GLOBAL_SCOPE = scope_create("Global", 6, NULL);

  symtab->current_scope = GLOBAL_SCOPE;

  return symtab;
}

void symtab_resize(SymTable *symtab) {
  size_t old_capacity = symtab->capacity;
  Symbol **old_buckets = symtab->buckets;

  size_t new_capacity = old_capacity << 1;
  Symbol **new_buckets = calloc(new_capacity, sizeof(Symbol *));

  if (!new_buckets)
    error(0, "calloc failed when creating buckets");

  for (size_t i = 0; i < old_capacity; i++) {
    Symbol *sym = old_buckets[i];

    while (sym) {
      Symbol *next = sym->next;

      size_t idx = sym->hash & (new_capacity - 1);

      sym->next = new_buckets[idx];
      new_buckets[idx] = sym;

      sym = next;
    }
  }

  free(old_buckets);

  symtab->buckets = new_buckets;
  symtab->capacity = new_capacity;
}

void symtab_destroy(SymTable *symtab) {
  size_t capacity = symtab->capacity;
  Symbol **buckets = symtab->buckets;
  for (size_t i = 0; i < capacity; i++) {
    Symbol *sym = buckets[i];

    while (sym) {
      Symbol *next = sym->next;
      free((void *)sym->name);
      free(sym);
      sym = next;
    }
  }

  free(buckets);
  free(symtab);
}

Symbol *symtab_insert(SymTable *symtab, const char *name, size_t len) {
  uint32_t hash = murmur3_32((uint8_t *)name, len, HASH_SEED);

  if (symtab->count > symtab->capacity * MAX_LOAD) {
    symtab_resize(symtab);
  }

  size_t idx = hash & (symtab->capacity - 1);

  Symbol *sym = malloc(sizeof(Symbol)); // use arenas / free lists in the future

  if (!sym)
    error(0, "malloc failed creating new symbol.");

  sym->name = strndup(name, len);
  sym->length = len;
  sym->hash = hash;
  sym->next = symtab->buckets[idx];
  symtab->buckets[idx] = sym;

  symtab->count++;

  return sym;
}

Symbol *symtab_lookup(SymTable *symtab, const char *name, size_t len) {
  uint32_t hash = murmur3_32((uint8_t *)name, len, HASH_SEED);

  size_t idx = hash & (symtab->capacity - 1);

  Symbol *sym = symtab->buckets[idx];

  while (sym) {
    if (sym->length == len && memcmp(name, sym->name, len) == 0 &&
        sym->scope == symtab->current_scope)
      return sym;

    sym = sym->next;
  }

  return NULL;
}

void symtab_print(SymTable *symtab) {
  printf("Symbol table:\n");
  size_t capacity = symtab->capacity;
  Symbol **buckets = symtab->buckets;
  for (size_t i = 0; i < capacity; i++) {
    Symbol *sym = buckets[i];

    while (sym) {
      symbol_print(sym);
      sym = sym->next;
    }
  }
}

char *variant_name(enum Variant v) {
  switch (v) {
  case FUNCTION:
    return "FUNCTION";
  case LABEL:
    return "LABEL";
  case VARIABLE:
    return "VARIABLE";
  case PARAMETER:
    return "PARAMETER";
  default:
    return "UNKNOWN";
  }
}

void symbol_print(Symbol *sym) {
  if (!sym)
    return;

  if (sym->is_pointer)
    printf("Symbol: [scope %.*s] PTR %-6s %s '%.*s' %zu\n",
           (int)sym->scope->length, sym->scope->name,
           token_type_name(sym->type), variant_name(sym->variant),
           (int)sym->length, sym->name, sym->length);
  else
    printf("Symbol: [scope %.*s] %-10s %s '%.*s' %zu\n",
           (int)sym->scope->length, sym->scope->name,
           token_type_name(sym->type), variant_name(sym->variant),
           (int)sym->length, sym->name, sym->length);
}

Scope *scope_create(char *name, size_t length, Scope *parent) {
  Scope *new_scope = malloc(sizeof(Scope));

  new_scope->parent = parent;
  new_scope->num_children = 0;
  new_scope->capacity = INITIAL_SCOPE_CAPACITY;
  new_scope->children = calloc(INITIAL_SCOPE_CAPACITY, sizeof(Scope *));
  if (!new_scope->children)
    error(0, "calloc failed when allocating memory for new scope");

  new_scope->name = name;
  new_scope->length = length;

  return new_scope;
}

void scope_resize(Scope *scope) {
  size_t new_capacity = scope->capacity << 1;

  if (new_capacity < scope->capacity)
    error(0, "capacity overflow");

  Scope **new_children =
      realloc(scope->children, new_capacity * sizeof(Scope *));

  if (!new_children)
    error(0, "realloc failed when resizing scope children");

  scope->children = new_children;
  scope->capacity = new_capacity;
}
