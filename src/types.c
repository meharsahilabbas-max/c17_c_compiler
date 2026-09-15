#include "mycc_types.h"

#include <stdlib.h>
#include <string.h>

static void *checked_realloc(void *memory, size_t size) {
    void *result = realloc(memory, size);
    if (!result) abort();
    return result;
}

void mycc_type_context_init(MyccTypeContext *context) {
    memset(context, 0, sizeof(*context));
}

void mycc_type_context_dispose(MyccTypeContext *context) {
    for (size_t i = 0; i < context->count; ++i) {
        free(context->types[i]->parameters);
        free(context->types[i]);
    }
    free(context->types);
    memset(context, 0, sizeof(*context));
}

MyccType *mycc_type_new(MyccTypeContext *context, MyccTypeKind kind) {
    if (context->count == context->capacity) {
        size_t capacity = context->capacity ? context->capacity * 2 : 32;
        context->types = checked_realloc(context->types, capacity * sizeof(*context->types));
        context->capacity = capacity;
    }
    MyccType *type = calloc(1, sizeof(*type));
    if (!type) abort();
    type->kind = kind;
    switch (kind) {
    case MYCC_TYPE_VOID: type->size = 0; type->alignment = 1; break;
    case MYCC_TYPE_BOOL: case MYCC_TYPE_CHAR: type->size = 1; type->alignment = 1; break;
    case MYCC_TYPE_INT: case MYCC_TYPE_UINT: type->size = 4; type->alignment = 4; break;
    case MYCC_TYPE_LONG: case MYCC_TYPE_ULONG: case MYCC_TYPE_POINTER: type->size = 8; type->alignment = 8; break;
    case MYCC_TYPE_FLOAT: type->size = 4; type->alignment = 4; break;
    case MYCC_TYPE_DOUBLE: type->size = 8; type->alignment = 8; break;
    default: type->size = 0; type->alignment = 1; break;
    }
    context->types[context->count++] = type;
    return type;
}

MyccType *mycc_type_pointer(MyccTypeContext *context, MyccType *element) {
    MyccType *type = mycc_type_new(context, MYCC_TYPE_POINTER);
    type->element = element;
    return type;
}

MyccType *mycc_type_array(MyccTypeContext *context, MyccType *element, size_t length) {
    MyccType *type = mycc_type_new(context, MYCC_TYPE_ARRAY);
    type->element = element;
    type->array_length = length;
    type->size = mycc_type_size(element) * length;
    type->alignment = mycc_type_alignment(element);
    return type;
}

MyccType *mycc_type_function(MyccTypeContext *context, MyccType *return_type,
                            MyccType **parameters, size_t parameter_count, bool variadic) {
    MyccType *type = mycc_type_new(context, MYCC_TYPE_FUNCTION);
    if (parameter_count) {
        type->parameters = calloc(parameter_count, sizeof(*type->parameters));
        if (!type->parameters) abort();
        memcpy(type->parameters, parameters, parameter_count * sizeof(*parameters));
    }
    type->return_type = return_type;
    type->parameter_count = parameter_count;
    type->variadic = variadic;
    type->alignment = 1;
    return type;
}

bool mycc_type_compatible(const MyccType *left, const MyccType *right) {
    if (left == right) return true;
    if (!left || !right || left->kind != right->kind) return false;
    if (left->kind == MYCC_TYPE_POINTER) return mycc_type_compatible(left->element, right->element);
    if (left->kind == MYCC_TYPE_ARRAY) return left->array_length == right->array_length && mycc_type_compatible(left->element, right->element);
    if (left->kind == MYCC_TYPE_FUNCTION) {
        if (!mycc_type_compatible(left->return_type, right->return_type) || left->parameter_count != right->parameter_count || left->variadic != right->variadic) return false;
        for (size_t i = 0; i < left->parameter_count; ++i) if (!mycc_type_compatible(left->parameters[i], right->parameters[i])) return false;
        return true;
    }
    if (left->kind == MYCC_TYPE_STRUCT || left->kind == MYCC_TYPE_UNION || left->kind == MYCC_TYPE_ENUM) return left->tag && right->tag && strcmp(left->tag, right->tag) == 0;
    return true;
}

bool mycc_type_assignable(const MyccType *target, const MyccType *value) {
    if (mycc_type_compatible(target, value)) return true;
    if (!target || !value) return false;
    if (target->kind == MYCC_TYPE_POINTER && value->kind == MYCC_TYPE_POINTER) return value->element && value->element->kind == MYCC_TYPE_VOID;
    return (target->kind >= MYCC_TYPE_BOOL && target->kind <= MYCC_TYPE_ULONG) && (value->kind >= MYCC_TYPE_BOOL && value->kind <= MYCC_TYPE_ULONG);
}

size_t mycc_type_size(const MyccType *type) { return type ? type->size : 0; }
size_t mycc_type_alignment(const MyccType *type) { return type ? type->alignment : 1; }
