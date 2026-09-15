#include "mycc_optimizer.h"

#include <stdlib.h>

static int value_used(const MyccIRFunction *function, unsigned id) {
    for (const MyccIRBlock *block = function->first_block; block; block = block->next) {
        for (const MyccIRInstruction *instruction = block->first; instruction; instruction = instruction->next) {
            if ((!instruction->left.is_constant && instruction->left.id == id) ||
                (!instruction->right.is_constant && instruction->right.id == id)) return 1;
        }
    }
    return 0;
}

static size_t simplify_block(const MyccIRFunction *function, MyccIRBlock *block, MyccOptimizationStats *stats) {
    size_t removed = 0;
    MyccIRInstruction *previous = NULL;
    MyccIRInstruction *instruction = block->first;
    while (instruction) {
        MyccIRInstruction *next = instruction->next;
        if (instruction->op == MYCC_IR_ADD || instruction->op == MYCC_IR_SUB || instruction->op == MYCC_IR_MUL) {
            if ((instruction->op == MYCC_IR_ADD || instruction->op == MYCC_IR_SUB) && instruction->right.is_constant && instruction->right.constant == 0) ++stats->algebraic_simplifications;
            if (instruction->op == MYCC_IR_MUL && ((instruction->right.is_constant && instruction->right.constant == 1) || (instruction->left.is_constant && instruction->left.constant == 1))) ++stats->algebraic_simplifications;
            if (instruction->op == MYCC_IR_MUL && ((instruction->right.is_constant && instruction->right.constant == 0) || (instruction->left.is_constant && instruction->left.constant == 0))) ++stats->algebraic_simplifications;
        }
        if (instruction->op == MYCC_IR_CONST || instruction->op == MYCC_IR_ADD || instruction->op == MYCC_IR_SUB || instruction->op == MYCC_IR_MUL || instruction->op == MYCC_IR_DIV) {
            if (!value_used(function, instruction->result.id)) {
                if (previous) previous->next = next; else block->first = next;
                if (block->last == instruction) block->last = previous;
                free(instruction); ++removed; instruction = next; continue;
            }
        }
        previous = instruction;
        instruction = next;
    }
    return removed;
}

MyccOptimizationStats mycc_optimize(MyccIRFunction *function, MyccOptimizationLevel level) {
    MyccOptimizationStats stats = {0};
    if (!function || level == MYCC_OPT_O0) return stats;
    stats.constant_folds = mycc_ir_fold_constants(function);
    if (level >= MYCC_OPT_O2) {
        for (MyccIRBlock *block = function->first_block; block; block = block->next) stats.dead_instructions += simplify_block(function, block, &stats);
    }
    if (level >= MYCC_OPT_O3) {
        for (MyccIRBlock *block = function->first_block; block; block = block->next) stats.dead_instructions += simplify_block(function, block, &stats);
    }
    return stats;
}
