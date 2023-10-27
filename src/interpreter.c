#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <inttypes.h>
#include <math.h>

#include "commands.h"
#include "interpreter.h"

char *w_typename(w_value_type_t type) {
	switch(type) {
		case W_VALUE_NULL:
			return "null";
		case W_VALUE_INT:
			return "int";
		case W_VALUE_FLOAT:
			return "float";
		case W_VALUE_EXTERNCMD:
			return "externcommand";
		case W_VALUE_COMMAND:
			return "command";
		case W_VALUE_STRING:
			return "string";
		case W_VALUE_LIST:
			return "list";
		case W_VALUE_MAP:
			return "map";
	}
}

void w_value_release(w_value_t *val) {
	switch(val->type) {
		case W_VALUE_STRING:
			if(--val->string->refcount == 0) {
				free(val->string->ptr);
				free(val->string);
			}
			break;
		case W_VALUE_LIST: {
			w_list_t *l = val->list;
			if(--l->refcount == 0) {
				for(size_t i = 0; i < l->len; i++)
					w_value_release(&l->ptr[i]);
				free(l->ptr);
				free(l);
			}
			break;
		}
		case W_VALUE_MAP: {
			w_map_t *m = val->map;
			if(--m->data == 0) {
				w_map_free(m);
				free(m);
			}
			break;
		}
		case W_VALUE_EXTERNCMD: {
			w_ecmd_t *c = val->externcmd;
			if(--c->refcount == 0) {
				if(c->obj != NULL)
					w_value_release(c->obj);
				free(c->obj);
				free(c);
			}
			break;
		}
		case W_VALUE_COMMAND: {
			w_cmd_t *c = val->cmd;
			if(--c->refcount == 0) {
				// note: c->this is not released, since internal command's $this pointers are always weak references.
				// this does lead to the possibility of a segfault, however.
				free(c->this);
				for(size_t i = 0; i < c->argc; i++)
					free(c->args[i].name.ptr);
				free(c->args);
				w_ast_free(&c->impl);
				free(c);
			}
		}
	}
}

void w_value_ref(w_value_t *val) {
	switch(val->type) {
		case W_VALUE_STRING:
			val->string->refcount++;
			break;
		case W_VALUE_LIST:
			val->list->refcount++;
			break;
		case W_VALUE_MAP:
			val->map->data++;
			break;
		case W_VALUE_EXTERNCMD:
			val->externcmd->refcount++;
			break;
		case W_VALUE_COMMAND:
			val->cmd->refcount++;
			break;
	}
}

// actual implementation of tostring, using a w_writer
static void value_tostring(bool toplevel, w_writer_t *w, w_value_t *val) {
	switch(val->type) {
		case W_VALUE_NULL:
			w_writer_putcs(w, "null");
			break;
		case W_VALUE_FLOAT: {
			char buf[256];
			snprintf(buf, 256, "%f", val->float_);
			w_writer_putcs(w, buf);
			break;
		}
		case W_VALUE_INT: {
			char buf[256];
			snprintf(buf, 256, "%" PRId64, val->int_);
			w_writer_putcs(w, buf);
			break;
		}
		case W_VALUE_EXTERNCMD: {
			char buf[256];
			snprintf(buf, 256, "<externcmd @ %p>", val->externcmd->cmd);
			w_writer_putcs(w, buf);
			break;
		}
		case W_VALUE_STRING:
			if(!toplevel)
				w_writer_putch(w, '"');
			w_writer_puts(w, val->string->len, val->string->ptr);
			if(!toplevel)
				w_writer_putch(w, '"');
			break;
		case W_VALUE_LIST:
			w_writer_putcs(w, "[list");
			for(size_t i = 0; i < val->list->len; i++) {
				w_writer_putch(w, ' ');
				value_tostring(false, w, &val->list->ptr[i]);
			}
			w_writer_putch(w, ']');
			break;
		case W_VALUE_MAP: {
			w_map_t *map = val->map;
			w_writer_putcs(w, "[map");
			for(size_t i = 0; i < map->capacity; i++) {
				w_map_list_t *curr = map->ptr[i];
				while(curr != NULL) {
					w_writer_putch(w, ' ');
					w_writer_puts(w, curr->key.len, curr->key.ptr);
					w_writer_putch(w, ' ');
					value_tostring(false, w, &curr->item);
					curr = curr->next;
				}
			}
			w_writer_putch(w, ']');
			break;
		}
		case W_VALUE_COMMAND: {
			w_cmd_t *cmd = val->cmd;
			w_writer_putcs(w, "[cmd");
			for(size_t i = 0; i < cmd->argc; i++) {
				w_writer_putch(w, ' ');
				w_astring_t *s = &cmd->args[i].name;
				w_writer_putch(w, '$');
				w_writer_puts(w, s->len, s->ptr);
			}
			w_writer_putcs(w, " ...");
			w_writer_putch(w, ']');
			break;
		}
	}
}

