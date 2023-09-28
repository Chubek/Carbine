#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib>
#include <mqueue.h>
#include <unistd.h>
#include <errno.h>
#include <wchar.h>

#include "symtable.h"
#include "symstack.h"
#include "m4builtin.h"

typedef struct m4Cookie {
	FILE *input, *output, *outerr, *trace;
	uint8_t *trace_str;
	size_t trace_len;
	bool *traceonptr;
	mqd_t channel;
	symtable_t symtable;
	symstack_t symstack;
} m4cookie_t;

static void
m4cookie_init(
		m4cookie_t *cookie, 
		uint8_t *infile, 
		uint8_t *outfile,
		uint8_t *errfile,
		bool *traceonptr
	) {

	cookie->input = infile 
				? fopen(infile, "r")
				: stdin;
	cookie->output = outfile 
				? fopen(outfile, "w")
				: stdout;
	cookie->outerr = errfile
				? fopen(errfile, "w")
				: stderr;
	cookie->traceonptr = traceonptr;
	cookie->trace = open_memstream(&cookie->trace_str, &cookie->trace_len);
}

static void
m4cookie_destroy(m4cookie_t *cookie)
{
	!isatty(cookie->input) 
				? fclose(cookie->input)
				: NULL;

	!isatty(cookie->output)
				? fclose(cookie->output)
				: NULL;
	!isatty(cookie->stderr)
				? fclose(cookie->outerr)
				: NULL;
	fclose(cookie->trace);
	free(cookie->trace_str);
}



