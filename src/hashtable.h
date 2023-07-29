// type-generic macro for hashtables, and a hashing function for w_astring_t

#ifndef HASHTABLE_H
#define HASHTABLE_H

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "parser.h"

size_t w_hash(w_astring_t *str);


// TODO: (automatic) hashtable resizing

// DATA is the type of some arbitrary data that is in the hashmap struct
#define W_HASHTABLE_H(NAME, T, DATA) \
typedef struct NAME##_list { \
	w_astring_t key; /* the key for this value */ \
	T item; /* the item */ \
	struct NAME##_list *next; /* next item */ \
} NAME##_list_t; \
 \
typedef struct NAME { \
	size_t capacity; /* current capacity (for now this is effectively a constant) */ \
	NAME##_list_t **ptr; \
	DATA data; \
} NAME##_t; \
 \
NAME##_t NAME##_new(size_t capacity, DATA data); /* creates a new hashmap with the given capacity */\
void NAME##_free(NAME##_t *tbl); /* Frees a hashmap */ \
void NAME##_set(NAME##_t *tbl, w_astring_t *str, T value); /* sets a value in a hashmap */ \
void NAME##_setc(NAME##_t *tbl, char *str, T value); /* same as above, except uses a cstring instead */ \
T *NAME##_get(NAME##_t *tbl, w_astring_t *str); /* gets a value from a hashmap. returns NULL if it doesn't exist */\
T *NAME##_gets(NAME##_t *tbl, char *str); /* same as above, except uses a cstring instead */\
void NAME##_del(NAME##_t *tbl, w_astring_t *str); /* deletes a value from a hashmap */ \
void NAME##_delc(NAME##_t *tbl, char *str); /* same as above, except uses a cstring instead */ \
NAME##_t NAME##_clone(NAME##_t *tbl, DATA new_data);

// FREE and CLONE are freeing and cloning functions. They are both called with the data and an item.
#define W_HASHTABLE_C(NAME, T, DATA, FREE, CLONE) \
NAME##_t NAME##_new(size_t capacity, DATA data) { \
	NAME##_list_t **ptr = malloc(sizeof(NAME##_list_t *)*capacity); \
	for(size_t i = 0; i < capacity; i++) \
		ptr[i] = NULL; \
	return (NAME##_t){capacity, ptr, data}; \
} \
static void NAME##_list_free(DATA d, NAME##_list_t *l) { \
	free(l->key.ptr); \
	FREE(d, &l->item); \
	free(l); \
} \
void NAME##_free(NAME##_t *tbl) { \
	for(size_t i = 0; i < tbl->capacity; i++) { \
		NAME##_list_t *curr = tbl->ptr[i]; \
		while(curr != NULL) { \
			NAME##_list_t *next = curr->next; \
			NAME##_list_free(tbl->data, curr); \
			curr = next; \
		} \
	} \
	free(tbl->ptr); \
} \
void NAME##_set(NAME##_t *tbl, w_astring_t *str, T value) { \
	size_t hash = w_hash(str)%tbl->capacity; \
	NAME##_list_t *curr = tbl->ptr[hash], *prev = curr; \
	while(curr != NULL) { \
		if(w_astreq(str, &curr->key)) { \
			curr->item = value; \
			return; \
		} \
		prev = curr; \
		curr = curr->next; \
	} \
	NAME##_list_t *node = malloc(sizeof(NAME##_list_t)); \
	*node = (NAME##_list_t){w_astrdup(str), value, NULL}; \
	if(prev == NULL) { \
		tbl->ptr[hash] = node; \
		return; \
	} \
	prev->next = node; \
} \
void NAME##_setc(NAME##_t *tbl, char *str, T value) { \
	w_astring_t a = (w_astring_t){strlen(str), str}; \
	NAME##_set(tbl, &a, value); \
} \
T *NAME##_get(NAME##_t *tbl, w_astring_t *str) { \
	size_t hash = w_hash(str)%tbl->capacity; \
	NAME##_list_t *curr = tbl->ptr[hash]; \
	while(true) { \
		if(curr == NULL) \
			return NULL; \
		if(w_astreq(str, &curr->key)) \
			return &curr->item; \
		curr = curr->next; \
	} \
} \
T *NAME##_getc(NAME##_t *tbl, char *str) { \
	w_astring_t a = (w_astring_t){strlen(str), str}; \
	return NAME##_get(tbl, &a); \
} \
void NAME##_del(NAME##_t *tbl, w_astring_t *str) { \
	size_t hash = w_hash(str)%tbl->capacity; \
	NAME##_list_t *curr = tbl->ptr[hash], *prev = NULL; \
	if(curr == NULL) \
		return; \
	while(true) { \
		if(w_astreq(str, &curr->key)) { \
			if(prev == NULL) \
				tbl->ptr[hash] = curr->next; \
			else \
				prev->next = curr->next; \
			NAME##_list_free(tbl->data, curr); \
			return; \
		} \
		prev = curr; \
		curr = curr->next;  \
	} \
} \
void NAME##_delc(NAME##_t *tbl, char *str) { \
	w_astring_t a = (w_astring_t){strlen(str), str}; \
	NAME##_del(tbl, &a); \
} \
static NAME##_list_t *NAME##_list_clone(DATA d, NAME##_list_t *old) { \
	NAME##_list_t *new = malloc(sizeof(NAME##_list_t)); \
	*new = (NAME##_list_t){w_astrdup(&old->key), CLONE(d, &old->item), NULL}; \
	return new; \
} \
NAME##_t NAME##_clone(NAME##_t *tbl, DATA new_data) { \
	NAME##_t ret = NAME##_new(tbl->capacity, new_data); \
	for(size_t i = 0; i < ret.capacity; i++) { \
		NAME##_list_t *old = tbl->ptr[i]; \
		if(old == NULL) { \
			ret.ptr[i] = NULL; \
			continue; \
		} \
		NAME##_list_t *new = NAME##_list_clone(tbl->data, old), *head = new; \
		while(true) { \
			old = old->next; \
			if(old == NULL) \
				break; \
			new->next = NAME##_list_clone(tbl->data, old); \
			new = new->next; \
		} \
		ret.ptr[i] = head; \
	} \
	return ret; \
}

#endif
