#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>

#ifdef __GLIBC__
#include <execinfo.h>
#endif
#ifdef _WIN32
#include <libloaderapi.h>
#include <fileapi.h>
#else
#include <unistd.h>
#include <sys/stat.h>
#endif

#include "util.h"

w_error_t *w_error_new(w_filepos_t pos, char *fmt, ...) {
	char *msg = malloc(W_MAX_ERROR_LEN);

	va_list ap;
	va_start(ap, fmt);
	vsnprintf(msg, W_MAX_ERROR_LEN, fmt, ap);
	va_end(ap);
	
	w_error_t *e = malloc(sizeof(w_error_t));
	*e = (w_error_t){pos, msg};
	return e;
}

void w_error_print(w_error_t *err, FILE *fp) {
	fprintf(fp, "%s:%zu:%zu: %s\n", err->pos.filename, err->pos.line+1, err->pos.col+1, err->msg);
}

void w_error_free(w_error_t *err) {
	free(err->msg);
	free(err);
}

void w_value_release(w_value_t *ptr);
void w_value_ref(w_value_t *ptr);

void w_status_free(w_status_t *s) {
	switch(s->tag) {
		case W_STATUS_OK:
			break;
		case W_STATUS_ERR:
			w_error_free(s->err);
			break;
		case W_STATUS_RETURN:
			w_value_release(s->ret);
			free(s->ret);
			break;
	}
}

void w_status_simple(w_status_t *s, w_status_tag_t tag) {
	w_status_free(s);
	s->tag = tag;
}

void w_status_err(w_status_t *s, w_error_t *err) {
	w_status_free(s);
	s->tag = W_STATUS_ERR;
	s->err = err;
}

void w_hack_set_value(w_value_t *a, w_value_t *b); // does *a = *b
																									 // here to bypass C #include hell
w_value_t *w_hack_malloc_value(void);

void w_status_return(w_status_t *s, w_value_t *val) {
	w_status_free(s);
	w_value_t *pv = w_hack_malloc_value();
	w_hack_set_value(pv, val);
	w_value_ref(pv);
	s->tag = W_STATUS_RETURN;
	s->ret = pv;
}

#if __GLIBC__
void w_print_backtrace(void) {
	void *buf[4096];
	int size = backtrace(buf, 4096);
	char **strings = backtrace_symbols(buf, size);
	if(strings != NULL) {
		for(int i = 0; i < size; i++)
			printf("%s\n", strings[i]);
	}
	free(strings);
}
#endif

w_writer_t w_writer_new(void) {
	return (w_writer_t){malloc(4096), 4096, 0};
}

void w_writer_putch(w_writer_t *w, char c) {
	w->buf[w->len++] = c;
	if(w->len >= w->buf_len) {
		w->buf_len += 4096;
		w->buf = realloc(w->buf, w->buf_len);
	}
}

void w_writer_puts(w_writer_t *w, size_t len, char *str) {
	for(size_t i = 0; i < len; i++)
		w_writer_putch(w, str[i]);
}

void w_writer_putcs(w_writer_t *w, char *str) {
	w_writer_puts(w, strlen(str), str);
}
void w_writer_resize(w_writer_t *w) {
	w->buf = realloc(w->buf, w->len);
	w->buf_len = w->len;
}

bool w_is_int(char *str, size_t len) {
	for(size_t i = 0; i < len; i++) {
		char c = str[i];
		if(i == 0 && c == '-' && len > 1)
			continue;
		if(c < '0' || c > '9') 
			return false;
	}
	return true;
}

bool w_is_float(char *str, size_t len) {
	bool dot = false;
	for(size_t i = 0; i < len; i++) {
		char c = str[i];
		if(i == 0 && c == '-' && len > 1)
			continue;
		if(c == '.' && !dot) {
			dot = true;
			break;
		}
		if(c < '0' || c > '9') 
			return false;
	}
	return true;
	
}

int64_t w_parse_int(char *str, size_t len) {
	bool minus = false;
	int64_t ret = 0;
	for(size_t i = 0; i < len; i++) {
		if(str[i] == '-') {
			minus = true;
			continue;
		}
		ret *= 10;
		ret += str[i]-'0';
	}
	if(minus)
		ret = -ret;
	return ret;
}

double w_parse_float(char *str, size_t len) {
	bool minus = false, decimal = false;
	double ret = 0, place = 0.1; // place for decimals
	for(size_t i = 0; i < len; i++) {
		char c = str[i];
		if(c == '-') {
			minus = true;
			continue;
		}
		if(c == '.') {
			decimal = true;
			continue;
		}
		if(decimal) {
			ret += (c-'0')*place;
			place *= 0.1;
			continue;
		}
		ret *= 10;
		ret += c-'0';
	}
	if(minus)
		ret = -ret;
	return ret;
}

char *w_readline(char *prompt) {
	printf("%s", prompt);
	w_writer_t w = w_writer_new();
	int c;
	while(true) {
		c = getchar();
		if(c == '\n')
			break;
		if(c == EOF) {
			free(w.buf);
			return NULL;
		}
		w_writer_putch(&w, c);
	}
	w_writer_putch(&w, '\0');
	w_writer_resize(&w);
	return w.buf;
}

char *w_lib_path(void) {
	#define BUF_SIZE 4096
	char *buf = malloc(BUF_SIZE);
	// get executable directory
	#ifdef __linux__
		readlink("/proc/self/exe", buf, BUF_SIZE);
	#elif defined(__FreeBSD__)
		readlink("/proc/curproc/file", buf, BUF_SIZE);
	#elif defined(_WIN32)
		GetModuleFileNameA(NULL, buf, BUF_SIZE);
	#else
		#error "Unsupported operating system."
	#endif
	// replace filename with '/lib'
	for(size_t i = strlen(buf); i > 0; i--) {
		if(buf[i] == '/' || buf[i] == '\\') {
			strncpy(buf+i+1, "lib", BUF_SIZE-i-1);
			break;
		}
	}
	// create directory if it doesn't exist
	#ifdef _WIN32
		DWORD attrib = GetFileAttributes(buf);
		if(attrib == INVALID_FILE_ATTRIBUTES)
			CreateDirectoryA(buf, 0);
	#else
		struct stat st;
		if(stat(buf, &st) != 0)
			mkdir(buf, 0755);
	#endif
	return buf;
	#undef BUF_SIZE
}
