#include "mycc_ir.h"

#include <stdlib.h>
#include <string.h>

static char *copy_string(const char *text) {
    size_t length = strlen(text);
    char *copy = malloc(length + 1);
    if (!copy) abort();
    memcpy(copy, text, length + 1);
    return copy;
}

MyccIRFunction *mycc_ir_function_create(const char *name) {
    MyccIRFunction *function = calloc(1, sizeof(*function));
    if (!function) abort();
    function->name = copy_string(name);
    return function;
}

static void destroy_instructions(MyccIRInstruction *instruction) {
    while (instruction) { MyccIRInstruction *next = instruction->next; free(instruction); instruction = next; }
}

void mycc_ir_function_destroy(MyccIRFunction *function) {
    if (!function) return;
    MyccIRBlock *block = function->first_block;
    while (block) { MyccIRBlock *next = block->next; destroy_instructions(block->first); free(block); block = next; }
    free(function->name); free(function);
}

MyccIRBlock *mycc_ir_block_create(MyccIRFunction *function) {
    MyccIRBlock *block = calloc(1, sizeof(*block));
    if (!block) abort();
    block->id = function->next_block++;
    if (function->last_block) function->last_block->next = block; else function->first_block = block;
    function->last_block = block;
    return block;
}

MyccIRValue mycc_ir_constant(long value) { MyccIRValue result = {0, true, value}; return result; }

static MyccIRValue next_value(MyccIRFunction *function) { MyccIRValue result = {function->next_value++, false, 0}; return result; }

MyccIRValue mycc_ir_emit_binary(MyccIRFunction *function, MyccIRBlock *block, MyccIROp op, MyccIRValue left, MyccIRValue right) {
    MyccIRInstruction *instruction = calloc(1, sizeof(*instruction));
    if (!instruction) abort();
    instruction->op = op; instruction->result = next_value(function); instruction->left = left; instruction->right = right;
    if (block->last) block->last->next = instruction; else block->first = instruction;
    block->last = instruction;
    return instruction->result;
}

void mycc_ir_emit_return(MyccIRBlock *block, MyccIRValue value) {
    MyccIRInstruction *instruction = calloc(1, sizeof(*instruction));
    if (!instruction) abort();
    instruction->op = MYCC_IR_RETURN; instruction->left = value;
    if (block->last) block->last->next = instruction; else block->first = instruction;
    block->last = instruction;
}

static long evaluate(MyccIROp op, long left, long right, bool *valid) {
    switch (op) { case MYCC_IR_ADD: return left + right; case MYCC_IR_SUB: return left - right; case MYCC_IR_MUL: return left * right; case MYCC_IR_DIV: if (right) return left / right; *valid = false; return 0; default: *valid = false; return 0; }
}

size_t mycc_ir_fold_constants(MyccIRFunction *function) {
    size_t folded = 0;
    for (MyccIRBlock *block = function->first_block; block; block = block->next) for (MyccIRInstruction *instruction = block->first; instruction; instruction = instruction->next) {
        if (instruction->left.is_constant && instruction->right.is_constant) { bool valid = true; long result = evaluate(instruction->op, instruction->left.constant, instruction->right.constant, &valid); if (valid) { instruction->op = MYCC_IR_CONST; instruction->result.is_constant = true; instruction->result.constant = result; instruction->left = mycc_ir_constant(result); instruction->right = mycc_ir_constant(0); ++folded; } }
    }
    return folded;
}
