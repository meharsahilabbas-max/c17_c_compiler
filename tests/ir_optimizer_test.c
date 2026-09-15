#include "mycc_ir.h"
#include "mycc_optimizer.h"

int main(void) {
    MyccIRFunction *function = mycc_ir_function_create("optimizer_test");
    MyccIRBlock *block = mycc_ir_block_create(function);
    MyccIRValue left = mycc_ir_constant(20);
    MyccIRValue right = mycc_ir_constant(22);
    MyccIRValue result = mycc_ir_emit_binary(function, block, MYCC_IR_ADD, left, right);
    mycc_ir_emit_return(block, result);
    MyccOptimizationStats stats = mycc_optimize(function, MYCC_OPT_O2);
    int passed = stats.constant_folds == 1;
    mycc_ir_function_destroy(function);
    return passed ? 0 : 1;
}
