#ifndef MYCC_TYPES_H
#define MYCC_TYPES_H

#include <stdbool.h>
#include <stddef.h>

typedef enum {
    MYCC_TYPE_VOID,
    MYCC_TYPE_BOOL,
    MYCC_TYPE_CHAR,
    MYCC_TYPE_INT,
    MYCC_TYPE_UINT,
    MYCC_TYPE_LONG,
    MYCC_TYPE_ULONG,
    MYCC_TYPE_FLOAT,
    MYCC_TYPE_DOUBLE,
    MYCC_TYPE_POINTER,
    MYCC_TYPE_ARRAY,
    MYCC_TYPE_FUNCTION,
    MYCC_TYPE_STRUCT,
    MYCC_TYPE_UNION,
    MYCC_TYPE_ENUM
} MyccTypeKind;

typedef struct MyccType MyccType;
typedef struct MyccTypeContext MyccTypeContext;

struct MyccType {
    MyccTypeKind kind;
    unsigned qualifiers;
    size_t size;
    size_t alignment;
    MyccType *element;
    size_t array_length;
    MyccType **parameters;
    size_t parameter_count;
    bool variadic;
    MyccType *return_type;
    const char *tag;
};

struct MyccTypeContext {
    MyccType **types;
    size_t count;
    size_t capacity;
};

void mycc_type_context_init(MyccTypeContext *context);
void mycc_type_context_dispose(MyccTypeContext *context);
MyccType *mycc_type_new(MyccTypeContext *context, MyccTypeKind kind);
MyccType *mycc_type_pointer(MyccTypeContext *context, MyccType *element);
MyccType *mycc_type_array(MyccTypeContext *context, MyccType *element, size_t length);
MyccType *mycc_type_function(MyccTypeContext *context, MyccType *return_type,
                            MyccType **parameters, size_t parameter_count, bool variadic);
bool mycc_type_compatible(const MyccType *left, const MyccType *right);
bool mycc_type_assignable(const MyccType *target, const MyccType *value);
size_t mycc_type_size(const MyccType *type);
size_t mycc_type_alignment(const MyccType *type);

#endif
