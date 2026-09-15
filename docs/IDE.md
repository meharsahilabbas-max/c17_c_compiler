# mycc IDE

`mycc-ide` is the Windows desktop shell for the compiler. It is a native Win32 executable, so it does not require a browser runtime or a separate framework.

## Features

- C source editor with a fixed-width programming font
- New, Open, and Save workflows using UTF-8 files
- Build menu and toolbar action
- `-O0`, `-O1`, `-O2`, and `-O3` selection passed to the compiler
- Compiler diagnostics and generated assembly in the output panel
- Keyboard-labelled menu commands for common actions
- Explicit status reporting when a feature is unavailable
- Compiler process kept outside the UI so CLI builds remain scriptable

## Build

Install Visual Studio Build Tools with the Desktop development with C++ workload, then use an x64 developer PowerShell:

```powershell
cmake -S . -B build -G "Visual Studio 17 2022" -A x64
cmake --build build --config Release
```

The binaries are written to `build/Release/mycc.exe` and `build/Release/mycc-ide.exe`.

## Design boundaries

The IDE is deliberately a shell around the compiler contract. It does not embed a second parser, does not execute arbitrary source from inside the editor, and does not hide compiler errors. The current compiler backend emits assembly only, so Run reports that linking is unavailable instead of pretending to execute a program.

Production extensions should be added behind explicit interfaces: a project manifest, file tree, asynchronous build job, diagnostic parser, syntax-highlighting layer, debugger adapter, and linker/toolchain profile. These are separate from the compiler frontend and should not be mixed into `main.c`.

## C17 status

The IDE can edit any C17 source text, but editor capability is not compiler capability. The current compiler supports an integer-focused subset documented in the root README. Full C17 parsing, preprocessing, type checking, linking, code completion, and debugging are not yet implemented.
