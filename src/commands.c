#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>

#ifdef HAS_READLINE
	#include <readline/readline.h>
	#include <readline/history.h>
#endif

#include "commands.h"
#include "info.h"

// arg checking macros

#define ARGS_NONE(NAME) \
	if(args.len != 0) { \
		w_status_err(ctx->status, w_error_new(pos, NAME " takes no arguments.")); \
		return (w_value_t){}; \
	}

#define ARGS_EQUAL(NAME, AMT) \
	if(args.len != AMT) { \
		w_status_err(ctx->status, w_error_new(pos, NAME " takes exactly " #AMT " arguments.")); \
		return (w_value_t){}; \
	}

#define ARGS_GTE(NAME, AMT) \
	if(args.len < AMT) { \
		w_status_err(ctx->status, w_error_new(pos, NAME " takes at least " #AMT " arguments.")); \
		return (w_value_t){}; \
	}

#define ARGS_BETWEEN(NAME, MIN, MAX) \
	if(args.len < MIN || args.len > MAX) { \
		w_status_err(ctx->status, w_error_new(pos, NAME " takes between " #MIN " and " #MAX " arguments.")); \
		return (w_value_t){}; \
	}

// IO

W_COMMAND(w_cmd_echo) {
	for(size_t i = 0; i < args.len; i++) {
		w_value_t val = w_evalt(ctx, this, &args.ptr[i]);
		if(ctx->status->tag != W_STATUS_OK)
			return (w_value_t){};
		w_value_print(&val, stdout);
		w_value_release(&val);
	}
	fflush(stdout);
	return (w_value_t){.type = W_VALUE_NULL};
}

W_COMMAND(w_cmd_echoln) {
	w_value_t val = w_cmd_echo(pos, ctx, this, obj, args);
	if(ctx->status->tag != W_STATUS_OK)
		return (w_value_t){};
	printf("\n");
	fflush(stdout);
	return val;
}

W_COMMAND(w_cmd_read) {
	ARGS_BETWEEN("read", 0, 1);
	w_writer_t w = w_writer_new();
	FILE *fp = stdin;
	if(args.len == 1) {
		w_value_t v_ = w_evalt(ctx, this, &args.ptr[0]);
		w_value_t v = w_value_tostring(&v_);
		w_value_release(&v_);
		if(ctx->status->tag != W_STATUS_OK)
			return (w_value_t){};
		char *str = w_cstring(v.string);
		fp = fopen(str, "r");
		if(fp == NULL) {
			w_status_err(ctx->status, w_error_new(pos, "Could not open file '%s'.", str));
			free(str);
			return (w_value_t){};
		}
		w_value_release(&v);
		free(str);
	}
	char c;
	while((c = fgetc(fp)) != EOF)
		w_writer_putch(&w, c);
	w_writer_resize(&w);
	w_string_t *str = malloc(sizeof(w_string_t));
	*str = (w_string_t){1, w.len, w.buf};
	if(args.len == 1)
		fclose(fp);
	return (w_value_t){.type = W_VALUE_STRING, .string = str};
}

W_COMMAND(w_cmd_readln) {
	// first echo prompt
	w_value_t val = w_cmd_echo(pos, ctx, this, obj, args);
	if(ctx->status->tag != W_STATUS_OK)
		return (w_value_t){};
	// then readline
	#ifdef HAS_READLINE
		char *line = readline("");
	#else
		char *line = w_readline("");
	#endif
	if(line == NULL) {
		w_status_err(ctx->status, w_error_new(pos, "EOF"));
		return (w_value_t){};
	}
	#ifdef HAS_READLINE
		add_history(line);
	#endif
	w_string_t *str = malloc(sizeof(w_string_t));
	*str = (w_string_t){1, strlen(line), line};
	return (w_value_t){.type = W_VALUE_STRING, .string = str};
}

// ops

#define OP_CMD(OP) W_COMMAND(w_cmd_##OP) { \
	ARGS_GTE(#OP, 1); \
	w_value_t acc = w_evalt(ctx, this, &args.ptr[0]); \
	if(ctx->status->tag != W_STATUS_OK) \
		return (w_value_t){}; \
	for(size_t i = 1; i < args.len; i++) { \
		w_value_t v = w_evalt(ctx, this, &args.ptr[i]); \
		if(ctx->status->tag != W_STATUS_OK) { \
			w_value_release(&acc); \
			return (w_value_t){}; \
		} \
		w_value_t n = w_value_##OP(ctx, &acc, &v); \
		w_value_release(&v); \
		w_value_release(&acc); \
		if(ctx->status->tag != W_STATUS_OK) { \
			ctx->status->err->pos = pos; \
			return (w_value_t){}; \
		} \
		acc = n; \
	} \
	return acc; \
}

OP_CMD(add);
OP_CMD(sub);
OP_CMD(mul);
OP_CMD(div);
OP_CMD(mod);

#undef OP_CMD

W_COMMAND(w_cmd_or) {
	ARGS_GTE("or", 2);
	for(size_t i = 0; i < args.len; i++) {
		w_value_t v = w_evalt(ctx, this, &args.ptr[i]);
		if(w_value_truthy(&v))
			return v;
		w_value_release(&v);
	}
	return (w_value_t){.type = W_VALUE_INT, .int_ = 0};
}

W_COMMAND(w_cmd_and) {
	ARGS_GTE("and", 2);
	for(size_t i = 0; i < args.len; i++) {
		w_value_t v = w_evalt(ctx, this, &args.ptr[i]);
		if(!w_value_truthy(&v))
			return v;
		w_value_release(&v);
	}
	return (w_value_t){.type = W_VALUE_INT, .int_ = 1};
}

// type conversions

#define TYPE_CMD(NAME, FUNC) \
	W_COMMAND(w_cmd_##NAME) { \
		ARGS_EQUAL(#NAME, 1); \
		w_value_t v = w_evalt(ctx, this, &args.ptr[0]); \
		w_value_t ret = FUNC(&v); \
		w_value_release(&v); \
		if(ctx->status->tag != W_STATUS_OK) \
			return (w_value_t){}; \
		return ret; \
	}

TYPE_CMD(int, w_value_toint);
TYPE_CMD(float, w_value_tofloat);
TYPE_CMD(string, w_value_tostring);

#undef TYPE_CMD

// context manipulation

#define VAR_CMD(NAME, CMDNAME, FN) \
	W_COMMAND(w_cmd_##NAME) { \
		ARGS_GTE(CMDNAME, 2); \
		if(args.len%2 != 0) { \
			w_status_err(ctx->status, w_error_new(pos, CMDNAME " argument count must be a multiple of 2.")); \
			return (w_value_t){}; \
		} \
		w_value_t v; \
		for(size_t i = 0; i < args.len; i += 2) { \
			w_ast_t *var = &args.ptr[i]; \
			if(var->type != W_AST_VAR) { \
				w_status_err(ctx->status, w_error_new(pos, CMDNAME " can only bind variables.")); \
				return (w_value_t){}; \
			} \
			v = w_evalt(ctx, this, &args.ptr[i+1]); \
			if(ctx->status->tag != W_STATUS_OK) \
				return (w_value_t){}; \
			FN(ctx, &var->string, v); \
			if(ctx->status->tag != W_STATUS_OK) { \
				ctx->status->err->pos = pos; \
				return (w_value_t){}; \
			} \
		} \
		w_value_ref(&v); \
		return v; \
	}

VAR_CMD(set, "set!", w_ctx_set);
VAR_CMD(let, "let!", w_ctx_let);

#undef VAR_CMD

W_COMMAND(w_cmd_del) {
	ARGS_GTE("del!", 1);
	for(size_t i = 0; i < args.len; i++) {
		w_ast_t *var = &args.ptr[i];
		if(var->type != W_AST_VAR) {
			w_status_err(ctx->status, w_error_new(pos, "del! can only delete variables."));
			return (w_value_t){};
		}
		w_ctx_del(ctx, &var->string);
	}
	return (w_value_t){.type = W_VALUE_NULL};
}

W_COMMAND(w_cmd_swap) {
	ARGS_EQUAL("swap!", 2);
	w_ast_t *a = &args.ptr[0];
	w_ast_t *b = &args.ptr[1];
	if(a->type != W_AST_VAR || b->type != W_AST_VAR) {
		w_status_err(ctx->status, w_error_new(pos, "swap! can only operate on variables."));
		return (w_value_t){};
	}
	w_value_t *va = w_ctx_get(ctx, &a->string);
	w_value_t *vb = w_ctx_get(ctx, &b->string);
	w_value_t tmp = *vb;
	*vb = *va;
	*va = tmp;
	return (w_value_t){.type = W_VALUE_NULL};
}

// boolean ops

W_COMMAND(w_cmd_equ) {
	ARGS_EQUAL("=", 2);
	w_value_t a = w_evalt(ctx, this, &args.ptr[0]);
	if(ctx->status->tag != W_STATUS_OK)
		return (w_value_t){};
	w_value_t b = w_evalt(ctx, this, &args.ptr[1]);
	if(ctx->status->tag != W_STATUS_OK) {
		w_value_release(&a);
		return (w_value_t){};
	}
	bool ret = w_value_equal(&a, &b);
	w_value_release(&a);
	w_value_release(&b);
	return (w_value_t){.type = W_VALUE_INT, .int_ = ret ? 1 : 0};
}

W_COMMAND(w_cmd_neq) {
	ARGS_EQUAL("!=", 2);
	w_value_t a = w_evalt(ctx, this, &args.ptr[0]);
	if(ctx->status->tag != W_STATUS_OK)
		return (w_value_t){};
	w_value_t b = w_evalt(ctx, this, &args.ptr[1]);
	if(ctx->status->tag != W_STATUS_OK) {
		w_value_release(&a);
		return (w_value_t){};
	}
	bool ret = !w_value_equal(&a, &b);
	w_value_release(&a);
	w_value_release(&b);
	return (w_value_t){.type = W_VALUE_INT, .int_ = ret ? 1 : 0};
}


// can't use this for the earlier ones because lt, lte, gt, and gte all use ctx, while equals doesn't. this means a lot of code repition which is bad and I should fix it.
#define BOOL_CMD(OP, NAME) W_COMMAND(w_cmd_##OP) { \
	ARGS_EQUAL(#NAME, 2); \
	w_value_t a = w_evalt(ctx, this, &args.ptr[0]); \
	if(ctx->status->tag != W_STATUS_OK) \
		return (w_value_t){}; \
	w_value_t b = w_evalt(ctx, this, &args.ptr[1]); \
	if(ctx->status->tag != W_STATUS_OK) { \
		w_value_release(&a); \
		return (w_value_t){}; \
	} \
	bool ret = w_value_##OP(ctx, &a, &b); \
	w_value_release(&a); \
	w_value_release(&b); \
	return (w_value_t){.type = W_VALUE_INT, .int_ = ret ? 1 : 0}; \
}

BOOL_CMD(lt, <);
BOOL_CMD(lte, <=);
BOOL_CMD(gt, >);
BOOL_CMD(gte, >=);

#undef BOOL_CMD

// control

W_COMMAND(w_cmd_if) {
	ARGS_GTE("if", 2);
	// number of conditions
	size_t conds = args.len/2;
	for(size_t i = 0; i < conds; i++) {
		w_value_t cond = w_evalt(ctx, this, &args.ptr[i*2]);
		if(ctx->status->tag != W_STATUS_OK)
			return (w_value_t){};
		bool t = w_value_truthy(&cond);
		w_value_release(&cond);
		if(t)
			return w_evalt(ctx, this, &args.ptr[i*2+1]); // if there's an error it'll get caught by the next function up anyways
	}
	// execute else condition if there is one
	if(conds*2 != args.len)
		return w_evalt(ctx, this, &args.ptr[args.len-1]); // again, no need to catch error here
	return (w_value_t){.type = W_VALUE_NULL};
}

W_COMMAND(w_cmd_break) {
	ARGS_NONE("break");
	w_status_break(ctx->status);
	return (w_value_t){};
}

W_COMMAND(w_cmd_continue) {
	ARGS_NONE("continue");
	w_status_continue(ctx->status);
	return (w_value_t){};
}

W_COMMAND(w_cmd_return) {
	ARGS_BETWEEN("return", 0, 1);
	w_value_t val;
	if(args.len == 1) {
		val = w_evalt(ctx, this, &args.ptr[0]);
		if(ctx->status->tag != W_STATUS_OK)
			return (w_value_t){};
	}
	else
		val = (w_value_t){.type = W_VALUE_NULL};
	w_status_return(ctx->status, &val);
	w_value_release(&val);
	return (w_value_t){};
}

W_COMMAND(w_cmd_while) {
	ARGS_EQUAL("while", 2);
	w_ast_t *cond = &args.ptr[0];
	w_ast_t *body = &args.ptr[1];
	w_value_t vbody = (w_value_t){.type = W_VALUE_NULL};
	while(true) {
		w_value_release(&vbody);
		w_value_t vcond = w_evalt(ctx, this, cond);
		if(ctx->status->tag != W_STATUS_OK)
			return (w_value_t){};
		if(!w_value_truthy(&vcond)) {
			w_value_release(&vcond);
			break;
		}
		w_value_release(&vcond);
		vbody = w_evalt(ctx, this, body);
		switch(ctx->status->tag) {
			case W_STATUS_OK:
				break;
			case W_STATUS_BREAK:
				w_status_ok(ctx->status);
				goto done;
			case W_STATUS_CONTINUE:
				w_status_ok(ctx->status);
				goto cont;
			default:
				w_value_release(&vbody);
				return (w_value_t){};
		}
		cont:;
	}
	done:
	return vbody;
}

W_COMMAND(w_cmd_do) {
	ARGS_EQUAL("do", 1);
	return w_evalt(ctx, this, &args.ptr[0]);
}

W_COMMAND(w_cmd_for) {
	ARGS_BETWEEN("for", 2, 4);
	w_value_t coll = w_evalt(ctx, this, &args.ptr[args.len-2]);
	if(ctx->status->tag != W_STATUS_OK)
		return (w_value_t){};
	w_astring_t *idx = NULL, *elem = NULL;
	#define GET_VAR(NAME, IDX) \
		if(args.ptr[IDX].type != W_AST_VAR) { \
			w_status_err(ctx->status, w_error_new(args.ptr[IDX].pos, "Argument " #IDX " must be a variable.")); \
			return (w_value_t){}; \
		} \
		NAME = &args.ptr[IDX].string;
	switch(args.len) {
		case 3:
			GET_VAR(elem, 0);
			break;
		case 4:
			GET_VAR(idx, 0);
			GET_VAR(elem, 1);
			break;
	}
	#undef GET_VAR
	w_ast_t *body = &args.ptr[args.len-1];
	w_value_t v = (w_value_t){.type = W_VALUE_NULL};
	if(coll.type == W_VALUE_LIST) {
		w_list_t *l = coll.list;
		for(size_t i = 0; i < l->len; i++) {
			w_ctx_t sub = w_ctx_clone(ctx); // this creates a context for every iteration, which is probably suboptimal. however, in order to just have one, I'd need a way
																			// of tracking which variables aren't created by the command in order to delete them after every iteration. so I'll keep this for now.
			w_value_release(&v);
			if(elem != NULL) {
				w_value_t item = l->ptr[i];
				w_value_ref(&item);
				w_ctx_let(&sub, elem, item);
				if(idx != NULL)
					w_ctx_let(&sub, idx, (w_value_t){.type = W_VALUE_INT, .int_ = i});
			}
			v = w_evalst(ctx, &sub, this, body);
			switch(ctx->status->tag) {
				case W_STATUS_OK:
					break;
				case W_STATUS_BREAK:
					w_ctx_free(&sub);
					w_status_ok(ctx->status);
					goto list_done;
				case W_STATUS_CONTINUE:
					w_status_ok(ctx->status);
					goto list_cont;
				default:
					w_value_release(&coll);
					w_ctx_free(&sub);
					return (w_value_t){};
			}
			list_cont:
			w_ctx_free(&sub);
		}
		list_done:
		w_value_release(&coll);
		return v;
	}
	if(coll.type == W_VALUE_MAP) {
		w_map_t *map = coll.map;
		for(size_t i = 0; i < map->capacity; i++) {
			w_map_list_t *curr = map->ptr[i];
			while(curr != NULL) {
				w_ctx_t sub = w_ctx_clone(ctx); 
				w_value_release(&v);
				if(elem != NULL) {
					w_value_ref(&curr->item);
					w_ctx_let(&sub, elem, curr->item);
					if(idx != NULL) {
						w_astring_t s = w_astrdup(&curr->key);
						w_string_t *str = malloc(sizeof(w_string_t));
						*str = (w_string_t){1, s.len, s.ptr};
						w_ctx_let(&sub, idx, (w_value_t){.type = W_VALUE_STRING, .string = str});
					}
				}
				v = w_evalst(ctx, &sub, this, body);
				switch(ctx->status->tag) {
					case W_STATUS_OK:
						break;
					case W_STATUS_BREAK:
						w_ctx_free(&sub);
						w_status_ok(ctx->status);
						goto map_done;
					case W_STATUS_CONTINUE:
						w_status_ok(ctx->status);
						goto map_cont;
					default:
						w_value_release(&coll);
						w_ctx_free(&sub);
						return (w_value_t){};
				}
				map_cont:
				w_ctx_free(&sub);
				curr = curr->next;
			}
		}
		map_done:
		w_value_release(&coll);
		return v;
	}
	w_status_err(ctx->status, w_error_new(args.ptr[1].pos, "%s is not iterable.", w_typename(coll.type)));
	w_value_release(&coll);
	return (w_value_t){};
}

// structures

W_COMMAND(w_cmd_list) {
	w_list_t *l = malloc(sizeof(w_list_t));
	*l = (w_list_t){1, args.len, malloc(sizeof(w_value_t)*args.len)};
	for(size_t i = 0; i < args.len; i++) {
		w_value_t v = w_evalt(ctx, this, &args.ptr[i]);
		if(ctx->status->tag != W_STATUS_OK) {
			for(size_t j = 0; j < i; j++)
				w_value_release(&l->ptr[j]);
			free(l->ptr);
			return (w_value_t){};
		}
		l->ptr[i] = v;
	}
	return (w_value_t){.type = W_VALUE_LIST, .list = l};
}


// gets an integer
#define GET_INT(RET, IDX) { \
	w_value_t x = w_evalt(ctx, this, &args.ptr[IDX]); \
	if(ctx->status->tag != W_STATUS_OK) \
		return (w_value_t){}; \
	if(x.type != W_VALUE_INT) { \
		w_status_err(ctx->status, w_error_new(args.ptr[0].pos, "Expected int, got %s.", w_typename(x.type))); \
		w_value_release(&x); \
		return (w_value_t){}; \
	} \
	RET = x.int_; \
}

W_COMMAND(w_cmd_range) {
	ARGS_BETWEEN("range", 1, 2);
	int64_t min, max;
		
	if(args.len == 1) {
		min = 0;
		GET_INT(max, 0);
	}
	else {
		GET_INT(min, 0);
		GET_INT(max, 1);
	}
	int64_t dif = max-min;
	if(dif == 0) {
		// empty list
		w_list_t *l = malloc(sizeof(w_list_t));
		*l = (w_list_t){1, 0, NULL};
		return (w_value_t){.type = W_VALUE_LIST, .list = l};
	}
	if(dif < 0)
		dif = -dif;
	w_list_t *l = malloc(sizeof(w_list_t));
	*l = (w_list_t){1, dif, malloc(sizeof(w_value_t)*dif)};
	if(max > min) {
		for(int64_t i = 0; i < dif; i++)
			l->ptr[i] = (w_value_t){.type = W_VALUE_INT, .int_ = min+i};
	}
	else {
		for(int64_t i = 0; i < dif; i++)
			l->ptr[i] = (w_value_t){.type = W_VALUE_INT, .int_ = min-i-1};
	}
	return (w_value_t){.type = W_VALUE_LIST, .list = l};
}

W_COMMAND(w_cmd_map) {
	if(args.len%2 != 0) {
		w_status_err(ctx->status, w_error_new(pos, "map must have an even amount of arguments."));
		return (w_value_t){};
	}
	w_map_t *map = malloc(sizeof(w_map_t));
	*map = w_map_new(128, 1);
	w_value_t vmap = (w_value_t){.type = W_VALUE_MAP, .map = map};
	for(size_t i = 0; i < args.len; i += 2) {
		w_value_t key;
		{
			w_value_t vkey = w_evalt(ctx, this, &args.ptr[i]);
			if(ctx->status->tag != W_STATUS_OK) {
				w_map_free(map);
				free(map);
				return (w_value_t){};
			}
			if(vkey.type != W_VALUE_STRING) {
				key = w_value_tostring(&vkey);
				w_value_release(&vkey);
			}
			else
				key = vkey;
		}
		w_value_t value = w_evalt(ctx, this, &args.ptr[i+1]);
		if(ctx->status->tag != W_STATUS_OK) {
			w_map_free(map);
			free(map);
			w_value_release(&key);
			return (w_value_t){};
		}
		// set $this pointer if value is a command and doesn't already have $this set
		if(value.type == W_VALUE_COMMAND) {
			// modify in-place if this is the only reference, otherwise clone and modify
			if(value.cmd->refcount > 1) {
				// releasing and then using here is fine - it won't be freed since refcount > 1
				w_value_release(&value);
				value = w_value_clone(&value);
			}
			value.cmd->this = malloc(sizeof(w_value_t));
			*value.cmd->this = vmap;
		}
		w_astring_t str = (w_astring_t){key.string->len, key.string->ptr};
		w_map_set(map, &str, value);
		w_value_release(&key);
	}
	return vmap;
}

W_COMMAND(w_cmd_refcount) {
	ARGS_EQUAL("refcount", 1);
	w_value_t v = w_evalt(ctx, this, &args.ptr[0]);
	int64_t refcount = 0;
	switch(v.type) {
		case W_VALUE_STRING:
			refcount = v.string->refcount;
			break;
		case W_VALUE_LIST:
			refcount = v.list->refcount;
			break;
		case W_VALUE_MAP:
			refcount = v.map->data;
			break;
		case W_VALUE_EXTERNCMD:
			refcount = v.externcmd->refcount;
			break;
		case W_VALUE_COMMAND:
			refcount = v.cmd->refcount;
			break;
	}
	w_value_release(&v);
	// -1 on refcount since we're creating a new reference by evalling a thing that returns the object
	return (w_value_t){.type = W_VALUE_INT, .int_ = refcount-1};
}

W_COMMAND(w_cmd_list_set_mut) {
	ARGS_EQUAL("list:set", 2);
	w_list_t *l = obj->list;
	w_value_t vidx = w_evalt(ctx, this, &args.ptr[0]);
	if(ctx->status->tag != W_STATUS_OK)
		return (w_value_t){};
	if(vidx.type != W_VALUE_INT) {
		w_value_release(&vidx);
		w_status_err(ctx->status, w_error_new(args.ptr[0].pos, "Index must be an int."));
		return (w_value_t){};
	}
	int64_t idx = vidx.int_;
	if(idx < 0 || idx >= l->len) {
		w_status_err(ctx->status, w_error_new(args.ptr[0].pos, "Index %" PRId64 " out of bounds for array of length %zu.", idx, l->len));
		return (w_value_t){};
	}
	w_value_t val = w_evalt(ctx, this, &args.ptr[1]);
	if(ctx->status->tag != W_STATUS_OK)
		return (w_value_t){};
	w_value_release(&l->ptr[idx]);
	l->ptr[idx] = val;
	w_value_ref(obj);
	return *obj;
}

// macro to create a non-mutating version of another command
#define UNMUT(NAME, MUT, REFCOUNT) \
	W_COMMAND(NAME) { \
		if(obj->REFCOUNT == 1) /* if there's only one reference, we can safely do the operation in-place */ \
			return MUT(pos, ctx, this, obj, args); \
		w_value_t new = w_value_clone(obj); \
		w_value_t v = MUT(pos, ctx, this, &new, args); \
		if(ctx->status->tag != W_STATUS_OK) { \
			w_value_release(&new); \
			return (w_value_t){}; \
		} \
		w_value_release(&v); \
		return new; \
	}

UNMUT(w_cmd_list_set, w_cmd_list_set_mut, list->refcount);

W_COMMAND(w_cmd_list_push_mut) {
	ARGS_GTE("list:push", 1);
	w_list_t *l = obj->list;
	size_t prevlen = l->len;
	l->len += args.len;
	l->ptr = realloc(l->ptr, sizeof(w_value_t)*l->len);
	for(size_t i = 0; i < args.len; i++) {
		w_value_t v = w_evalt(ctx, this, &args.ptr[i]);
		if(ctx->status->tag != W_STATUS_OK) {
			for(size_t j = 0; j < i; j++)
				w_value_release(&l->ptr[prevlen+j]);
			l->len = prevlen;
			if(l->len == 0)
				free(l->ptr);
			else
				l->ptr = realloc(l->ptr, sizeof(w_value_t)*prevlen);
			return (w_value_t){};
		}
		l->ptr[prevlen+i] = v;
	}
	w_value_ref(obj);
	return *obj;
}

UNMUT(w_cmd_list_push, w_cmd_list_push_mut, list->refcount);

// maybe I should make a macro to generalize this and list:push
// TODO

W_COMMAND(w_cmd_list_unshift_mut) {
	ARGS_GTE("list:unshift", 1);
	w_list_t *l = obj->list;
	size_t prevlen = l->len;
	l->len += args.len;
	l->ptr = realloc(l->ptr, sizeof(w_value_t)*l->len);
	memmove(l->ptr+args.len, l->ptr, sizeof(w_value_t)*prevlen);
	for(size_t i = 0; i < args.len; i++) {
		w_value_t v = w_evalt(ctx, this, &args.ptr[i]);
		if(ctx->status->tag != W_STATUS_OK) {
			for(size_t j = 0; j < i; j++)
				w_value_release(&l->ptr[i]);
			memmove(l->ptr, l->ptr+args.len, sizeof(w_value_t)*prevlen);
			l->len = prevlen;
			if(l->len == 0)
				free(l->ptr);
			else
				l->ptr = realloc(l->ptr, sizeof(w_value_t)*prevlen);
			return (w_value_t){};
		}
		l->ptr[i] = v;
	}
	w_value_ref(obj);
	return *obj;
}

UNMUT(w_cmd_list_unshift, w_cmd_list_unshift_mut, list->refcount);

W_COMMAND(w_cmd_list_pop_mut) {
	ARGS_NONE("list:pop");
	w_list_t *l = obj->list;
	w_value_t v = l->ptr[l->len-1];
	l->ptr = realloc(l->ptr, sizeof(w_value_t)*(--l->len));
	return v;
}

UNMUT(w_cmd_list_pop, w_cmd_list_pop_mut, list->refcount);

W_COMMAND(w_cmd_list_shift_mut) {
	ARGS_NONE("list:shift");
	w_list_t *l = obj->list;
	w_value_t v = l->ptr[0];
	memmove(l->ptr, l->ptr+1, sizeof(w_value_t)*(--l->len));
	l->ptr = realloc(l->ptr, sizeof(w_value_t)*l->len);
	return v;
}

UNMUT(w_cmd_list_shift, w_cmd_list_shift_mut, list->refcount);

W_COMMAND(w_cmd_list_slice_mut) {
	ARGS_EQUAL("list:slice", 2);
	uint64_t start, end;
	GET_INT(start, 0);
	GET_INT(end, 1);
	w_list_t *l = obj->list;
	// checks
	if(start < 0 || start >= l->len) {
		w_status_err(ctx->status, w_error_new(pos, "slice start %" PRId64 " is out of range for list of length %zu.", start, l->len));
		return (w_value_t){};
	}
	if(end < start) {
		w_status_err(ctx->status, w_error_new(pos, "slice end %" PRId64 " is less than slice start %" PRId64 ".", end, start));
		return (w_value_t){};
	}
	if(end >= l->len) {
		w_status_err(ctx->status, w_error_new(pos, "slice end %" PRId64 " is out of range for list of length %zu.", end, l->len));
		return (w_value_t){};
	}
	// release everything outside of range first
	for(size_t i = 0; i < start; i++)
		w_value_release(&l->ptr[i]);
	for(size_t i = end; i < l->len; i++)
		w_value_release(&l->ptr[i]);
	// then do array slice
	l->len = end-start;
	memmove(l->ptr, &l->ptr[start], sizeof(w_value_t)*l->len);
	l->ptr = realloc(l->ptr, sizeof(w_value_t)*l->len);
	w_value_ref(obj);
	return *obj;
}

UNMUT(w_cmd_list_slice, w_cmd_list_slice_mut, list->refcount);

W_COMMAND(w_cmd_list_cat_mut) {
	ARGS_GTE("list:cat", 1);
	w_list_t *l = obj->list;
	for(size_t i = 0; i < args.len; i++) {
		w_value_t v = w_evalt(ctx, this, &args.ptr[i]);
		if(v.type != W_VALUE_LIST) {
			w_value_release(&v);
			w_status_err(ctx->status, w_error_new(pos, "list expected, got %s.", w_typename(v.type)));
			return (w_value_t){};
		}
		w_list_t *list = v.list;
		for(size_t i = 0; i < list->len; i++)
			w_value_ref(&list->ptr[i]);
		size_t prevlen = l->len;
		l->len += list->len;
		l->ptr = realloc(l->ptr, sizeof(w_value_t)*l->len);
		memcpy(&l->ptr[prevlen], list->ptr, sizeof(w_value_t)*list->len);
		w_value_release(&v);
	}
	w_value_ref(obj);
	return *obj;
}

UNMUT(w_cmd_list_cat, w_cmd_list_cat_mut, list->refcount);

W_COMMAND(w_cmd_list_fill_mut) {
	ARGS_EQUAL("list:fill", 1);
	w_value_t v = w_evalt(ctx, this, &args.ptr[0]);
	if(ctx->status->tag != W_STATUS_OK)
		return (w_value_t){};
	w_list_t *l = obj->list;
	for(size_t i = 0; i < l->len; i++) {
		w_value_release(&l->ptr[i]);
		w_value_t clone = w_value_clone(&v);
		l->ptr[i] = clone;
	}
	w_value_release(&v);
	w_value_ref(obj);
	return *obj;
}

UNMUT(w_cmd_list_fill, w_cmd_list_fill_mut, map->data);

W_COMMAND(w_cmd_list_dup_mut) {
	ARGS_EQUAL("list:dup", 1);
	int64_t amt;
	GET_INT(amt, 0);
	if(amt < 0) {
		w_status_err(ctx->status, w_error_new(pos, "Amount of duplications must be positive."));
		return (w_value_t){};
	}
	w_list_t *l = obj->list;
	if(amt == 0) {
		l->len = 0;
		free(l->ptr);
		l->ptr = NULL;
		goto ret;
	}
	if(amt == 1)
		goto ret;
	size_t len = l->len, newlen = len*amt;
	l->ptr = realloc(l->ptr, newlen*sizeof(w_value_t));
	for(size_t i = len; i < newlen; i += len) {
		memcpy(&l->ptr[i], l->ptr, len*sizeof(w_value_t));
		for(size_t i = 0; i < len; i++)
			w_value_ref(&l->ptr[i]);
	}
	l->len *= amt;
	ret:
	w_value_ref(obj);
	return *obj;
}

UNMUT(w_cmd_list_dup, w_cmd_list_dup_mut, list->refcount);

W_COMMAND(w_cmd_list_reverse_mut) {
	ARGS_EQUAL("list:reverse", 0);
	w_list_t *l = obj->list;
	for(size_t i = 0; i < l->len/2; i++) {
		w_value_t tmp = l->ptr[i];
		l->ptr[i] = l->ptr[l->len-i-1];
		l->ptr[l->len-i-1] = tmp;
	}
	w_value_ref(obj);
	return *obj;
}

UNMUT(w_cmd_list_reverse, w_cmd_list_reverse_mut, list->refcount);

W_COMMAND(w_cmd_map_set_mut) {
	ARGS_EQUAL("map:set", 2);
	w_map_t *map = obj->map;
	w_value_t key;
	{
		w_value_t vkey = w_evalt(ctx, this, &args.ptr[0]);
		if(ctx->status->tag != W_STATUS_OK)
			return (w_value_t){};
		if(vkey.type != W_VALUE_STRING) {
			key = w_value_tostring(&vkey);
			w_value_release(&vkey);
		}
		key = vkey;
	}
	w_value_t value = w_evalt(ctx, this, &args.ptr[1]);
	if(ctx->status->tag != W_STATUS_OK) {
		w_value_release(&key);
		return (w_value_t){};
	}
	w_astring_t astr = (w_astring_t){key.string->len, key.string->ptr};
	w_map_set(map, &astr, value);
	w_value_ref(obj);
	return *obj;
}

UNMUT(w_cmd_map_set, w_cmd_map_set_mut, map->data);

W_COMMAND(w_cmd_map_del_mut) {
	ARGS_GTE("map:del", 1);
	w_map_t *map = obj->map;
	for(size_t i = 0; i < args.len; i++) {
		w_value_t v = w_evalt(ctx, this, &args.ptr[i]);
		if(ctx->status->tag != W_STATUS_OK)
			return (w_value_t){};
		if(v.type != W_VALUE_STRING) {
			w_value_t v2 = w_value_tostring(&v);
			w_value_release(&v);
			v = v2;
		}
		w_astring_t astr = (w_astring_t){v.string->len, v.string->ptr};
		w_map_del(map, &astr);
		w_value_release(&v);
	}
	w_value_ref(obj);
	return *obj;
}

UNMUT(w_cmd_map_del, w_cmd_map_del_mut, map->data);

W_COMMAND(w_cmd_new_list) {
	ARGS_EQUAL("new-list", 1);
	int64_t len;
	GET_INT(len, 0);
	if(len < 0) {
		w_status_err(ctx->status, w_error_new(args.ptr[0].pos, "List length must be positive."));
		return (w_value_t){};
	}
	w_value_t *ptr = malloc(sizeof(w_value_t)*len);
	for(size_t i = 0; i < len; i++)
		ptr[i].type = W_VALUE_NULL;
	w_list_t *l = malloc(sizeof(w_list_t));
	*l = (w_list_t){1, len, ptr};
	return (w_value_t){.type = W_VALUE_LIST, .list = l};
}

W_COMMAND(w_cmd_string_slice_mut) {
	ARGS_EQUAL("string:slice", 2);
	int64_t start, end;
	GET_INT(start, 0);
	GET_INT(end, 1);
	w_string_t *s = obj->string;
	if(start < 0 || start >= s->len) {
		w_status_err(ctx->status, w_error_new(pos, "slice start %" PRId64 " is out of range for string of length %zu.", start, s->len));
		return (w_value_t){};
	}
	if(end < start) {
		w_status_err(ctx->status, w_error_new(pos, "slice end %" PRId64 " is less than slice start %" PRId64 ".", end, start));
		return (w_value_t){};
	}
	if(end >= s->len) {
		w_status_err(ctx->status, w_error_new(pos, "slice end %" PRId64 " is out of range for string of length %zu.", end, s->len));
		return (w_value_t){};
	}
	size_t len = end-start;
	s->len = len;
	if(len == 0) {
		free(s->ptr);
		s->ptr = NULL;
		w_value_ref(obj);
		return *obj;
	}
	memcpy(s->ptr, &s->ptr[start], len);
	w_value_ref(obj);
	return *obj;
}

UNMUT(w_cmd_string_slice, w_cmd_string_slice_mut, string->refcount);

W_COMMAND(w_cmd_string_set_mut) {
	ARGS_EQUAL("string:set", 2);
	int64_t idx;
	GET_INT(idx, 0);
	w_string_t *s = obj->string;
	if(idx < 0 || idx >= s->len) {
		w_status_err(ctx->status, w_error_new(pos, "Index %" PRId64 " is out of range for string of length %zu.", idx, s->len));
		return (w_value_t){};
	}
	w_value_t v = w_evalt(ctx, this, &args.ptr[1]);
	switch(v.type) {
		case W_VALUE_FLOAT:
			s->ptr[idx] = (char)v.float_;
			break;
		case W_VALUE_INT:
			s->ptr[idx] = v.int_;
			break;
		case W_VALUE_STRING: {
			w_string_t *str = v.string;
			if(str->len != 1) {
				w_status_err(ctx->status, w_error_new(pos, "Value string must be of length 1."));
				return (w_value_t){};
			}
			s->ptr[idx] = str->ptr[0];
			break;
		}
		default:
			w_value_release(&v);
			w_status_err(ctx->status, w_error_new(pos, "string:set takes 2 arguments"));
			return (w_value_t){};
	}
	w_value_ref(obj);
	return *obj;
}

UNMUT(w_cmd_string_set, w_cmd_string_set_mut, string->refcount);

W_COMMAND(w_cmd_string_dup_mut) {
	ARGS_EQUAL("string:dup", 1);
	int64_t amt;
	GET_INT(amt, 0);
	if(amt < 0) {
		w_status_err(ctx->status, w_error_new(pos, "Amount of duplications must be positive."));
		return (w_value_t){};
	}
	w_string_t *str = obj->string;
	if(amt == 0) {
		str->len = 0;
		free(str->ptr);
		str->ptr = NULL;
		goto ret;
	}
	if(amt == 1)
		goto ret;
	size_t len = str->len, newlen = len*amt;
	str->ptr = realloc(str->ptr, newlen);
	for(size_t i = len; i < newlen; i += len) {
		memcpy(str->ptr+i, str->ptr, len);
	}
	str->len *= amt;
	ret:
	w_value_ref(obj);
	return *obj;
}

UNMUT(w_cmd_string_dup, w_cmd_string_dup_mut, string->refcount);

W_COMMAND(w_cmd_string_reverse_mut) {
	ARGS_EQUAL("string:reverse", 0);
	w_string_t *str = obj->string;
	for(size_t i = 0; i < str->len/2; i++) {
		char tmp = str->ptr[i];
		str->ptr[i] = str->ptr[str->len-i-1];
		str->ptr[str->len-i-1] = tmp;
	}
	w_value_ref(obj);
	return *obj;
}

UNMUT(w_cmd_string_reverse, w_cmd_string_reverse_mut, string->refcount);

W_COMMAND(w_cmd_string_cat_mut) {
	ARGS_GTE("string:cat", 1);
	w_string_t *str = obj->string;
	for(size_t i = 0; i < args.len; i++) {
		w_value_t v = w_evalt(ctx, this, &args.ptr[i]);
		if(v.type != W_VALUE_STRING) {
			w_value_t vs = w_value_tostring(&v);
			w_value_release(&v);
			v = vs;
		}
		w_string_t *s = v.string;
		str->ptr = realloc(str->ptr, str->len+s->len);
		memcpy(str->ptr+str->len, s->ptr, s->len);
		str->len += s->len;
		w_value_release(&v);
	}
	w_value_ref(obj);
	return *obj;
}

UNMUT(w_cmd_string_cat, w_cmd_string_cat_mut, string->refcount);

W_COMMAND(w_cmd_string_split) {
	ARGS_EQUAL("string:split", 1);
	w_value_t vby = w_evalt(ctx, this, &args.ptr[0]);
	if(vby.type != W_VALUE_STRING) {
		w_status_err(ctx->status, w_error_new(pos, "Expected string, got %s.", w_typename(vby.type)));
		return (w_value_t){};
	}
	w_string_t *by = vby.string;
	w_string_t *s = obj->string;
	w_list_t *ret = malloc(sizeof(w_list_t));
	*ret = (w_list_t){1, 0, NULL};
	if(by->len > s->len) {
		ret->ptr = malloc(sizeof(w_value_t));
		ret->ptr[0] = w_value_clone(obj);
		w_value_release(&vby);
		return (w_value_t){.type = W_VALUE_LIST, .list = ret};
	}
	size_t start = 0;
	#define ADD(END) do { \
		w_string_t *slice = malloc(sizeof(w_string_t)); \
		size_t len = END-start; \
		if(len == 0) \
			*slice = (w_string_t){1, 0, NULL}; \
		else \
			*slice = (w_string_t){1, len, malloc(len)}; \
		memcpy(slice->ptr, &s->ptr[start], len); \
		ret->ptr = realloc(ret->ptr, sizeof(w_value_t)*(++ret->len)); \
		ret->ptr[ret->len-1] = (w_value_t){.type = W_VALUE_STRING, .string = slice}; \
	} while(0)
	for(size_t i = 0; i < s->len-by->len+1; i++) {
		if(memcmp(&s->ptr[i], by->ptr, by->len) == 0) {
			ADD(i);
			start = i += by->len;
		}
	}
	ADD(s->len);
	w_value_release(&vby);
	return (w_value_t){.type = W_VALUE_LIST, .list = ret};
}

W_COMMAND(w_cmd_clone) {
	ARGS_NONE("clone");
	return w_value_clone(obj);
}

W_COMMAND(w_cmd_cmd) {
	ARGS_GTE("cmd", 1);
	size_t argc = args.len-1;
	w_cmd_arg_t *argv = malloc(sizeof(w_cmd_arg_t)*argc);
	for(size_t i = 0; i < argc; i++) {
		if(args.ptr[i].type != W_AST_VAR) {
			w_status_err(ctx->status, w_error_new(args.ptr[i].pos, "cmd takes a list of vars, and then an expression."));
			free(argv);
			return (w_value_t){};
		}
		argv[i] = (w_cmd_arg_t){w_astrdup(&args.ptr[i].string)};
	}
	w_cmd_t *cmd = malloc(sizeof(w_cmd_t));
	*cmd = (w_cmd_t){1, argc, argv, w_ast_dup(&args.ptr[args.len-1]), NULL};
	return (w_value_t){.type = W_VALUE_COMMAND, .cmd = cmd};
}
