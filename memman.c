static inline void
init_mestack(void) {  obstack_init(MEMSTACK); }

static inline void
free_memstack(void) {  obstack_free(MEMSTACK, NULL); }

static inline void
deallocate_on_memstack(uintptr_t address)
{
	obstack_free(MEMSTACK, (void*)address);
}

static inline uintptr_t
get_variable_address(int8_t *ident)
{
	char *format_string;
	uintptr_t address;
	sprintf(format_string, "%%*s %s -> %%lu\n", ident);
	fscanf(BOOK_STREAM, format_string, &address);
	
	return pointer;
}

static inline uintptr_t
allocate_variable_on_memstack(int8_t *ident, void *data, int size)
{
	obstack_grow(MEMSTACK, data, size);
	return (uintptr_t)obstack_finish(MEMSTACK);
}

static inline void
record_variable_address(int8_t *data, uintptr_t address)
{
	fprintf(BOOK_STREAM, " %s -> %lu\n", ident, (uintptr_t)address);
}


