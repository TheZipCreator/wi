#include "hashtable.h"

size_t w_hash(w_astring_t *str) {
	if(str->len == 0)
		return 0;
	size_t h = 37;
	for(size_t i = 0; i < str->len; i++)
		h = (h*54059) ^ (str->ptr[i] * 76963);
	return h;
}
