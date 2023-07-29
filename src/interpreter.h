/// Describes the interpreter

#ifndef W_INTERPRETER_H
#define W_INTERPRETER_H

#include <stdint.h>
#include <stdio.h>

#include "parser.h"
#include "util.h"

#include "hashtable.h"

typedef uint32_t w_refcount_t;
typedef uint32_t w_scope_t;

/// Type of a value
typedef enum w_value_type {
	// non-refcounted
	W_VALUE_NULL, // null value
	W_VALUE_INT, // 64-bit signed integer
	W_VALUE_FLOAT, // 64-bit floating point
	// refcounted
	W_VALUE_EXTERNCMD, // external command
	W_VALUE_COMMAND, // internal command
	W_VALUE_STRING,
	W_VALUE_LIST,
	W_VALUE_MAP
} w_value_type_t;

typedef struct w_value w_value_t;

/// Represents a string
typedef struct w_string {
	w_refcount_t refcount; /// Reference count
	size_t len; /// Length of the string
	char *ptr; /// String data
} w_string_t;

/// Represents a list
typedef struct w_list {
	w_refcount_t refcount; /// Reference count
	size_t len; /// Length of the list
	w_value_t *ptr; /// Contents of the list
} w_list_t;

typedef struct w_map w_map_t;

/// Arguments given to a function
typedef struct w_args {
	size_t len; /// Number of arguments
	w_ast_t *ptr; /// Arguments as ASTs - they must be w_eval()'ed in order to get the actual value.
} w_args_t;

typedef struct w_var w_var_t;
typedef struct w_ecmd w_ecmd_t;
typedef struct w_ctx w_ctx_t;
typedef struct w_cmd w_cmd_t;

/// A value
typedef struct w_value {
	w_value_type_t type;
	union {
		int64_t int_;
		double float_;
		w_ecmd_t *externcmd;
		w_cmd_t *cmd;
		w_string_t *string;
		w_list_t *list;
		w_map_t *map;
	};
} w_value_t;

typedef w_value_t (*w_externcmd_t)(w_filepos_t, w_ctx_t *, w_value_t *, w_value_t *, w_args_t); // unfortunately I can't typedef this earlier, since it's used in w_value_t.

// simple macro to make my life easier
#define W_COMMAND(NAME) w_value_t NAME(w_filepos_t pos, w_ctx_t *ctx, w_value_t *this, w_value_t *obj, w_args_t args)

/// An external command
struct w_ecmd {
	w_refcount_t refcount;
	w_externcmd_t cmd;
	w_value_t *obj; // an object being acted upon - if you have something like $l:my-command, this will be $l (this is used by the list functions, for example)
};

/// A command argument
typedef struct w_cmd_arg {
	w_astring_t name; /// Name of argument
} w_cmd_arg_t;

/// internal command
struct w_cmd {
	w_refcount_t refcount; /// Reference count
	size_t argc; /// Number of arguments
	w_cmd_arg_t *args; /// Arguments
	w_ast_t impl; /// Implementation
	w_value_t *this; /// $this pointer
};

/// A var in the var table
typedef struct w_var {
	w_scope_t scope;
	w_value_t *val;
} w_var_t;

/// vartable
W_HASHTABLE_H(w_vartable, w_var_t, w_scope_t);
/// Represents a map
W_HASHTABLE_H(w_map, w_value_t, w_refcount_t);

/// An interpreting context
struct w_ctx {
	w_scope_t scope; /// Current scope
	w_vartable_t vartable; // the vartable
	w_status_t *status; /// Interpreter status
};

char *w_typename(w_value_type_t t); /// Returns a string representing the name of a type.

void w_value_release(w_value_t *val); /// Releases a value, decrementing its refcount and freeing if it's 0
void w_value_ref(w_value_t *val); /// Increases a value's refcount
void w_value_print(w_value_t *val, FILE *fp); /// Prints a value to a given file
bool w_value_truthy(w_value_t *v); /// Whether a value is truthy
w_value_t w_value_index(w_ctx_t *ctx, w_value_t *left, w_value_t *right); /// Indexes a value. NOTE: does not give a file position.
char *w_cstring(w_string_t *str); /// Converts a string to a C string
bool w_streqc(w_string_t *a, char *b); /// Compares a w_string_t to a C string
w_value_t w_value_clone(w_value_t *val); /// Performs a shallow clone of a value

// operations
// note: the errors these return do not give file positions. They must be set after usage to ensure that errors have correct file positions.
w_value_t w_value_add(w_ctx_t *ctx, w_value_t *a, w_value_t *b);
w_value_t w_value_sub(w_ctx_t *ctx, w_value_t *a, w_value_t *b);
w_value_t w_value_mul(w_ctx_t *ctx, w_value_t *a, w_value_t *b);
w_value_t w_value_div(w_ctx_t *ctx, w_value_t *a, w_value_t *b);
w_value_t w_value_mod(w_ctx_t *ctx, w_value_t *a, w_value_t *b);

// boolean operations
// these also do not give file positions
bool w_value_equal(w_value_t *a, w_value_t *b); /// Compares two values
bool w_value_lt(w_ctx_t *ctx, w_value_t *a, w_value_t *b);
bool w_value_lte(w_ctx_t *ctx, w_value_t *a, w_value_t *b);
bool w_value_gt(w_ctx_t *ctx, w_value_t *a, w_value_t *b);
bool w_value_gte(w_ctx_t *ctx, w_value_t *a, w_value_t *b);

// conversions
w_value_t w_value_toint(w_value_t *val); // converts a value to an int (returns null on failure)
w_value_t w_value_tofloat(w_value_t *val); // converts a value to a float (returns null on failure)
w_value_t w_value_tostring(w_value_t *val); /// Converts a value to a string

// ctx functions

w_ctx_t w_empty_ctx(w_status_t *status); /// Creates an empty context
w_ctx_t w_default_ctx(w_status_t *status); /// Creates a context with all standard commands
w_value_t *w_ctx_get(w_ctx_t *ctx, w_astring_t *str); /// Gets a variable. Returns NULL if such a variable does not exist.
void w_ctx_let(w_ctx_t *ctx, w_astring_t *str, w_value_t val); /// Declares a variable
void w_ctx_set(w_ctx_t *ctx, w_astring_t *str, w_value_t val); /// Sets a variable
void w_ctx_del(w_ctx_t *ctx, w_astring_t *str); /// Deletes a variable
w_value_t *w_ctx_getc(w_ctx_t *ctx, char *cstr); /// Same as w_ctx_set, except using a cstring
void w_ctx_setc(w_ctx_t *ctx, char *cstr, w_value_t val); /// Same as w_ctx_get, except using a cstring.
void w_ctx_letc(w_ctx_t *ctx, char *cstr, w_value_t val); /// Same as w_ctx_let, except using a cstring.
w_ctx_t w_ctx_clone(w_ctx_t *ctx); /// Clones a context, incrementing the scope.
void w_ctx_free(w_ctx_t *ctx); /// Frees a context


w_value_t w_eval(w_ctx_t *ctx, w_ast_t *ast); /// Evaluates an AST
w_value_t w_evals(w_ctx_t *ctx, w_ctx_t *sub, w_ast_t *ast); /// Evaluates with a subcontext
w_value_t w_evalt(w_ctx_t *ctx, w_value_t *this, w_ast_t *ast); // Evaluates with a this pointer
w_value_t w_evalst(w_ctx_t *ctx, w_ctx_t *sub, w_value_t *this, w_ast_t *ast); /// Evaluates with a subcontext and a this pointer

#endif
