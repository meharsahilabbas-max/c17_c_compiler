#include "mycc_preprocessor.h"

#include <ctype.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_MACROS 512
#define MAX_CONDITIONS 128
#define MAX_LINE 65536

typedef struct {
    char name[128];
    char value[1024];
    char parameters[16][64];
    size_t parameter_count;
    int function_like;
    int variadic;
    int defined;
} Macro;

typedef struct {
    Macro macros[MAX_MACROS];
    size_t macro_count;
    int active[MAX_CONDITIONS];
    int parent_active[MAX_CONDITIONS];
    int branch_taken[MAX_CONDITIONS];
    size_t condition_count;
    int errors;
    char *output;
    size_t length;
    size_t capacity;
    const char *file;
    const MyccPreprocessorOptions *options;
} Preprocessor;

static int identifier_start(char c);
static int identifier_continue(char c);

static void report(Preprocessor *pp, size_t line, const char *message) {
    fprintf(stderr, "%s:%zu: error: %s\n", pp->file, line, message);
    pp->errors++;
}

static void append_text(Preprocessor *pp, const char *text, size_t length) {
    if (length > SIZE_MAX - pp->length - 1) {
        report(pp, 0, "preprocessor output is too large");
        return;
    }
    size_t required = pp->length + length + 1;
    if (required > pp->capacity) {
        size_t capacity = pp->capacity ? pp->capacity : 4096;
        while (capacity < required) {
            if (capacity > SIZE_MAX / 2) { capacity = required; break; }
            capacity *= 2;
        }
        char *resized = realloc(pp->output, capacity);
        if (!resized) { report(pp, 0, "out of memory"); return; }
        pp->output = resized;
        pp->capacity = capacity;
    }
    memcpy(pp->output + pp->length, text, length);
    pp->length += length;
    pp->output[pp->length] = '\0';
}

static int is_active(const Preprocessor *pp) {
    return pp->condition_count == 0 || pp->active[pp->condition_count - 1];
}

static Macro *find_macro(Preprocessor *pp, const char *name) {
    for (size_t i = 0; i < pp->macro_count; ++i) {
        if (strcmp(pp->macros[i].name, name) == 0) return &pp->macros[i];
    }
    return NULL;
}

static void define_macro(Preprocessor *pp, const char *name, const char *value) {
    Macro *macro = find_macro(pp, name);
    if (!macro) {
        if (pp->macro_count == MAX_MACROS) { report(pp, 0, "too many macros"); return; }
        macro = &pp->macros[pp->macro_count++];
        strncpy(macro->name, name, sizeof(macro->name) - 1);
        macro->name[sizeof(macro->name) - 1] = '\0';
    }
    strncpy(macro->value, value, sizeof(macro->value) - 1);
    macro->value[sizeof(macro->value) - 1] = '\0';
    macro->defined = 1;
}

static void define_function_macro(Preprocessor *pp, const char *name, const char *parameters, const char *value) {
    Macro *macro = find_macro(pp, name);
    if (!macro) {
        if (pp->macro_count == MAX_MACROS) { report(pp, 0, "too many macros"); return; }
        macro = &pp->macros[pp->macro_count++];
        strncpy(macro->name, name, sizeof(macro->name) - 1); macro->name[sizeof(macro->name) - 1] = '\0';
    }
    macro->function_like = 1; macro->parameter_count = 0; macro->variadic = 0;
    const char *cursor = parameters;
    while (*cursor && *cursor != ')') {
        while (isspace((unsigned char)*cursor) || *cursor == ',') ++cursor;
        if (cursor[0] == '.' && cursor[1] == '.' && cursor[2] == '.') { macro->variadic = 1; cursor += 3; while (*cursor && *cursor != ')') ++cursor; break; }
        size_t length = 0; while (identifier_continue(*cursor) && length + 1 < sizeof(macro->parameters[0])) macro->parameters[macro->parameter_count][length++] = *cursor++;
        macro->parameters[macro->parameter_count][length] = '\0';
        if (length && macro->parameter_count + 1 < 16) ++macro->parameter_count;
        while (*cursor && *cursor != ',' && *cursor != ')') ++cursor;
    }
    strncpy(macro->value, value, sizeof(macro->value) - 1); macro->value[sizeof(macro->value) - 1] = '\0'; macro->defined = 1;
}