w_value_t w_value_tostring(w_value_t *val) {
	w_writer_t writer = w_writer_new();
	value_tostring(true, &writer, val);
	w_writer_resize(&writer);
	w_string_t *str = malloc(sizeof(w_string_t));
	*str = (w_string_t){1, writer.len, writer.buf};
	return (w_value_t){
		.type = W_VALUE_STRING,
		.string = str
	};
}
//W_VALUE_NULL, // null value
//W_VALUE_INT, // 64-bit signed integer
//W_VALUE_FLOAT, // 64-bit floating point
//// refcounted
//W_VALUE_EXTERNCMD, // external command
//W_VALUE_COMMAND, // internal command
//W_VALUE_STRING,
//W_VALUE_LIST,
//W_VALUE_MAP

w_value_t w_value_toint(w_value_t *val) {
	switch(val->type) {
		case W_VALUE_INT:
		case W_VALUE_NULL:
			return *val;
		case W_VALUE_FLOAT:
			return (w_value_t){.type = W_VALUE_INT, .int_ = (int64_t)val->float_};
		case W_VALUE_STRING: {
			w_string_t *str = val->string;
			if(!w_is_int(str->ptr, str->len))
				return (w_value_t){.type = W_VALUE_NULL};
			return (w_value_t){.type = W_VALUE_INT, .int_ = w_parse_int(str->ptr, str->len)};
		}
		case W_VALUE_EXTERNCMD:
		case W_VALUE_COMMAND:
		case W_VALUE_LIST:
		case W_VALUE_MAP:
			return (w_value_t){.type = W_VALUE_NULL};
	}
}
w_value_t w_value_tofloat(w_value_t *val) {
	switch(val->type) {
		case W_VALUE_FLOAT:
		case W_VALUE_NULL:
			return *val;
		case W_VALUE_INT:
			return (w_value_t){.type = W_VALUE_FLOAT, .float_ = (double)val->int_};
		case W_VALUE_STRING: {
			w_string_t *str = val->string;
			if(!w_is_float(str->ptr, str->len))
				return (w_value_t){.type = W_VALUE_NULL};
			return (w_value_t){.type = W_VALUE_FLOAT, .float_ = w_parse_float(str->ptr, str->len)};
		}
		case W_VALUE_EXTERNCMD:
		case W_VALUE_COMMAND:
		case W_VALUE_LIST:
		case W_VALUE_MAP:
			return (w_value_t){.type = W_VALUE_NULL};
	}
}

void w_value_print(w_value_t *val, FILE *fp) {
	w_value_t v = w_value_tostring(val);
	w_string_t *str = v.string;
	for(size_t i = 0; i < str->len; i++)
		fputc(str->ptr[i], fp);
	w_value_release(&v);
}

// these only needs to exist because of stupid C dependency bullshit that I can't be bothered to deal with right now
void w_hack_set_value(w_value_t *a, w_value_t *b) {
	*a = *b;
}
w_value_t *w_hack_malloc_value(void) {
	return malloc(sizeof(w_value_t));
}

