#include <stdlib.h>
#include <string.h>
#include <inttypes.h>
#include <stdio.h>
#include "parser.h"

static void commands_free(w_ast_commands_t *cmds) {
	for(size_t i = 0; i < cmds->len; i++) {
		for(size_t j = 0; j < cmds->ptr[i].len; j++)
			w_ast_free(&cmds->ptr[i].ptr[j]);
		free(cmds->ptr[i].ptr);
	}
	free(cmds->ptr);
}

void w_ast_free(w_ast_t *ast) {
	switch(ast->type) {
		case W_AST_STRING:
		case W_AST_VAR:
			free(ast->string.ptr);
			break;
		case W_AST_COMMANDS: {
			commands_free(&ast->commands);
			break;
		}
		case W_AST_INDEX: {
			w_ast_index_t idx = ast->index;
			w_ast_free(idx.left);
			w_ast_free(idx.right);
			free(idx.left);
			free(idx.right);
			break;
		}
	}
}

w_ast_t w_ast_dup(w_ast_t *ast) {
	switch(ast->type) {
		case W_AST_FLOAT:
		case W_AST_INT:
			return *ast;
		case W_AST_STRING:
			return (w_ast_t){.type = W_AST_STRING, .pos = ast->pos, .string = w_astrdup(&ast->string)};
		case W_AST_VAR:
			return (w_ast_t){.type = W_AST_VAR, .pos = ast->pos, .string = w_astrdup(&ast->string)};
		case W_AST_COMMANDS: {
			w_ast_commands_t *old = &ast->commands;
			w_ast_command_t *cmds = malloc(sizeof(w_ast_command_t)*old->len);
			for(size_t i = 0; i < old->len; i++) {
				w_ast_command_t *oldcmd = &old->ptr[i];
				w_ast_t *cmd = malloc(sizeof(w_ast_t)*oldcmd->len);
				for(size_t i = 0; i < oldcmd->len; i++)
					cmd[i] = w_ast_dup(&oldcmd->ptr[i]);
				cmds[i] = (w_ast_command_t){oldcmd->len, cmd};
			}
			return (w_ast_t){.type = W_AST_COMMANDS, .pos = ast->pos, .commands = (w_ast_commands_t){old->len, cmds}};
		}
		case W_AST_INDEX: {
			w_ast_index_t *idx = &ast->index;
			w_ast_t *left = malloc(sizeof(w_ast_t));
			w_ast_t *right = malloc(sizeof(w_ast_t));
			*left = w_ast_dup(idx->left);
			*right = w_ast_dup(idx->right);
			return (w_ast_t){.type = W_AST_INDEX, .pos = ast->pos, .index = (w_ast_index_t){left, right}};
		}
	}
}

void w_ast_print(w_ast_t *ast) {
	switch(ast->type) {
		case W_AST_STRING: {
			// check whether there are control chars in the string to know whether to quote it or not.
			bool control = false;
			if(ast->string.len == 0)
				control = true;
			else for(size_t i = 0; i < ast->string.len; i++) {
				char c = ast->string.ptr[i];
				switch(c) {
					case ' ':
					case '\t':
					case '[':
					case ']':
					case ';':
						control = true;
						goto after_check;
				}
			}
			after_check:
			if(control)
				putchar('"');
			for(size_t i = 0; i < ast->string.len; i++)
				putchar(ast->string.ptr[i]);
			if(control)
				putchar('"');
			break;
		}
		case W_AST_VAR:
			putchar('$');
			for(size_t i = 0; i < ast->string.len; i++)
				putchar(ast->string.ptr[i]);
			break;
		case W_AST_INT:
			printf("%" PRId64, ast->int_);
			break;
		case W_AST_FLOAT:
			printf("%f", ast->float_);
			break;
		case W_AST_NULL:
			printf("null");
			break;
		case W_AST_COMMANDS: {
			printf("[");
			w_ast_commands_t *cmds = &ast->commands;
			for(size_t i = 0; i < cmds->len; i++) {
				if(i != 0)
					printf(";");
				w_ast_command_t *cmd = &cmds->ptr[i];
				for(size_t j = 0; j < cmd->len; j++) {
					if(j != 0)
						printf(" "); 
					w_ast_print(&cmd->ptr[j]);
				}
			}
			printf("]");
			break;
		}
		case W_AST_INDEX: {
			w_ast_print(ast->index.left);
			printf(":");
			w_ast_print(ast->index.right);
			break;
		}
	}
}

/// Parser state
typedef struct parser {
	char *filename, *code;
	size_t pos, len, start; // len is just the cached strlen() of code
	w_status_t *status;
} parser_t;

/// Get current position
static w_filepos_t get_pos(parser_t *p) {
	size_t line = 0, col = 0;
	for(size_t i = p->pos; i > 0; i--) {
		switch(p->code[i]) {
			case '\n':
				line++;
				col = 0;
				break;
			case '\r':
				break;
			default:
				col++;
		}
	}
	return (w_filepos_t){p->filename, line, col};
}