static void undefine_macro(Preprocessor *pp, const char *name) {
    Macro *macro = find_macro(pp, name);
    if (macro) macro->defined = 0;
}

static FILE *open_include(Preprocessor *pp, const char *name, char *resolved, size_t resolved_size) {
    FILE *file = fopen(name, "rb");
    if (file) { strncpy(resolved, name, resolved_size - 1); resolved[resolved_size - 1] = '\0'; return file; }
    if (!pp->options) return NULL;
    for (size_t i = 0; i < pp->options->include_path_count; ++i) {
        int written = snprintf(resolved, resolved_size, "%s/%s", pp->options->include_paths[i], name);
        if (written < 0 || (size_t)written >= resolved_size) continue;
        file = fopen(resolved, "rb");
        if (file) return file;
    }
    return NULL;
}

static int identifier_start(char c) { return isalpha((unsigned char)c) || c == '_'; }
static int identifier_continue(char c) { return isalnum((unsigned char)c) || c == '_'; }

static void expand_line(Preprocessor *pp, const char *line) {
    const char *cursor = line;
    while (*cursor) {
        if (identifier_start(*cursor)) {
            const char *start = cursor++;
            while (identifier_continue(*cursor)) ++cursor;
            size_t length = (size_t)(cursor - start);
            char name[128];
            if (length >= sizeof(name)) { append_text(pp, start, length); continue; }
            memcpy(name, start, length); name[length] = '\0';
            Macro *macro = find_macro(pp, name);
            if (macro && macro->defined && macro->function_like && *cursor == '(') {
                ++cursor; const char *argument_start = cursor; const char *arguments[16] = {0}; size_t argument_lengths[16] = {0}; size_t argument_count = 0; int depth = 0;
                while (*cursor && (depth > 0 || *cursor != ')')) { if (*cursor == '(') ++depth; if (*cursor == ')' && depth > 0) --depth; if (*cursor == ',' && depth == 0 && argument_count < 16) { arguments[argument_count] = argument_start; argument_lengths[argument_count++] = (size_t)(cursor - argument_start); argument_start = cursor + 1; } ++cursor; }
                if (argument_count < 16 && argument_start < cursor) { arguments[argument_count] = argument_start; argument_lengths[argument_count++] = (size_t)(cursor - argument_start); }
                if (*cursor == ')') ++cursor;
                const char *replacement = macro->value;
                while (*replacement) { if (identifier_start(*replacement)) { const char *replacement_start = replacement++; while (identifier_continue(*replacement)) ++replacement; size_t replacement_length = (size_t)(replacement - replacement_start); bool substituted = false; for (size_t parameter = 0; parameter < macro->parameter_count && parameter < argument_count; ++parameter) if (strlen(macro->parameters[parameter]) == replacement_length && strncmp(replacement_start, macro->parameters[parameter], replacement_length) == 0) { append_text(pp, arguments[parameter], argument_lengths[parameter]); substituted = true; break; } if (!substituted) append_text(pp, replacement_start, replacement_length); } else append_text(pp, replacement++, 1); }
            } else if (macro && macro->defined) append_text(pp, macro->value, strlen(macro->value));
            else append_text(pp, start, length);
        } else {
            append_text(pp, cursor++, 1);
        }
    }
    append_text(pp, "\n", 1);
}