#define OPERATION(OP, FOP) { \
	if(a->type == W_VALUE_INT && b->type == W_VALUE_INT) \
		return (w_value_t){ .type = W_VALUE_INT, .int_ = a->int_ OP b->int_ }; \
	double x, y; \
	/* kinda ugly code reptition. maybe I could use another macro for it here but that's also ugly */ \
	if(a->type == W_VALUE_FLOAT) \
		x = a->float_; \
	else if(a->type == W_VALUE_INT) \
		x = a->int_; \
	else { \
		w_status_err(ctx->status, w_error_new((w_filepos_t){}, "Cannot perform operation " #OP " on types %s and %s.", w_typename(a->type), w_typename(b->type))); \
		return (w_value_t){}; \
	} \
	if(b->type == W_VALUE_FLOAT) \
		y = b->float_; \
	else if(b->type == W_VALUE_INT) \
		y = b->int_; \
	else { \
		w_status_err(ctx->status, w_error_new((w_filepos_t){}, "Cannot perform operation " #OP " on types %s and %s.", w_typename(a->type), w_typename(b->type))); \
		return (w_value_t){}; \
	} \
	/* FOP is a hack since % does not work on float (you need to use fmod) however I can get it to work with other operations by having other macros for them */ \
	return (w_value_t){ .type = W_VALUE_FLOAT, .float_ = FOP(x, y)}; \
}

// aforementioned macros
#define ADD(x, y) x+y
#define SUB(x, y) x-y
#define MUL(x, y) x*y
#define DIV(x, y) x/y

w_value_t w_value_add(w_ctx_t *ctx, w_value_t *a, w_value_t *b) OPERATION(+, ADD)
w_value_t w_value_sub(w_ctx_t *ctx, w_value_t *a, w_value_t *b) OPERATION(-, SUB)
w_value_t w_value_mul(w_ctx_t *ctx, w_value_t *a, w_value_t *b) OPERATION(*, MUL)
w_value_t w_value_div(w_ctx_t *ctx, w_value_t *a, w_value_t *b) OPERATION(/, DIV)
w_value_t w_value_mod(w_ctx_t *ctx, w_value_t *a, w_value_t *b) OPERATION(%, fmod)

#undef OPERATION
#undef ADD
#undef SUB
#undef MUL
#undef DIV

// boolean operations
// yeah, this is sort of a repeat of the other macro. I should probably make a more general macro that can handle both cases, but I'm lazy.
#define OPERATION(OP) { \
	if(a->type == W_VALUE_INT && b->type == W_VALUE_INT) \
		return a->int_ OP b->int_; \
	double x, y; \
	if(a->type == W_VALUE_FLOAT) \
		x = a->float_; \
	else if(a->type == W_VALUE_INT) \
		x = a->int_; \
	else { \
		w_status_err(ctx->status, w_error_new((w_filepos_t){}, "Cannot perform operation " #OP " on types %s and %s.", w_typename(a->type), w_typename(b->type))); \
		return false; \
	} \
	if(b->type == W_VALUE_FLOAT) \
		y = b->float_; \
	else if(b->type == W_VALUE_INT) \
		y = b->int_; \
	else { \
		w_status_err(ctx->status, w_error_new((w_filepos_t){}, "Cannot perform operation " #OP " on types %s and %s.", w_typename(a->type), w_typename(b->type))); \
		return false; \
	} \
	return x OP y; \
}

bool w_value_lt (w_ctx_t *ctx, w_value_t *a, w_value_t *b) OPERATION(<)
bool w_value_lte(w_ctx_t *ctx, w_value_t *a, w_value_t *b) OPERATION(<=)
bool w_value_gt (w_ctx_t *ctx, w_value_t *a, w_value_t *b) OPERATION(>)
bool w_value_gte(w_ctx_t *ctx, w_value_t *a, w_value_t *b) OPERATION(>=)

#undef OPERATION

// epsilon for float comparisons
#define EPSILON (0.00001)
#define FEQUAL(A, B) ((A-B) > -EPSILON && (A-B) < EPSILON)

bool w_value_equal(w_value_t *a, w_value_t *b) {
	switch(a->type) {
		case W_VALUE_NULL:
			return b->type == W_VALUE_NULL;
		case W_VALUE_INT:
			switch(b->type) {
				case W_VALUE_INT:
					return a->int_ == b->int_;
				case W_VALUE_FLOAT:
					return FEQUAL(a->int_, b->float_);
				default:
					return false;
			}
		case W_VALUE_FLOAT:
			switch(b->type) {
				case W_VALUE_INT:
					return FEQUAL(a->float_, b->int_);
				case W_VALUE_FLOAT:
					return FEQUAL(a->float_, b->float_);
				default:
					return false;
			}
		case W_VALUE_EXTERNCMD:
			if(b->type != W_VALUE_EXTERNCMD)
				return false;
			return a->externcmd == b->externcmd;
		case W_VALUE_COMMAND:
			if(b->type != W_VALUE_COMMAND)
				return false;
			return a->cmd == b->cmd;
		case W_VALUE_STRING: {
			if(b->type != W_VALUE_STRING)
				return false;
			w_string_t *sa = a->string;
			w_string_t *sb = b->string;
			if(sa->len != sb->len)
				return false;
			for(size_t i = 0; i < sa->len; i++)
				if(sa->ptr[i] != sb->ptr[i])
					return false;
			return true;
		}
		case W_VALUE_LIST: {
			if(b->type != W_VALUE_LIST)
				return false;
			w_list_t *la = a->list;
			w_list_t *lb = b->list;
			if(la->len != lb->len)
				return false;
			for(size_t i = 0; i < la->len; i++)
				if(w_value_equal(&la->ptr[i], &lb->ptr[i]))
					return false;
			return true;
		}
		case W_VALUE_MAP: {
			if(b->type != W_VALUE_MAP)
				return false;
			// TODO: full implementation
			return a->map == b->map;
		}
	}
}
//	W_VALUE_NULL, // null value
//	W_VALUE_INT, // 64-bit signed integer
//	W_VALUE_FLOAT, // 64-bit floating point
//	W_VALUE_EXTERNCMD, // external command
//	// refcounted
//	W_VALUE_STRING,
//	W_VALUE_LIST,
//	W_VALUE_MAP

bool w_value_truthy(w_value_t *v) {
	switch(v->type) {
		case W_VALUE_NULL:
			return false;
		case W_VALUE_INT:
			return v->int_ != 0;
		case W_VALUE_FLOAT:
			return !FEQUAL(0, v->float_);
		case W_VALUE_EXTERNCMD:
		case W_VALUE_STRING:
		case W_VALUE_LIST:
		case W_VALUE_MAP:
			return true;
	}
}

char *w_cstring(w_string_t *str) {
	char *cstr = malloc(str->len+1);
	memcpy(cstr, str->ptr, str->len);
	cstr[str->len] = '\0';
	return cstr;
}

bool w_streqc(w_string_t *a, char *b) {
	size_t blen = strlen(b);
	if(a->len != blen)
		return false;
	for(size_t i = 0; i < blen; i++)
		if(a->ptr[i] != b[i])
			return false;
	return true;
}

static w_value_t list_get(w_ctx_t *ctx, w_list_t *l, int64_t idx) {
	if(idx < 0 || idx >= l->len) {
		w_status_err(ctx->status, w_error_new((w_filepos_t){}, "Index %" PRId64 " out of bounds for list of length %zu.", idx, l->len));
		return (w_value_t){};
	}
	w_value_t v = l->ptr[idx];
	w_value_ref(&v);
	return v;
}

static w_value_t string_get(w_ctx_t *ctx, w_string_t *s, int64_t idx) {
	if(idx < 0 || idx >= s->len) {
		w_status_err(ctx->status, w_error_new((w_filepos_t){}, "Index %" PRId64 " out of bounds for string of length %zu.", idx, s->len));
		return (w_value_t){};
	}
	char *c = malloc(1);
	*c = s->ptr[idx];
	w_string_t *str = malloc(sizeof(w_string_t));
	*str = (w_string_t){1, 1, c};
	return (w_value_t){.type = W_VALUE_STRING, .string = str};
}

w_value_t w_value_index(w_ctx_t *ctx, w_value_t *left, w_value_t *right) {
	#define CMD(NAME) do { \
		w_ecmd_t *ecmd = malloc(sizeof(w_ecmd_t)); \
		w_value_t *v = malloc(sizeof(w_value_t)); \
		*v = *left; \
		w_value_ref(v); \
		*ecmd = (w_ecmd_t){1, &w_cmd_##NAME, v}; \
		return (w_value_t){.type = W_VALUE_EXTERNCMD, .externcmd = ecmd}; \
	} while(0)
	switch(left->type) {
		case W_VALUE_STRING:
			switch(right->type) {
				case W_VALUE_INT:
					return string_get(ctx, left->string, right->int_);
				case W_VALUE_FLOAT:
					return string_get(ctx, left->string, right->float_);
				case W_VALUE_STRING: {
					w_string_t *str = right->string;
					if(w_streqc(str, "len"))
						return (w_value_t){.type = W_VALUE_INT, .int_ = (int64_t)left->string->len};
					if(w_streqc(str, "set!"))
						CMD(string_set_mut);
					if(w_streqc(str, "set"))
						CMD(string_set);
					if(w_streqc(str, "slice!"))
						CMD(string_slice_mut);
					if(w_streqc(str, "slice"))
						CMD(string_slice);
					if(w_streqc(str, "dup!"))
						CMD(string_dup_mut);
					if(w_streqc(str, "dup"))
						CMD(string_dup);
					if(w_streqc(str, "split"))
						CMD(string_split);
					if(w_streqc(str, "reverse!"))
						CMD(string_reverse_mut);
					if(w_streqc(str, "reverse"))
						CMD(string_reverse);
					if(w_streqc(str, "cat!"))
						CMD(string_cat_mut);
					if(w_streqc(str, "cat"))
						CMD(string_cat);
					char *cstr = w_cstring(str);
					w_status_err(ctx->status, w_error_new((w_filepos_t){}, "No member '%s' in string.", cstr));
					free(cstr);
					return (w_value_t){};
				}
			}
		case W_VALUE_LIST:
			switch(right->type) {
				case W_VALUE_INT:
					return list_get(ctx, left->list, right->int_);
				case W_VALUE_FLOAT:
					return list_get(ctx, left->list, floor(right->float_));
				case W_VALUE_STRING: {
					w_string_t *str = right->string;
					// maybe I should make a hashtable of builtin functions to speed this up, since this is effectively just linear search
					// TODO
					if(w_streqc(str, "len"))
						return (w_value_t){.type = W_VALUE_INT, .int_ = (int64_t)left->list->len};
					if(w_streqc(str, "set!"))
						CMD(list_set_mut);
					if(w_streqc(str, "set"))
						CMD(list_set);
					if(w_streqc(str, "clone"))
						CMD(clone);
					if(w_streqc(str, "push!"))
						CMD(list_push_mut);
					if(w_streqc(str, "push"))
						CMD(list_push);
					if(w_streqc(str, "unshift!"))
						CMD(list_unshift_mut);
					if(w_streqc(str, "unshift"))
						CMD(list_unshift);
					if(w_streqc(str, "pop!"))
						CMD(list_pop_mut);
					if(w_streqc(str, "pop"))
						CMD(list_pop);
					if(w_streqc(str, "shift!"))
						CMD(list_shift_mut);
					if(w_streqc(str, "shift"))
						CMD(list_shift);
					if(w_streqc(str, "slice!"))
						CMD(list_slice_mut);
					if(w_streqc(str, "slice"))
						CMD(list_slice);
					if(w_streqc(str, "cat!"))
						CMD(list_cat_mut);
					if(w_streqc(str, "cat"))
						CMD(list_cat);
					if(w_streqc(str, "fill!"))
						CMD(list_fill_mut);
					if(w_streqc(str, "fill"))
						CMD(list_fill);
					if(w_streqc(str, "dup!"))
						CMD(list_dup_mut);
					if(w_streqc(str, "dup"))
						CMD(list_dup);
					if(w_streqc(str, "reverse!"))
						CMD(list_reverse_mut);
					if(w_streqc(str, "reverse"))
						CMD(list_reverse);
					char *cstr = w_cstring(str);
					w_status_err(ctx->status, w_error_new((w_filepos_t){}, "No member '%s' in list.", cstr));
					free(cstr);
					return (w_value_t){};
				}
			}
			break;
		case W_VALUE_MAP:
			switch(right->type) {
				case W_VALUE_STRING: {
					w_string_t *str = right->string;
					if(w_streqc(str, "set!"))
						CMD(map_set_mut);
					if(w_streqc(str, "set"))
						CMD(map_set);
					else if(w_streqc(str, "clone"))
						CMD(clone);
					else if(w_streqc(str, "del!"))
						CMD(map_del_mut);
					else if(w_streqc(str, "del"))
						CMD(map_del);
					w_astring_t astr = (w_astring_t){str->len, str->ptr};
					w_value_t *val = w_map_get(left->map, &astr);
					if(val == NULL) {
						char *cstr = w_cstring(str);
						w_status_err(ctx->status, w_error_new((w_filepos_t){}, "No member '%s' in map.", cstr));
						free(cstr);
						return (w_value_t){};
					}
					w_value_ref(val);
					return *val;
				}
			}
			break;
	}
	w_status_err(ctx->status, w_error_new((w_filepos_t){}, "Can not index %s with %s.", w_typename(left->type), w_typename(right->type)));
	return (w_value_t){};
	#undef CMD
}

w_value_t w_value_clone(w_value_t *v) {
	switch(v->type) {
		default:
			return *v; // for non refcounted types
		case W_VALUE_LIST: {
			w_list_t *l = v->list;
			w_value_t *ptr = malloc(sizeof(w_value_t)*l->len);
			memcpy(ptr, l->ptr, sizeof(w_value_t)*l->len);
			for(size_t i = 0; i < l->len; i++)
				w_value_ref(&ptr[i]);
			w_list_t *new = malloc(sizeof(w_list_t));
			*new = (w_list_t){1, l->len, ptr};
			return (w_value_t){.type = W_VALUE_LIST, .list = new};
		}
		case W_VALUE_STRING: {
			w_string_t *s = v->string;
			char *ptr = malloc(s->len);
			memcpy(ptr, s->ptr, s->len);
			w_string_t *new = malloc(sizeof(w_string_t));
			*new = (w_string_t){1, s->len, ptr};
			return (w_value_t){.type = W_VALUE_STRING, .string = new};
		}
		case W_VALUE_MAP: {
			w_map_t *map = malloc(sizeof(w_map_t));
			*map = w_map_clone(v->map, 1); // clone with refcount 1
			return (w_value_t){.type = W_VALUE_MAP, .map = map};
		}
	}
}

// map impl

static void map_free(w_refcount_t refcount, w_value_t *val) {
	w_value_release(val);
}

static w_value_t map_clone(w_refcount_t refcount, w_value_t *val) {
	w_value_ref(val);
	return *val;
}

W_HASHTABLE_C(w_map, w_value_t, w_refcount_t, map_free, map_clone);

// vartable impl

static void vt_free(w_scope_t scope, w_var_t *var) {
	if(var->scope == scope) {
		w_value_release(var->val);
		free(var->val);
	}
}

static w_var_t vt_clone(w_scope_t scope, w_var_t *var) {
	return *var;
}

W_HASHTABLE_C(w_vartable, w_var_t, w_scope_t, vt_free, vt_clone);

w_ctx_t w_empty_ctx(w_status_t *status) {
	return (w_ctx_t){0, w_vartable_new(512, 0), status};
}

w_value_t w_make_command(w_externcmd_t fp) {
	w_ecmd_t *ecmd = malloc(sizeof(w_ecmd_t));
	*ecmd = (w_ecmd_t){1, fp, NULL};
	return (w_value_t){.type = W_VALUE_EXTERNCMD, .externcmd = ecmd};
}

w_ctx_t w_default_ctx(w_status_t *status) {
	w_ctx_t ctx = w_empty_ctx(status);
	#define CMD(NAME) w_make_command(&w_cmd_##NAME)
	w_ctx_letc(&ctx, "echo", CMD(echo));
	w_ctx_letc(&ctx, "echoln", CMD(echoln));
	w_ctx_letc(&ctx, "read", CMD(read));
	w_ctx_letc(&ctx, "readln", CMD(readln));
	w_ctx_letc(&ctx, "write", CMD(write));
	
	w_ctx_letc(&ctx, "+", CMD(add));
	w_ctx_letc(&ctx, "-", CMD(sub));
	w_ctx_letc(&ctx, "*", CMD(mul));
	w_ctx_letc(&ctx, "/", CMD(div));
	w_ctx_letc(&ctx, "%", CMD(mod));
	
	w_ctx_letc(&ctx, "=", CMD(equ));
	w_ctx_letc(&ctx, "!=", CMD(neq));
	w_ctx_letc(&ctx, "<", CMD(lt));
	w_ctx_letc(&ctx, "<=", CMD(lte));
	w_ctx_letc(&ctx, ">", CMD(gt));
	w_ctx_letc(&ctx, ">=", CMD(gte));
	w_ctx_letc(&ctx, "&", CMD(and));
	w_ctx_letc(&ctx, "|", CMD(or));
	
	w_ctx_letc(&ctx, "int", CMD(int));
	w_ctx_letc(&ctx, "float", CMD(float));
	w_ctx_letc(&ctx, "string", CMD(string));
	
	w_ctx_letc(&ctx, "set!", CMD(set));
	w_ctx_letc(&ctx, "let!", CMD(let));
	w_ctx_letc(&ctx, "swap!", CMD(swap));
	w_ctx_letc(&ctx, "del!", CMD(del));
	
	w_ctx_letc(&ctx, "if", CMD(if));
	w_ctx_letc(&ctx, "break", CMD(break));
	w_ctx_letc(&ctx, "continue", CMD(continue));
	w_ctx_letc(&ctx, "return", CMD(return));
	w_ctx_letc(&ctx, "while", CMD(while));
	w_ctx_letc(&ctx, "do", CMD(do));
	w_ctx_letc(&ctx, "for", CMD(for));
	
	w_ctx_letc(&ctx, "list", CMD(list));
	w_ctx_letc(&ctx, "new-list", CMD(new_list));
	w_ctx_letc(&ctx, "range", CMD(range));
	w_ctx_letc(&ctx, "map", CMD(map));
	
	w_ctx_letc(&ctx, "refcount", CMD(refcount));
	
	w_ctx_letc(&ctx, "cmd", CMD(cmd));
	#undef CMD
	return ctx;
}

w_value_t *w_ctx_get(w_ctx_t *ctx, w_astring_t *str) {
	w_var_t *var = w_vartable_get(&ctx->vartable, str);
	if(var == NULL)
		return NULL;
	return var->val;
}

void w_ctx_set(w_ctx_t *ctx, w_astring_t *str, w_value_t val) {
	w_var_t *vp = w_vartable_get(&ctx->vartable, str);
	if(vp == NULL) {
		char *cstr = w_ast_cstr(str);
		w_status_err(ctx->status, w_error_new((w_filepos_t){}, "Variable %s does not exist. (perhaps you meant to use let!)", cstr));
		free(cstr);
		return;
	}
	w_value_release(vp->val);
	*vp->val = val;
}

void w_ctx_let(w_ctx_t *ctx, w_astring_t *str, w_value_t val) {
	w_var_t *vp = w_vartable_get(&ctx->vartable, str);
	if(vp != NULL && vp->scope == ctx->scope) {
		char *cstr = w_ast_cstr(str);
		w_status_err(ctx->status, w_error_new((w_filepos_t){}, "Cannot redeclare variable %s. (perhaps you meant to use set!)", cstr));
		free(cstr);
		return;
	}
	w_var_t var = (w_var_t){ctx->scope, malloc(sizeof(w_value_t))};
	*var.val = val;
	w_vartable_set(&ctx->vartable, str, var);
}

void w_ctx_del(w_ctx_t *ctx, w_astring_t *str) {
	w_vartable_del(&ctx->vartable, str);
}

w_value_t *w_ctx_getc(w_ctx_t *ctx, char *cstr) {
	w_astring_t str = (w_astring_t){strlen(cstr), cstr};
	return w_ctx_get(ctx, &str);
}

void w_ctx_setc(w_ctx_t *ctx, char *cstr, w_value_t val) {
	w_astring_t str = (w_astring_t){strlen(cstr), cstr};
	return w_ctx_set(ctx, &str, val);
}

void w_ctx_letc(w_ctx_t *ctx, char *cstr, w_value_t val) {
	w_astring_t str = (w_astring_t){strlen(cstr), cstr};
	return w_ctx_let(ctx, &str, val);
}

w_ctx_t w_ctx_clone(w_ctx_t *ctx) {
	w_ctx_t new = (w_ctx_t){ctx->scope+1, w_vartable_clone(&ctx->vartable, ctx->vartable.data+1), ctx->status};
	return new;
}
void w_ctx_free(w_ctx_t *ctx) {
	w_vartable_free(&ctx->vartable);
}

// actual eval implementation (shared ctx replaces a call to w_ctx_sub() if present)
static w_value_t eval(w_ctx_t *ctx, w_ast_t *ast, w_ctx_t *sub_ctx, w_value_t *this) {
	switch(ast->type) {
		case W_AST_STRING: {
			w_string_t *str = malloc(sizeof(w_string_t));
			*str = (w_string_t){1, ast->string.len, malloc(ast->string.len)};
			memcpy(str->ptr, ast->string.ptr, str->len);
			return (w_value_t){
				.type = W_VALUE_STRING,
				.string = str
			};
		}
		case W_AST_INT:
			return (w_value_t){
				.type = W_VALUE_INT,
				.int_ = ast->int_
			};
		case W_AST_FLOAT:
			return (w_value_t){
				.type = W_VALUE_FLOAT,
				.float_ = ast->float_
			};
		case W_AST_NULL:
			return (w_value_t){.type = W_VALUE_NULL};
		case W_AST_VAR: {
			if(w_astreqc(&ast->string, "this")) {
				if(this == NULL)
					return (w_value_t){.type = W_VALUE_NULL};
				w_value_ref(this);
				return *this;
			}
			w_value_t *v = w_ctx_get(ctx, &ast->string);
			if(v == NULL) {
				char *name = w_ast_cstr(&ast->string);
				w_status_err(ctx->status, w_error_new(ast->pos, "Unbound string %s.", name));
				free(name);
				return (w_value_t){};
			}
			w_value_ref(v);
			return *v;
		}
		case W_AST_COMMANDS: {
			w_ctx_t _sub; // uninitialized if not used
			w_ctx_t *sub;
			if(sub_ctx == NULL) {
				_sub = w_ctx_clone(ctx);
				sub = &_sub;
			} else
				sub = sub_ctx;
			for(size_t i = 0; i < ast->commands.len; i++) {
				w_ast_command_t *cmd = &ast->commands.ptr[i];
				w_ast_t *name = &cmd->ptr[0];
				w_value_t vcmd;
				// get a command from the name AST
				if(name->type == W_AST_STRING) {
					w_value_t *v = w_ctx_get(sub, &name->string);
					if(v == NULL) {
						char *c = w_ast_cstr(&name->string);
						w_status_err(ctx->status, w_error_new(name->pos, "Unbound string %s.", c));
						free(c);
						if(sub_ctx == NULL)
							w_ctx_free(sub);
						return (w_value_t){};
					}
					if(v->type != W_VALUE_EXTERNCMD && v->type != W_VALUE_COMMAND) {
						w_status_err(ctx->status, w_error_new(name->pos, "0 Expected command, got %s.", w_typename(v->type)));
						w_value_release(v);
						if(sub_ctx == NULL)
							w_ctx_free(sub);
						return (w_value_t){};
					}
					w_value_ref(v);
					vcmd = *v;
				} else {
					w_value_t v = eval(sub, name, NULL, this);
					if(sub->status->tag != W_STATUS_OK) {
						if(sub_ctx == NULL)
							w_ctx_free(sub);
						return (w_value_t){};
					}
					switch(v.type) {
						case W_VALUE_EXTERNCMD:
						case W_VALUE_COMMAND:
							vcmd = v;
							break;
						default:
							w_value_release(&v);
							w_status_err(ctx->status, w_error_new(name->pos, "1 Expected command, got %s.", w_typename(v.type)));
							if(sub_ctx == NULL)
								w_ctx_free(sub);
							return (w_value_t){};
					}
				}
				// construct argument list
				w_args_t args = (w_args_t){cmd->len-1, cmd->ptr+1};
				// call command
				w_value_t ret;
				#define FREE \
					if(sub_ctx == NULL) \
						w_ctx_free(sub); \
					w_value_release(&vcmd);
				switch(vcmd.type) {
					case W_VALUE_EXTERNCMD: {
						w_ecmd_t *ecmd = vcmd.externcmd;
						ret = ecmd->cmd(name->pos, sub, this, ecmd->obj, args);
						if(sub->status->tag != W_STATUS_OK) {
							FREE;
							return (w_value_t){};
						}
						break;
					}
					case W_VALUE_COMMAND: {
						w_cmd_t *cmd = vcmd.cmd;
						w_ctx_t cmdctx = w_ctx_clone(sub);
						for(size_t i = 0; i < cmd->argc; i++) {
							if(i >= args.len) {
								w_ctx_let(&cmdctx, &cmd->args[i].name, (w_value_t){.type = W_VALUE_NULL});
								continue;
							}
							w_value_t val = eval(sub, &args.ptr[i], NULL, this);
							if(sub->status->tag != W_STATUS_OK) {
								FREE;
								w_ctx_free(&cmdctx);
								return (w_value_t){};
							}
							w_ctx_let(&cmdctx, &cmd->args[i].name, val);
						}
						ret = eval(&cmdctx, &cmd->impl, NULL, cmd->this != NULL ? cmd->this : this);
						w_ctx_free(&cmdctx);
						switch(sub->status->tag) {
							case W_STATUS_OK:
								break;
							case W_STATUS_RETURN:;
								w_value_t ret = *sub->status->ret;
								w_value_ref(&ret);
								w_status_ok(sub->status);
								w_value_release(&vcmd);
								return ret;
							default:
								FREE;
								return (w_value_t){};
						}
						break;
					}
				}
				#undef FREE
				w_value_release(&vcmd);
				if(i+1 == ast->commands.len) {
					if(sub_ctx == NULL)
						w_ctx_free(sub);
					return ret;
				}
				w_value_release(&ret);	
			}
			break;
		}
		case W_AST_INDEX: {
			w_ast_index_t idx = ast->index;
			w_value_t left = eval(ctx, idx.left, NULL, this);
			if(ctx->status->tag != W_STATUS_OK)
				return (w_value_t){};
			w_value_t right = eval(ctx, idx.right, NULL, this);
			if(ctx->status->tag != W_STATUS_OK) {
				w_value_release(&left);
				return (w_value_t){};
			}
			w_value_t ret = w_value_index(ctx, &left, &right);
			w_value_release(&left);
			w_value_release(&right);
			if(ctx->status->tag != W_STATUS_OK) {
				ctx->status->err->pos = idx.left->pos;
				return (w_value_t){};
			}
			return ret;
		}
	}
}

w_value_t w_eval(w_ctx_t *ctx, w_ast_t *ast) {
	return eval(ctx, ast, NULL, NULL);
}

w_value_t w_evals(w_ctx_t *ctx, w_ctx_t *sub, w_ast_t *ast) {
	return eval(ctx, ast, sub, NULL);
}

w_value_t w_evalt(w_ctx_t *ctx, w_value_t *this, w_ast_t *ast) {
	return eval(ctx, ast, NULL, this);
}

w_value_t w_evalst(w_ctx_t *ctx, w_ctx_t *sub, w_value_t *this, w_ast_t *ast) {
	return eval(ctx, ast, sub, this);
}
