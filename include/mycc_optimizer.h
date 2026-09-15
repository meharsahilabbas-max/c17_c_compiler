#ifndef MYCC_OPTIMIZER_H
#define MYCC_OPTIMIZER_H

#include "mycc_ir.h"

typedef enum {
    MYCC_OPT_O0,
    MYCC_OPT_O1,
    MYCC_OPT_O2,
    MYCC_OPT_O3
} MyccOptimizationLevel;

typedef struct {
    size_t constant_folds;
    size_t algebraic_simplifications;
    size_t dead_instructions;
} MyccOptimizationStats;

MyccOptimizationStats mycc_optimize(MyccIRFunction *function, MyccOptimizationLevel level);

#endif
