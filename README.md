# mycc

`mycc` is a maintainable C17-subset compiler written in C, with a native Windows IDE target. Its current backend emits GNU AT&T x86-64 assembly following the System V integer calling convention.

This repository is an actively developed compiler and IDE foundation, not a claim of full C17 compatibility. Unsupported language features are documented and rejected rather than silently accepted.

## Build

Windows PowerShell:

```powershell
$cmake = Get-ChildItem "$env:LOCALAPPDATA\Microsoft\WinGet\Packages" -Filter cmake.exe -Recurse | Select-Object -First 1 -ExpandProperty FullName
& $cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
& $cmake --build build
```

With CMake and Ninja on `PATH`:

```sh
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build
ctest --test-dir build --output-on-failure
```

The Makefile provides `make`, `make debug`, `make test`, and `make sanitize` targets on Unix-like environments.

## Usage

```sh
mycc -S examples/factorial.c -o factorial.s
mycc -c examples/factorial.c -o factorial.o
mycc examples/factorial.c -o factorial.exe
```

The current release supports `-E`, `-S`, `-c`, `-o`, `-I`, `-D`, `-U`, `-O0` through `-O3`, `-g`, `--help`, and `--version`. The driver owns preprocessing and assembly generation; LLVM-MinGW/Clang is used only as the external assembler and linker.

## Windows GUI

On Windows, the `mycc-ide.exe` target provides a native Win32 editor with New, Open, Save, and Build actions. It expects `mycc.exe` beside it, opens C source files, and displays compiler diagnostics and generated assembly in the output panel. Build both targets with the Visual Studio C++ workload using the Visual Studio generator:

```powershell
cmake -S . -B build -G "Visual Studio 17 2022" -A x64
cmake --build build --config Release
```

The GUI is intentionally a thin host around the compiler CLI: compiler behavior remains testable and scriptable, while the Windows shell provides the interactive workflow. It does not claim a full Dev-C++ feature set yet; project files, debugging, syntax coloring, linker orchestration, and package management remain future layers.

## Implemented subset

- Lexing of identifiers, decimal/octal/hex integers, strings, comments, keywords, and common operators
- Function definitions returning `int`
- Integer parameters and local declarations
- Arithmetic, comparisons, assignment, unary minus and logical-not
- Function calls with up to six integer arguments
- `return`, `if`/`else`, `while`, compound statements, and global integer data
- C-style `for` loops with declaration initialization
- Object-like macros, `#undef`, `#if`, `#ifdef`, `#ifndef`, `#elif`, `#else`, `#endif`, `#error`, and ignored `#pragma`
- Basic semantic validation for local scope, duplicate locals, undeclared identifiers, and unknown function calls
- Source locations and recoverable parser diagnostics
- x86-64 assembly emission, object generation, executable linking, and multi-file driver flow
- Linux/x86-64 CI configuration and runtime regression coverage
- Target-independent IR with constant folding and optimization-pass statistics
- `.file`/`.loc` assembly metadata and `-g` object/link propagation for debugger-compatible source metadata

## Not yet supported

Full preprocessing semantics, structs/unions/enums syntax integration, pointers and arrays, floating point, typedefs, qualifiers, initializers beyond integer globals, variadic ABI rules, complete DWARF variable/type locations, SSA, and graph-coloring register allocation remain incomplete. `-g` now emits source file/line assembly metadata and propagates debug mode to object/link steps, but does not claim complete source-level debugging yet. These are explicit compatibility boundaries, not hidden fallbacks.

## Architecture

The current executable is intentionally compact while preserving stage boundaries in the code: source loading, lexer, recursive-descent parser, semantic shape validation during parsing, and target emission. The data structures are ownership-local to one compiler invocation and are released before exit. Future extraction points are the token stream, AST arena, semantic/type layer, IR, optimizer, and target interface.

## Testing

The `tests/` directory contains source-level regression inputs. Differential tests should compare generated programs with a host reference compiler once a Linux or LLVM toolchain is available. Sanitizer builds use AddressSanitizer and UndefinedBehaviorSanitizer where the selected C compiler supports them.

## Compatibility report

**Supported:** integer expressions, integer globals, integer functions, local variables, calls, returns, conditionals, loops, comments, diagnostics, and assembly output.

**Partially supported:** C translation units, declarations, optimization flags, and source locations.

**Not supported:** full C17, preprocessing, complete type checking, pointers, aggregate types, floating point, standard headers, linking, debug metadata, and ARM64/RISC-V targets.

The IDE-specific design and build details are in [docs/IDE.md](docs/IDE.md). The Windows desktop shell is a real native executable target, but it should not be described as a full Dev-C++ replacement until project management, syntax services, linking, debugging, and the remaining compiler stages are implemented.
