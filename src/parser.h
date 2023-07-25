/// Describes the tungstyn parser

#ifndef W_PARSER_H
#define W_PARSER_H

#include <stdint.h>
#include <stdbool.h>

#include "util.h"

/// Type of an AST
typedef enum w_ast_type {
	// literals
	W_AST_STRING, W_AST_VAR, W_AST_INT, W_AST_FLOAT, W_AST_NULL,
	// constructs
	W_AST_COMMANDS, W_AST_INDEX
} w_ast_type_t;

/// A string in the AST. differs from w_string_t as it's not refcounted
typedef struct w_astring {
	size_t len;
	char *ptr;
} w_astring_t;

typedef struct w_ast w_ast_t;

/// Represents a single command
typedef struct w_ast_command {
	size_t len;
	w_ast_t *ptr;
} w_ast_command_t;

/// Command block in the AST
typedef struct w_ast_commands {
	size_t len;
	w_ast_command_t *ptr;
} w_ast_commands_t;

/// Dot expr in the AST
typedef struct w_ast_index {
	w_ast_t *left, *right;
} w_ast_index_t;

/// Tagged union for an AST
typedef struct w_ast {
	w_ast_type_t type; /// AST type
	w_filepos_t pos; /// Position of node
	/// Union of data for all types
	union {
		w_astring_t string; // for both string and var
		int64_t int_;
		double float_;
		w_ast_commands_t commands;
		w_ast_index_t index;
	};
} w_ast_t;

void w_ast_free(w_ast_t *ast); /// Frees an AST
void w_ast_print(w_ast_t *ast); /// Prints an AST
w_ast_t w_ast_dup(w_ast_t *ast); /// Duplicates an AST
w_ast_t w_parse(w_status_t *status, char *filename, char *code); /// Parses a file and returns an AST. 
w_astring_t w_astrdup(w_astring_t *str); /// Duplicates an AST string
bool w_astreq(w_astring_t *a, w_astring_t *b); /// Test if two ast strings are equal
bool w_astreqc(w_astring_t *a, char *b); /// Test if an AST string and a C string are equal
char *w_ast_cstr(w_astring_t *str); /// Converts an ast string to a cstring

#endif
