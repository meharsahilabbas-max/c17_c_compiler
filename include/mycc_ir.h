#ifndef MYCC_IR_H
#define MYCC_IR_H

#include <stdbool.h>
#include <stddef.h>

typedef enum {
    MYCC_IR_CONST,
    MYCC_IR_ADD,
    MYCC_IR_SUB,
    MYCC_IR_MUL,
    MYCC_IR_DIV,
    MYCC_IR_LOAD,
    MYCC_IR_STORE,
    MYCC_IR_COMPARE,
    MYCC_IR_BRANCH,
    MYCC_IR_CALL,
    MYCC_IR_RETURN
} MyccIROp;

typedef struct MyccIRValue MyccIRValue;
typedef struct MyccIRInstruction MyccIRInstruction;
typedef struct MyccIRBlock MyccIRBlock;
typedef struct MyccIRFunction MyccIRFunction;

struct MyccIRValue {
    unsigned id;
    bool is_constant;
    long constant;
};

struct MyccIRInstruction {
    MyccIROp op;
    MyccIRValue result;
    MyccIRValue left;
    MyccIRValue right;
    MyccIRBlock *target;
    MyccIRInstruction *next;
};

struct MyccIRBlock {
    unsigned id;
    MyccIRInstruction *first;
    MyccIRInstruction *last;
    MyccIRBlock *next;
};

struct MyccIRFunction {
    char *name;
    MyccIRBlock *first_block;
    MyccIRBlock *last_block;
    unsigned next_value;
    unsigned next_block;
};

MyccIRFunction *mycc_ir_function_create(const char *name);
void mycc_ir_function_destroy(MyccIRFunction *function);
MyccIRBlock *mycc_ir_block_create(MyccIRFunction *function);
MyccIRValue mycc_ir_constant(long value);
MyccIRValue mycc_ir_emit_binary(MyccIRFunction *function, MyccIRBlock *block,
                                MyccIROp op, MyccIRValue left, MyccIRValue right);
void mycc_ir_emit_return(MyccIRBlock *block, MyccIRValue value);
size_t mycc_ir_fold_constants(MyccIRFunction *function);

#endif
