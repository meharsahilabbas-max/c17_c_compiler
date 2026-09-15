#ifndef MYCC_PREPROCESSOR_H
#define MYCC_PREPROCESSOR_H

#include <stddef.h>

typedef struct {
	const char **defines;
	size_t define_count;
	const char **undefines;
	size_t undefine_count;
	const char **include_paths;
	size_t include_path_count;
} MyccPreprocessorOptions;

/* Returns an owned UTF-8 buffer. The caller releases it with free(). */
char *mycc_preprocess_with_options(const char *source, const char *file,
								   const MyccPreprocessorOptions *options,
								   int *error_count);

char *mycc_preprocess(const char *source, const char *file, int *error_count);

#endif