static void process_line(Preprocessor *pp, char *line, size_t line_number) {
    char *cursor = line;
    while (isspace((unsigned char)*cursor)) ++cursor;
    if (*cursor != '#') {
        if (is_active(pp)) expand_line(pp, line);
        return;
    }
    ++cursor;
    while (isspace((unsigned char)*cursor)) ++cursor;
    char directive[32] = {0};
    size_t directive_length = 0;
    while (identifier_continue(*cursor) && directive_length + 1 < sizeof(directive)) directive[directive_length++] = *cursor++;
    directive[directive_length] = '\0';
    while (isspace((unsigned char)*cursor)) ++cursor;

    if (strcmp(directive, "define") == 0) {
        if (!is_active(pp)) return;
        char name[128] = {0}; size_t length = 0;
        while (identifier_continue(*cursor) && length + 1 < sizeof(name)) name[length++] = *cursor++;
        name[length] = '\0';
        if (*cursor == '(') {
            const char *parameter_start = ++cursor; int depth = 1; while (*cursor && depth) { if (*cursor == '(') ++depth; else if (*cursor == ')') --depth; ++cursor; }
            if (depth != 0) { report(pp, line_number, "unterminated function-like macro parameters"); return; }
            char parameters[1024]; size_t parameter_length = (size_t)(cursor - parameter_start); if (parameter_length >= sizeof(parameters)) { report(pp, line_number, "macro parameter list is too long"); return; } memcpy(parameters, parameter_start, parameter_length); parameters[parameter_length] = '\0'; while (isspace((unsigned char)*cursor)) ++cursor; define_function_macro(pp, name, parameters, cursor); return;
        }
        while (isspace((unsigned char)*cursor)) ++cursor;
        if (name[0] == '\0') report(pp, line_number, "expected macro name");
        else define_macro(pp, name, cursor);
    } else if (strcmp(directive, "undef") == 0) {
        if (is_active(pp)) undefine_macro(pp, cursor);
    } else if (strcmp(directive, "ifdef") == 0 || strcmp(directive, "ifndef") == 0) {
        if (pp->condition_count == MAX_CONDITIONS) { report(pp, line_number, "conditional nesting is too deep"); return; }
        Macro *macro = find_macro(pp, cursor);
        int condition = macro && macro->defined;
        if (strcmp(directive, "ifndef") == 0) condition = !condition;
        size_t index = pp->condition_count++;
        pp->parent_active[index] = is_active(pp);
        pp->branch_taken[index] = condition;
        pp->active[index] = pp->parent_active[index] && condition;
    } else if (strcmp(directive, "if") == 0) {
        if (pp->condition_count == MAX_CONDITIONS) { report(pp, line_number, "conditional nesting is too deep"); return; }
        int condition = strcmp(cursor, "0") != 0;
        size_t index = pp->condition_count++;
        pp->parent_active[index] = is_active(pp);
        pp->branch_taken[index] = condition;
        pp->active[index] = pp->parent_active[index] && condition;
    } else if (strcmp(directive, "elif") == 0) {
        if (pp->condition_count == 0) report(pp, line_number, "#elif without matching conditional");
        else {
            size_t index = pp->condition_count - 1;
            int condition = strcmp(cursor, "0") != 0;
            pp->active[index] = pp->parent_active[index] && !pp->branch_taken[index] && condition;
            if (condition) pp->branch_taken[index] = 1;
        }
    } else if (strcmp(directive, "else") == 0) {
        if (pp->condition_count == 0) report(pp, line_number, "#else without matching conditional");
        else {
            size_t index = pp->condition_count - 1;
            pp->active[index] = pp->parent_active[index] && !pp->branch_taken[index];
            pp->branch_taken[index] = 1;
        }
    } else if (strcmp(directive, "endif") == 0) {
        if (pp->condition_count == 0) report(pp, line_number, "#endif without matching conditional");
        else --pp->condition_count;
    } else if (strcmp(directive, "include") == 0) {
        if (is_active(pp)) {
            char include_name[260] = {0};
            size_t include_length = strlen(cursor);
            if (include_length >= 3 && cursor[0] == '"' && cursor[include_length - 1] == '"') {
                include_length -= 2;
                if (include_length < sizeof(include_name)) {
                    memcpy(include_name, cursor + 1, include_length); include_name[include_length] = '\0';
                    char resolved_name[1024] = {0};
                    FILE *included = open_include(pp, include_name, resolved_name, sizeof(resolved_name));
                    if (included) {
                        fseek(included, 0, SEEK_END); long size = ftell(included); rewind(included);
                        if (size >= 0 && size <= 16 * 1024 * 1024) {
                            char *contents = malloc((size_t)size + 1);
                            if (contents && fread(contents, 1, (size_t)size, included) == (size_t)size) {
                                contents[size] = '\0';
                                const char *saved_file = pp->file; pp->file = resolved_name;
                                const char *nested = contents; size_t nested_line = 1;
                                while (*nested) { const char *end = strchr(nested, '\n'); size_t length = end ? (size_t)(end - nested) : strlen(nested); char nested_line_text[MAX_LINE]; if (length >= sizeof(nested_line_text)) { report(pp, nested_line, "included line is too long"); break; } memcpy(nested_line_text, nested, length); nested_line_text[length] = '\0'; process_line(pp, nested_line_text, nested_line++); nested = end ? end + 1 : nested + length; }
                                pp->file = saved_file;
                            } else report(pp, line_number, "could not read included file");
                            free(contents);
                        } else report(pp, line_number, "included file is too large");
                        fclose(included);
                    } else report(pp, line_number, "could not open included file");
                } else report(pp, line_number, "include path is too long");
            } else report(pp, line_number, "only quoted local includes are supported");
        }
    } else if (strcmp(directive, "error") == 0) {
        if (is_active(pp)) report(pp, line_number, cursor[0] ? cursor : "#error");
    } else if (strcmp(directive, "pragma") != 0 && directive[0] != '\0') {
        if (is_active(pp)) report(pp, line_number, "unsupported preprocessor directive");
    }
}