#define ADD_AST(CMDS, AST) { \
	w_ast_command_t *cmd = &(CMDS)->ptr[(CMDS)->len-1]; \
	cmd->ptr = realloc(cmd->ptr, sizeof(w_ast_t)*(++cmd->len)); \
	cmd->ptr[cmd->len-1] = AST; \
}

// adds a token
static void add(parser_t *p, w_ast_commands_t *cmds) {
	size_t len = p->pos-p->start;
	if(len == 0)
		return; // nothing to add
	char *start = &p->code[p->start];
	char *str;
	w_ast_t ast;
	if(start[0] == '$') {
		// var
		len--;
		start++;
		str = malloc(len);
		memcpy(str, start, len);
		ast = (w_ast_t){
			.type = W_AST_VAR,
			.string = (w_astring_t){len, str}
		};
	}
	else if(w_is_int(start, len)) {
		int64_t n = w_parse_int(start, len);
		ast = (w_ast_t){
			.type = W_AST_INT,
			.int_ = n
		};
	}
	else if(w_is_float(start, len)) {
		double n = w_parse_float(start, len);
		ast = (w_ast_t){
			.type = W_AST_FLOAT,
			.float_ = n
		};
	}
	// I feel like I could make this better but eh
	else if(len == 4 && start[0] == 'n' && start[1] == 'u' && start[2] == 'l' && start[3] == 'l') {
		ast = (w_ast_t){.type = W_AST_NULL};
	}
	else {
		str = malloc(len);
		memcpy(str, start, len);
		ast = (w_ast_t){
			.type = W_AST_STRING,
			.string = (w_astring_t){len, str}
		};
	}
	ast.pos = get_pos(p);
	ADD_AST(cmds, ast);
}

/// Adds the left and right to index expressions
static void resolve_index(parser_t *p, w_ast_commands_t *cmds) {
	while(true) {
		bool found_index = false;
		for(size_t i = 0; i < cmds->len; i++) {
			w_ast_command_t *cmd = &cmds->ptr[i];
			for(size_t j = 0; j < cmd->len; j++) {
				w_ast_t *ast = &cmd->ptr[j];
				switch(ast->type) {
					case W_AST_INDEX: {
						if(ast->index.left != NULL)
							continue; // not placeholder
						found_index = true;
						if(j == 0 || j+1 >= cmd->len) {
							w_status_err(p->status, w_error_new(ast->pos, "Unexpected ':'."));
							return;
						}
						w_filepos_t pos = ast->pos;
						// allocate left and right
						w_ast_t *left = malloc(sizeof(w_ast_t));
						w_ast_t *right = malloc(sizeof(w_ast_t));
						*left = cmd->ptr[j-1];
						*right = cmd->ptr[j+1];
						// shorten length by 2
						memmove(&cmd->ptr[j-1], &cmd->ptr[j+1], sizeof(w_ast_t)*(cmd->len-j-1));
						cmd->ptr = realloc(cmd->ptr, sizeof(w_ast_t)*(cmd->len-2));
						cmd->len -= 2;
						cmd->ptr[j-1] = (w_ast_t){
							.pos = pos,
							.type = W_AST_INDEX,
							.index = (w_ast_index_t){
								.left = left,
								.right = right
							}
						};
						goto cont;
					}
					case W_AST_COMMANDS:
						resolve_index(p, &ast->commands);
						if(p->status->tag != W_STATUS_OK)
							return;
						break;
				}
			}
		}
		if(!found_index)
			break;
		cont:;
	}
}

