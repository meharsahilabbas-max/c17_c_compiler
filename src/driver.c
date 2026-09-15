#include <stdio.h>
#include <stdlib.h>
#include <string.h>

extern int mycc_compile_main(int argc, char **argv);

static int has_suffix(const char *path, const char *suffix) {
    size_t path_length = strlen(path), suffix_length = strlen(suffix);
    return path_length >= suffix_length && strcmp(path + path_length - suffix_length, suffix) == 0;
}

static void usage(void) {
    puts("mycc 0.2.0 - C compiler driver\nUsage: mycc [options] file.c ...\n  -E              preprocess\n  -S              emit assembly\n  -c              assemble object file\n  -o FILE         output path\n  -DNAME[=VALUE]  define macro\n  -UNAME          undefine macro\n  -IPATH          add include path\n  -O0..-O3        optimization level\n  -g              request debug-compatible toolchain mode\n  --help          show help\n  --version       show version");
}

static int run_command(const char *command) {
    int status = system(command);
    if (status != 0) fprintf(stderr, "mycc: tool command failed: %s\n", command);
    return status == 0 ? 0 : 1;
}

static char *assembly_path(const char *source, size_t index) {
    size_t length = strlen(source) + 32;
    char *path = malloc(length);
    if (!path) return NULL;
    const char *dot = strrchr(source, '.');
    if (dot && has_suffix(source, ".c")) snprintf(path, length, "%.*s.mycc-%zu.s", (int)(dot - source), source, index);
    else snprintf(path, length, "%s.mycc-%zu.s", source, index);
    return path;
}

static char *object_path(const char *source, size_t index) {
    size_t length = strlen(source) + 32;
    char *path = malloc(length);
    if (!path) return NULL;
    const char *dot = strrchr(source, '.');
    if (dot && has_suffix(source, ".c")) snprintf(path, length, "%.*s.o", (int)(dot - source), source);
    else snprintf(path, length, "%s-%zu.o", source, index);
    return path;
}

static int compile_assembly(const char *source, const char *assembly, int argc, char **argv) {
    char **core_argv = calloc((size_t)argc + 6, sizeof(*core_argv));
    if (!core_argv) return 1;
    int count = 0;
    core_argv[count++] = "mycc-core"; core_argv[count++] = "-S";
    for (int i = 0; i < argc; ++i) core_argv[count++] = argv[i];
    core_argv[count++] = (char *)source;
    core_argv[count++] = "-o"; core_argv[count++] = (char *)assembly;
    core_argv[count] = NULL;
    int result = mycc_compile_main(count, core_argv);
    free(core_argv);
    return result;
}

int main(int argc, char **argv) {
    if (argc < 2) { usage(); return 2; }
    int assembly_mode = 0, preprocess_mode = 0, object_mode = 0, debug_mode = 0;
    const char *output = NULL;
    const char *inputs[256]; size_t input_count = 0;
    char *forwarded[256]; size_t forwarded_count = 0;
    for (int i = 1; i < argc; ++i) {
        const char *argument = argv[i];
        if (strcmp(argument, "--help") == 0) { usage(); return 0; }
        if (strcmp(argument, "--version") == 0) { puts("mycc 0.2.0"); return 0; }
        if (strcmp(argument, "-S") == 0) { assembly_mode = 1; continue; }
        if (strcmp(argument, "-E") == 0) { preprocess_mode = 1; continue; }
        if (strcmp(argument, "-c") == 0) { object_mode = 1; continue; }
        if (strcmp(argument, "-g") == 0) { debug_mode = 1; continue; }
        if (strcmp(argument, "-o") == 0 && i + 1 < argc) { output = argv[++i]; continue; }
        if (strncmp(argument, "-O", 2) == 0 || strncmp(argument, "-D", 2) == 0 || strncmp(argument, "-U", 2) == 0 || strncmp(argument, "-I", 2) == 0) { forwarded[forwarded_count++] = argv[i]; if ((argument[2] == '\0') && i + 1 < argc) forwarded[forwarded_count++] = argv[++i]; continue; }
        if (argument[0] == '-') { fprintf(stderr, "mycc: unknown option %s\n", argument); return 2; }
        if (input_count == 256) { fprintf(stderr, "mycc: too many input files\n"); return 2; }
        inputs[input_count++] = argument;
    }
    if (input_count == 0 || (preprocess_mode && input_count != 1) || (assembly_mode && input_count != 1)) { fprintf(stderr, "mycc: this mode requires exactly one input file\n"); return 2; }
    if (preprocess_mode || assembly_mode) {
        char *core_argv[260]; int count = 0; core_argv[count++] = "mycc-core"; core_argv[count++] = preprocess_mode ? "-E" : "-S";
        for (size_t i = 0; i < forwarded_count; ++i) core_argv[count++] = forwarded[i]; core_argv[count++] = (char *)inputs[0];
        if (output) { core_argv[count++] = "-o"; core_argv[count++] = (char *)output; }
        return mycc_compile_main(count, core_argv);
    }
    char *objects[256]; size_t object_count = 0;
    for (size_t i = 0; i < input_count; ++i) {
        char *assembly = assembly_path(inputs[i], i); char *object = object_path(inputs[i], i);
        if (!assembly || !object) return 1;
        int result = compile_assembly(inputs[i], assembly, (int)forwarded_count, forwarded);
        if (result != 0) { free(assembly); free(object); return result; }
        char command[4096]; snprintf(command, sizeof(command), "clang%s -c \"%s\" -o \"%s\"", debug_mode ? " -g" : "", assembly, object);
        result = run_command(command); remove(assembly);
        if (result != 0) { free(assembly); free(object); return result; }
        objects[object_count++] = object; if (object_mode) { if (output && input_count == 1 && strcmp(output, object) != 0) { remove(output); rename(object, output); objects[object_count - 1] = NULL; } continue; }
    }
    if (object_mode) { for (size_t i = 0; i < object_count; ++i) free(objects[i]); return 0; }
    const char *executable = output ? output : "a.exe"; char command[8192]; size_t used = (size_t)snprintf(command, sizeof(command), "clang");
    if (debug_mode) used += (size_t)snprintf(command + used, sizeof(command) - used, " -g");
    for (size_t i = 0; i < object_count; ++i) used += (size_t)snprintf(command + used, sizeof(command) - used, " \"%s\"", objects[i]);
    snprintf(command + used, sizeof(command) - used, " -o \"%s\"", executable);
    int result = run_command(command);
    for (size_t i = 0; i < object_count; ++i) { remove(objects[i]); free(objects[i]); }
    return result;
}
