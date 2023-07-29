#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#ifdef HAS_READLINE
	#include <readline/readline.h>
	#include <readline/history.h>
#endif

#include "parser.h"
#include "interpreter.h"
#include "info.h"
#include "main.h"

int main(int argc, char **argv) {
	// run interpreter if no arguments
	if(argc == 1) {
		repl();
		return 0;
	}
	// TODO: better program argument parsing
	size_t i;
	for(i = 1; i < argc; i++) {
		char *arg = argv[i];
		if(arg[0] == '-') {
			if(strcmp(arg, "-") == 0)
				break;
			if(strcmp(arg, "--help") == 0 || strcmp(arg, "-h") == 0) {
				printf("Usage: %s [interpreter arguments] <filename> [program arguments]\n", argv[0]);
				printf("Running with no arguments will launch an interactive REPL mode. Running with filename '-' will read the program from stdin.\n\n");
				printf("Interpreter Arguments:\n");
				printf("-h | --help\tShows this help information\n");
				return 0;
			}
			continue;	
		}
		break;
	}
	char *filename = argv[i];
	FILE *fp;
	if(strcmp(filename, "-") == 0)
		fp = stdin;
	else
		fp = fopen(filename, "r");
	if(fp == NULL) {
		printf("Could not open file %s.", filename);
		return 1;
	}
	char *code;
	// read to string
	{
		size_t len = 4096, codelen = 0;
		code = malloc(len);
		char c;
		while((c = fgetc(fp)) != EOF) {
			code[codelen++] = c;
			if(codelen+2 == len) {
				len += 4096;
				code = realloc(code, len);
			}
		}
		code[codelen] = '\0';
	}
	if(fp != stdin)
		fclose(fp);
	w_status_t status = W_INITIAL_STATUS;
	w_ast_t ast = w_parse(&status, filename, code);
	if(status.tag != W_STATUS_OK) {
		w_error_print(status.err, stdout);
		w_status_free(&status);
		free(code);
		return 2;
	}
	w_ctx_t ctx = w_default_ctx(&status);
	w_value_t val = w_eval(&ctx, &ast);
	if(status.tag != W_STATUS_OK) {
		w_error_print(status.err, stdout);
		w_status_free(&status);
		w_ctx_free(&ctx);
		w_ast_free(&ast);
		free(code);
		return 2;
	}
	w_value_release(&val);
	w_ctx_free(&ctx);
	w_ast_free(&ast);
	free(code);
}

void repl(void) {
	printf("wi - Tungstyn Interpreter, Version %s\n", W_VERSION);
	printf("Copyright (C) 2023, wi contributors.\n");
	printf("License GPLv3+: GNU GPL version 3 or later <http://gnu.org/licenses/gpl.html>\n\n");
	printf("This is free software; you are free to change and redistribute it.\n");
	printf("There is NO WARRANTY, to the extent permitted by law.\n\n");
	printf("To quit, either send an EOF, or type ':q'\n\n");
	w_status_t status = W_INITIAL_STATUS;
	w_ctx_t ctx = w_default_ctx(&status);
	w_ctx_t sub_ctx = w_ctx_clone(&ctx);
	while(true) {
		w_status_ok(&status);
		#ifdef HAS_READLINE
			char *line = readline("> ");
		#else
			char *line = w_readline("> ");
		#endif
		if(line == NULL)
			break;
		#ifdef HAS_READLINE
			add_history(line);
		#endif
		if(line[0] == ':') {
			if(strcmp(line, ":q") == 0) {
				free(line);
				break;
			}
			printf("Unknown REPL command %s.\n", line);
			free(line);
			continue;
		}
		w_ast_t ast = w_parse(&status, "<repl>", line);
		if(status.tag == W_STATUS_ERR) {
			w_error_print(status.err, stdout);
			free(line);
			continue;
		}
		// goto skip;
		w_value_t val = w_evals(&ctx, &sub_ctx, &ast);
		if(status.tag == W_STATUS_ERR) {
			w_error_print(status.err, stdout);
			w_ast_free(&ast);
			free(line);
			continue;
		}
		if(val.type != W_VALUE_NULL)
			w_value_print(&val, stdout);
		w_status_free(&status);
		w_value_release(&val);
		// skip:
		// w_ast_print(&ast);
		printf("\n");
		w_ast_free(&ast);
		free(line);
	}
	w_ctx_free(&ctx);
	w_ctx_free(&sub_ctx);
	#ifdef HAS_READLINE
		clear_history();
	#endif
}

