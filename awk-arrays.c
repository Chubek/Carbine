typedef struct AwkArray  awkarr_t;
	
	
static inline void
initialize_awkarray_container(void)
{
	AWKARR_ALL = 
		(awkarr_t**)GC_MALLOC(AWKARR_INITLEN * sizeof(awkarr_t*));
}

	
static inline void
initialze_awkarray(uint8_t *ident)
{
	AArMn_t *array = GC_MALLOC(sizeof(awkarray_t));
	AWKARR_ALL = 
		(awkarr_t**)GC_REALLOC(AWKARR_ALL, ++AWKARR_IDX * sizeof(awkarr_t*));
	hashtable_push(AWKARR_ADDRS, ident, (uint8_t*)array);
}

static inline void
push_awkarray(uint8_t *ident, uintptr_t key, uintptr_t value, AwkArrayType type)
{
	AArMn_t *array = 
		(AArMn_t*)hashtable_get(AWKARR_ADDRS, ident);
	array->key = key;
	array->value = value;
	array->value_type = type;
	array->deleted = false;
}

static inline void
delete_awkarray(uint8_t *ident)
{
	awkarr_t *array =
		(awkarr_t*)hashtable_get(AWKARRAY_ADDRS, ident);
	array->key = NULL;
	array->value = NULL;
	array->deleted = true;
	
}
