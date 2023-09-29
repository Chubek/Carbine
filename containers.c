#include <stdint.h>
#define GL_STACK_ELEMENT uint8_t*

#include "config.h"
#include "gl_hash_map.h"
#include "stack.h"
#include "obstack.h"
#include "unistr.h"

#include <stdio.h>
#include <stdlib.h>

typedef struct obstack gnuc_obstack_t;
typedef stack_type gl_stack_t;
typedef gl_map_t gl_hashmap_t;

static inline void
init_containers(
		gnuc_obstack_t *obstack, 
		gl_stack_t *stack, 
		gl_hashmap_t *hashmap
	)
{
	obstack_init(obstack);
	stack_init(stack);
	*hashmap = gl_map_nx_create_empty(
				&gl_hash_map_implementation,
				NULL, NULL, NULL, NULL,
			);
}

static inline void
hashtable_push(gl_hashmap_t *hashmap, uint8_t *key, uint8_t *value)
{
	return gl_map_nx_getput(
			hashmap, 
			(const void*)key, 
			(const void*)value, 
			NULL
		);
}

static inline uint8_t*
hashtable_get(gl_hashmap_t *hashmap, uint8_t *key)
{
	uint8_t *result;
	if (gl_map_nx_getput(hashmap, key, NULL &result))
		return result;
	return NULL;
}

static inline void
hashtable_rm(gl_hashmap_t *hashmap, uint8_t *key)
{
	return gl_map_nx_getremove(hashmap, key, NULL);
}






