# Architecture

## Pipeline

`main.c` owns one compiler invocation: source loading, tokenization, recursive-descent parsing, and assembly generation. Each invocation owns its token vector and syntax nodes; cleanup happens before exit. This compact first release keeps the ownership boundary obvious while leaving extraction points for future modules.

## Front end

The lexer tracks file, byte offset, line, and column in every token. Comments and whitespace are discarded. The parser consumes the token vector with precedence layers for assignment, comparisons, addition, multiplication, unary operators, and primary expressions. Statements are represented as nodes for blocks, declarations, expressions, returns, conditionals, and loops.

## Backend

The backend emits x86-64 assembly. Integer return values use `rax`; the first six integer parameters use `rdi`, `rsi`, `rdx`, `rcx`, `r8`, and `r9`. Functions use a frame pointer and reserve a fixed local area. The driver assembles generated text and links object files through the installed LLVM-MinGW toolchain. This is a correct baseline for the supported integer subset, not a complete aggregate/variadic ABI implementation.

## Driver modes

The driver separates frontend compilation from host tool invocation. `-E` stops after preprocessing, `-S` writes assembly, `-c` assembles each translation unit to an object, and the default mode links all objects into an executable. The generated assembly remains controlled by mycc; the external toolchain is used only for standard assembly and linking.

## Extension plan

The next stable boundaries are a preprocessor/token source, a type and symbol subsystem, a separate AST arena, typed three-address IR, pass manager, and a target interface. Those additions should preserve the current CLI and diagnostics contracts. Register allocation, aggregate ABI classes, object emission, and linker orchestration should remain backend responsibilities.

## Error handling

Malformed input produces a source-positioned diagnostic and increments an invocation-local error count. The parser makes local progress after a failed expectation so several independent errors can be reported. Allocation failures terminate with a controlled message.
