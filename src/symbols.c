#include "mycc_symbols.h"

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

static uint64_t hash_name(const char *name) {
    uint64_t hash = 1469598103934665603ULL;
    while (*name) { hash ^= (unsigned char)*name++; hash *= 1099511628211ULL; }
    return hash;
}

static char *copy_name(const char *name) {
    size_t length = strlen(name);
    char *copy = malloc(length + 1);
    if (!copy) abort();
    memcpy(copy, name, length + 1);
    return copy;
}

MyccScope *mycc_scope_create(MyccScope *parent, size_t bucket_count) {
    if (bucket_count == 0) bucket_count = 64;
    MyccScope *scope = calloc(1, sizeof(*scope));
    if (!scope) abort();
    scope->buckets = calloc(bucket_count, sizeof(*scope->buckets));
    if (!scope->buckets) abort();
    scope->parent = parent;
    scope->bucket_count = bucket_count;
    return scope;
}

void mycc_scope_destroy(MyccScope *scope) {
    if (!scope) return;
    for (size_t i = 0; i < scope->bucket_count; ++i) {
        MyccSymbol *symbol = scope->buckets[i];
        while (symbol) { MyccSymbol *next = symbol->next; free(symbol->name); free(symbol); symbol = next; }
    }
    free(scope->buckets);
    free(scope);
}

MyccSymbol *mycc_scope_declare(MyccScope *scope, const char *name, MyccSymbolKind kind, MyccType *type) {
    if (!scope || !name || mycc_scope_lookup_current(scope, name)) return NULL;
    size_t bucket = (size_t)(hash_name(name) % scope->bucket_count);
    MyccSymbol *symbol = calloc(1, sizeof(*symbol));
    if (!symbol) abort();
    symbol->name = copy_name(name); symbol->kind = kind; symbol->type = type;
    symbol->next = scope->buckets[bucket]; scope->buckets[bucket] = symbol;
    return symbol;
}

MyccSymbol *mycc_scope_lookup_current(const MyccScope *scope, const char *name) {
    if (!scope || !name) return NULL;
    size_t bucket = (size_t)(hash_name(name) % scope->bucket_count);
    for (MyccSymbol *symbol = scope->buckets[bucket]; symbol; symbol = symbol->next) if (strcmp(symbol->name, name) == 0) return symbol;
    return NULL;
}

MyccSymbol *mycc_scope_lookup(const MyccScope *scope, const char *name) {
    for (const MyccScope *current = scope; current; current = current->parent) {
        MyccSymbol *symbol = mycc_scope_lookup_current(current, name);
        if (symbol) return symbol;
    }
    return NULL;
}