char *mycc_preprocess_with_options(const char *source, const char *file, const MyccPreprocessorOptions *options, int *error_count) {
    Preprocessor *pp = calloc(1, sizeof(*pp));
    if (!pp) { if (error_count) *error_count = 1; return NULL; }
    pp->file = file; pp->options = options;
    if (options) {
        for (size_t i = 0; i < options->define_count; ++i) {
            char definition[1200]; strncpy(definition, options->defines[i], sizeof(definition) - 1); definition[sizeof(definition) - 1] = '\0';
            char *equals = strchr(definition, '='); if (equals) *equals++ = '\0'; else equals = "1"; define_macro(pp, definition, equals);
        }
        for (size_t i = 0; i < options->undefine_count; ++i) undefine_macro(pp, options->undefines[i]);
    }
    char *line = malloc(MAX_LINE);
    if (!line) { free(pp); if (error_count) *error_count = 1; return NULL; }
    const char *cursor = source; size_t line_number = 1;
    while (*cursor) {
        const char *end = strchr(cursor, '\n');
        size_t length = end ? (size_t)(end - cursor) : strlen(cursor);
        if (length >= MAX_LINE) { report(pp, line_number, "source line is too long"); break; }
        memcpy(line, cursor, length); line[length] = '\0'; process_line(pp, line, line_number++);
        cursor = end ? end + 1 : cursor + length;
    }
    if (pp->condition_count != 0) report(pp, line_number, "unterminated conditional directive");
    if (!pp->output) { pp->output = calloc(1, 1); if (!pp->output) pp->errors++; }
    if (error_count) *error_count = pp->errors;
    char *output = pp->output;
    free(line); free(pp);
    return output;
}

char *mycc_preprocess(const char *source, const char *file, int *error_count) {
    return mycc_preprocess_with_options(source, file, NULL, error_count);
}
