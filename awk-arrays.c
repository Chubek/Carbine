static inline void
initialize_awkarray_container(void)
{
	AWKARRAY_ALL = (struct AwkArrays*)GC_MALLOC(1 * sizeof(struct AwkArrays));
}

	
static inline void
initialze_awkarray(uint8_t *ident)
{
	struct AwkArray *array = GC_MALLOC(sizeof(uintptr_t));
	AWKARRAY_ALL = 
		(struct AwkArrays*)GC_REALLOC(AWKARRAY_ALL, ++AWKARRAY_NEXTIDX * sizeof(struct AwkArrays));;
	hashtable_push(AWKARRAY_ADDRESSES, ident, (uint8_t*)array);
}

static inline void
push_awkarray(uint8_t *ident, uintptr_t key, uintptr_t value, AwkArrayType type)
{
	struct AwkArray *array = 
		(struct AwkArray)hashtable_get(AWKARRAY_ADDRESSES, ident);
	array->key = key;
	array->value = value;
	array->value_type = type;
}

static inline void
delete_awkarray(uint8_t *ident)
{
	struct AwkArray *array =
		(struct AwkArray)hashtable_get(AWKARRAY_ADDRESSES, ident);
	GC_FREE(array);
}
