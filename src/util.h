/// Contains various utils and other things

#ifndef W_UTIL_H
#define W_UTIL_H

#include <stddef.h>
#include <stdio.h>
#include <stdbool.h>

/// Represents a position in a file
typedef struct w_filepos {
	char *filename;
	size_t line, col;
} w_filepos_t;

/// Maximum length of an error's msg
#define W_MAX_ERROR_LEN 4096

/// Represents an error
typedef struct w_error {
	w_filepos_t pos; /// File position the error occurred at
	char *msg; /// Message of the error
} w_error_t;

w_error_t *w_error_new(w_filepos_t pos, char *fmt, ...); /// Creates an error, uses printf formatting for args
void w_error_print(w_error_t *err, FILE *fp);
void w_error_free(w_error_t *err); /// Frees an error

/// Interpreter/Parser status tag
typedef enum w_status_tag {
	W_STATUS_OK, /// nothing wrong
	W_STATUS_ERR, /// error thrown
	W_STATUS_BREAK, /// break triggered
	W_STATUS_CONTINUE, /// continue triggered
	W_STATUS_RETURN /// Value has been returned
} w_status_tag_t;

typedef struct w_value w_value_t;

/// Interpreter/Parser status
typedef struct w_status {
	w_status_tag_t tag; /// Tag of the status
	union {
		w_error_t *err; /// Error
		w_value_t *ret; /// Return value
	};
} w_status_t;

// ctx status functions

void w_status_free(w_status_t *s); /// Frees values stored in a status
// for simple tags that don't have any data
void w_status_simple(w_status_t *s, w_status_tag_t tag);
// macros using status_simple
#define w_status_ok(s) w_status_simple(s, W_STATUS_OK)
#define w_status_break(s) w_status_simple(s, W_STATUS_BREAK)
#define w_status_continue(s) w_status_simple(s, W_STATUS_CONTINUE)
void w_status_err(w_status_t *s, w_error_t *err); // sets a status to err
void w_status_return(w_status_t *s, w_value_t *val); // returns a value

// when creating a status, set it to this.
#define W_INITIAL_STATUS (w_status_t){.tag = W_STATUS_OK}

void w_print_backtrace(void); // prints backtrace for debugging. GCC dependent, on other compilers using this will result in a linker error.

/// Basically an appendable string
typedef struct w_writer {
	char *buf;
	size_t buf_len, len;
} w_writer_t;

w_writer_t w_writer_new(void); /// Creates a new writer
void w_writer_putch(w_writer_t *w, char c); /// Puts a char into a writer
void w_writer_puts(w_writer_t *w, size_t len, char *str); /// Puts a string into a writer
void w_writer_putcs(w_writer_t *w, char *str); /// Puts a cstring into a writer
void w_writer_resize(w_writer_t *w); /// Resizes the buffer to be exactly the string length

bool w_is_int(char *str, size_t len); /// Checks whether a string is an int
bool w_is_float(char *str, size_t len); /// Checks whether a value is a float
int64_t w_parse_int(char *str, size_t len); /// Converts a string to an int
double w_parse_float(char *str, size_t len); /// Converts a string to a float

#endif
