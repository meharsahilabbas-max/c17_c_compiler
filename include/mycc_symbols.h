#ifndef MYCC_SYMBOLS_H
#define MYCC_SYMBOLS_H

#include "mycc_types.h"
#include <stdbool.h>
#include <stddef.h>

typedef enum {
    MYCC_SYMBOL_OBJECT,
    MYCC_SYMBOL_FUNCTION,
    MYCC_SYMBOL_TYPEDEF,
    MYCC_SYMBOL_ENUMERATOR,
    MYCC_SYMBOL_TAG
} MyccSymbolKind;

typedef struct MyccSymbol MyccSymbol;
typedef struct MyccScope MyccScope;

struct MyccSymbol {
    char *name;
    MyccSymbolKind kind;
    MyccType *type;
    MyccSymbol *next;
};

struct MyccScope {
    MyccScope *parent;
    MyccSymbol **buckets;
    size_t bucket_count;
};

MyccScope *mycc_scope_create(MyccScope *parent, size_t bucket_count);
void mycc_scope_destroy(MyccScope *scope);
MyccSymbol *mycc_scope_declare(MyccScope *scope, const char *name, MyccSymbolKind kind, MyccType *type);
MyccSymbol *mycc_scope_lookup_current(const MyccScope *scope, const char *name);
MyccSymbol *mycc_scope_lookup(const MyccScope *scope, const char *name);

#endif