/// Actual implementation of the parser
static w_ast_t parse(bool top_level, parser_t *p) {
	w_filepos_t cmd_pos = get_pos(p);
	w_ast_commands_t cmds;
	{
		w_ast_command_t *cmd = malloc(sizeof(w_ast_command_t));
		cmd[0] = (w_ast_command_t){0, NULL};
		cmds = (w_ast_commands_t){1, cmd};
	}
	#define ERR_POS(POS, ...) do { \
		commands_free(&cmds); \
		w_status_err(p->status, w_error_new(POS, __VA_ARGS__)); \
		return (w_ast_t){}; \
	} while(0)
	#define ERR(...) ERR_POS(get_pos(p), __VA_ARGS__)
	#define END do { \
		add(p, &cmds); \
		/* remove last command if empty */ \
		if(cmds.ptr[cmds.len-1].len == 0) { \
			if(cmds.len == 1) \
				ERR_POS(cmd_pos, "Empty command."); \
			cmds.ptr = realloc(cmds.ptr, sizeof(w_ast_command_t)*(--cmds.len)); \
		} \
		resolve_index(p, &cmds); \
		if(p->status->tag != W_STATUS_OK) { \
			commands_free(&cmds); \
			return (w_ast_t){}; \
		} \
		return (w_ast_t){ \
			.pos = cmd_pos, \
			.type = W_AST_COMMANDS, \
			.commands = cmds \
		}; \
	} while(0)
	while(true) {
		if(p->pos >= p->len) {
			if(!top_level)
				ERR("Unexpected EOF (possibly missing ']').");
			END;
		}
		switch(p->code[p->pos]) {
			case ' ':
			case '\r':
			case '\n':
			case '\t':
				add(p, &cmds);
				p->start = p->pos+1;
				break;
			case '[': {
				add(p, &cmds);
				p->pos++;
				p->start = p->pos;
				w_ast_t ast = parse(false, p);
				if(p->status->tag != W_STATUS_OK) {
					commands_free(&cmds);
					return (w_ast_t){};
				}
				w_ast_command_t *cmd = &cmds.ptr[cmds.len-1];
				cmd->ptr = realloc(cmd->ptr, sizeof(w_ast_t)*(++cmd->len));
				cmd->ptr[cmd->len-1] = ast;
				p->start = p->pos+1;
				break;
			}
			case ']':
				if(top_level)
					ERR("Unexpected ']'");
				END;
			case ';':

				add(p, &cmds);
				p->start = p->pos+1;
				if(cmds.ptr[cmds.len-1].len == 0)
					break;
				cmds.ptr = realloc(cmds.ptr, sizeof(w_ast_command_t)*(++cmds.len));
				cmds.ptr[cmds.len-1] = (w_ast_command_t){0, NULL};
				break;
			// this doesn't strictly need to be here but it allows x$y to be parsed x $y (string var) instead of as a single string, which is nice for code golf I guess
			case '$':
				add(p, &cmds);
				p->start = p->pos;
				break;
			// comment
			case '#':
				add(p, &cmds);
				p->start = p->pos+1;
				while(p->code[p->pos] != '\n' && p->pos < p->len)
					p->pos++;
				p->start = p->pos+1;
				break;
			// index
			case ':':
				add(p, &cmds);
				p->start = p->pos+1;
				// add placeholder ast
				ADD_AST(&cmds, ((w_ast_t){.pos = get_pos(p), .type = W_AST_INDEX, .index = (w_ast_index_t){.left = NULL, .right = NULL}}));
				break;
			// string
			case '"': {
				w_filepos_t pos = get_pos(p);
				size_t len = 0, buf_len = 4096;
				char *str = malloc(buf_len);
				p->pos++;
				while(true) {
					if(p->pos >= p->len)
						ERR_POS(pos, "No matching '\"'");
					char c = p->code[p->pos];
					switch(c) {
						case '"':
							goto string_done;
						case '\\':
							c = p->code[++p->pos];
							switch(c) {
								case 'a':
									c = '\a';
									break;
								case 'b':
									c = '\b';
									break;
								case 'e':
									c = 0x1B;
									break;
								case 'f':
									c = '\f';
									break;
								case 'n':
									c = '\n';
									break;
								case 'r':
									c = '\r';
									break;
								case 't':
									c = '\t';
									break;
								case 'v':
									c = '\v';
									break;
								// these chars represent themselves
								case '\\':
								case '\'':
								case '"':
									break;
								// TODO: hex and unicode escape codes
							}
							// intentional fallthrough
						default:
							str[len++] = c;
							if(len >= buf_len) {
								buf_len += 4096;
								str = realloc(str, buf_len);
							}
					}
					p->pos++;
				}
				string_done:
				str = realloc(str, len); // remove extra bytes
				w_ast_t ast = (w_ast_t){
					.type = W_AST_STRING,
					.string = (w_astring_t){len, str}
				};
				ADD_AST(&cmds, ast);
				p->start = p->pos+1;
			}
		}
		p->pos++;
	}
	#undef ERR
	#undef END
	#undef ERR_POS
}

#undef ADD_AST

w_ast_t w_parse(w_status_t *status, char *filename, char *code) {
	parser_t parser = (parser_t){filename, code, 0, strlen(code), 0, status};
	return parse(true, &parser);
}

w_astring_t w_astrdup(w_astring_t *str) {
	char *buf = malloc(str->len);
	memcpy(buf, str->ptr, str->len);
	return (w_astring_t){str->len, buf};
}

bool w_astreq(w_astring_t *a, w_astring_t *b) {
	if(a->len != b->len)
		return false;
	for(size_t i = 0; i < a->len; i++)
		if(a->ptr[i] != b->ptr[i])
			return false;
	return true;
}
bool w_astreqc(w_astring_t *a, char *b) {
	size_t blen = strlen(b);
	if(a->len != blen)
		return false;
	for(size_t i = 0; i < a->len; i++)
		if(a->ptr[i] != b[i])
			return false;
	return true;
}
char *w_ast_cstr(w_astring_t *str) {
	char *cstr = malloc(str->len+1);
	memcpy(cstr, str->ptr, str->len);
	cstr[str->len] = '\0';
	return cstr;
}
