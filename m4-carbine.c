#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <stdarg.h>
#include <string.h>
#include <limits.h>
#include <unistd.h>
#include <setjmp.h>
#include <regexp.h>
#include <errno.h>
#include <sys/types.h>
#include <signal.h>
#include <hat-trie/hat-trie.h>

#include "_eval.yy.c"
#include "m4-carbine.h"

static struct RuntimeState
{
	static struct Flags
	{
		bool force_traceon;
		bool force_prefix;
		bool run_interactive;
		bool line_directive;
	}
	FLAGS;
	
	
	static struct IO
	{
		FILE *instream;
		FILE *outstream;
		FILE *errstream;
		FILE *hold_stream;
		FILE *null_stream;
	}
	IO;

	static struct Tracer
	{
		bool trace_on;
		uint8_t LAST_TRACE[MAX_TRACE_LEN];
	}
	TRACER;

	static struct Container
	{
		gl_stack *SYMSTACK;
		gl_hash_map *SYMTABLE;
	}
	CONTAINERS;

	static struct Invokation
	{
		size_t argc;
		size_t ARGVLENS[MAX_ARGV_NUM];
		uint8_t ARGV[MAX_ARGV_NUM][MAX_ARGV_LEN];
		uint8_t *argv_space_joined;
		uint8_t *argv_comma_joined;
	}
	INVOKATION;
	
	static struct JmpBufs
	{
		jmp_buf BUILTIN[NUM_BUILTINS];
		jmp_buf ERRORS[NUM_ERRORS];
	} 
	NONLOC_JMPBUFS;
	static struct Diversions
	{
		FILE *STREAMS[NUM_DIVERT];
		size_t LENGTHS[NUM_DIVERTS];
		uint8_t *BUFFERS[NUM_DIVERTS];
		int divert_num;
		int undivert_num;
	}
	DIVERSIONS;
	
	static struct Wrap
	{
		FILE *stream;
		size_t length;
		uint8_t *buffer;
	}
	WRAP;

	static struct Syntax
	{
		uint8_t left_quote[MAX_TOKEN_LEN];
		uint8_t right_quote[MAX_TOKEN_LEN];
		uint8_t begin_comment[MAX_TOKEN_LEN];
		uint8_t end_comment[MAX_TOKEN_LEN];
		size_t left_quote_len;
		size_t right_quote_len;
		size_t begin_comment_len;
		size_t end_comment_len;
	}
	SYNTAX;

}
STATE;

#define CURR_ARGC	STATE.INVOKATION.argc
#define ARGV_name 	&STATE.INVOKATION.ARGV[0][0]
#define ARGV_nth(N)	&STATE.INVOKATION.ARGV[N][0]
#define ARGV_star	&STATE.INVOKATION.argv_comma_joined[0]
#define ARGV_atsign	&STATE.INVOKATION.argv_space_joined[0]

#define INSTREAM	STATE.IO.instream
#define OUTSTREAM	STATE.IO.outstream
#define PRISTREAM	STATE.IO.errstream
#define HOLDSTREAM	STATE.IO.holdstream
#define NULLSTREAM	STATE.IO.nullstream

#define SWAP_HOLD_STREAM(to_hold, to_replace)				\
	do {  HOLDSTREAM = to_hold; TO_HOLD = to_replace;  } while (0)
#define RESTORE_HELD_STREAM(held_stream)				\
	do { held_stream = HOLDSTREAM; } while (0)

static void
m4_incr(void)
{
	SET_JMP(ID_INCR);

	uint8_t *inc_num = ARGV_nth(1);
	int64_t eval_inc_num = strtoll(inc_num, NULL, 10);
	OUTPUT_FMT(OUTSTREAM, "%ld", ++eval_inc_num);

	BACKTRACK(INCR_DONE);
}
 
static void
m4_decr(void)
{
 	SET_JMP(ID_DECR);

	uint8_t *dec_num = ARGV_nth(1);
	int64_t eval_dec_num = strtoll(dec_num, NULL, 10);
	OUTPUT_FMT(OUTSTREAM, "%ld", --eval_dec_num);

	BACKTRACK(DECR_DONE);
}


static void
m4_m4wrap(void)
{	
	SET_JMP(ID_M4WRAP);
	
	uint8_t wrap_text = ARGV_nth(1);
	OUTPUT_FMT(WRAPSTREAM, "%s", wrap_text);

	BACKTRACK(M4WRAP_DONE);
}


static void
m4_divert(void)
{
	SET_JMP(ID_DIVERT);
	
	char *buff_num = ARGV_nth(1);
	DIVERTNUM = atoi(buff_num);
	
	if (DIVERTNUM > DIVERT_NUM) 
		// todo errror
	else if (DIVERTNUM < 0) 
	{
		SWAP_HOLD_STREAM(OUTSTREAM, NULLSREAM);
	}
	else
	{
		FILE *divert_stream = DIVERTSTREAM_nth(eval_buff_num);
		SWAP_HOLD_STREAM(OUTSTREAM, divert_stream);
	}
	
	BACKTRACK(DIVERT_DONE);
}


static void
m4_undivert(void)
{
	SET_JMP(ID_UNDIVERT);
	
	char *buff_num = ARGV_nth(1);
	UNDIVERTNUM = atoi(buff_num);
	
	if (UNDIVERTNUM < 0)
		// todo error

	FILE *divert_stream = DIVIERTSREAM_nth(UNDIVERTNUM);
	uint8_t *divert_str = DIVERTSTR_nth(UNDIVERTNUM);
	
	RESTORE_HELD_STREAM(OUTSTREAM);
	OUTPUT_FMT(OUTSTREAM, "%s", divert_str);
	ftruncte(divert_stream, 0);

	BACKTRACK(UNDIVERT_DONE);
}


static void
m4_divnum(void)
{
	SET_JMP(ID_DIVNUM);

	OUTPUT_FMT(OUTSTREAM, "%ld", DIVERTNUM);

	BACKTRACK(DIVNUM_DONE);
}


static void
m4_dnl(void)
{
	SET_JMP(ID_DNL);

	INPUT_FMT(INSTREAM, "%*s\n", NULL);

	BACKTRACK(DNL_DONE);
}




static void
m4_eval(void)
{	
	SET_JMP(ID_EVAL);
	
	yyfinal = 0;
       	eval_yy_init(ARGV_nth(1)); 
	eval_yy_parse();
	eval_yy_restore();

	OUTPUT_FMT(OUTSTREAM, "%ld", yyfinal);

	BACKTRACK(EVAL_DONE);

}




static void
m4_ifdef(void)
{
	SET_JMP(ID_IFDEF);

	uint8_t *focal_symbol = ARGV_nth(1); 
	uint8_t *ifdef = ARGV_nth(2); 
	uint8_t *ifndef = ARGV_nth(3);

	SYMBOL_EXISTS(focal_symbol) 
		? OUTPUT_FMT(OUTSTREAM, ATTEMPT_EVAL, ifdef)
		: OUTPUT_FMT(OUTSTREAM, ATTEMPT_EVAL, ifndef);

	BACKTRACK(IFDEF_DONE);
}

static void
m4_ifelse(void)
{
	SET_JMP(ID_IFELSE);

	ifeqtop = 0;
	int i, j, k, m;

	switch (STATE.argc)
	{
		case 0:
		case 1:
		case 2:
			REJECT(ERRID_IFELSE, ARGC_INSUFF);
			break;
		default:
fill:
			i = --STATE.argc; j = --STATE.argc; 
			k = --STATE.argc; m = --STATE.argc;
			STATE.argc <= 0 
				? goto done
				: goto pushifeq;
pushifeq:
			IFEQ_STACK[ifeqtop].lhs = &STATE.argv[i];
			IFEQ_STACK[ifeqtop].rhs = &STATE.argv[j];
			IFEQ_STACK[ifeqtop].ifeq = &STATE.argv[k];
			IFEQ_STACK[ifeqtop++].ifne = m <= 0
				? NULL
				: &STATE.argv[m];
			goto fill;
done:
			BACKTRACK(IFELSE_DONE);
			break;
	}
}


static void
m4_index(void)
{
	SET_JMP(ID_INDEX);


	uint8_t *haystack = ARGV_nth(1), *needle = ARGV_nth(2);
	size_t idx = strspn(haystack, needle);
	
	fprintf(OUTSTREAM, "%lu", idx);

	BACKTRACK(INDEX_DONE);
}

static void
m4_len(void)
{
	SET_JMP(ID_LEN);


	uint8_t *subject = ARGV_nth(1);
	size_t lensubj = strlen(subject);

	fprintf(OUTSTREAM, "%lu", lensubj);
	
	BACKTRACK(LEN_DONE);
}

static void
m4_mkstemp(void)
{
	SET_JMP(ID_MKSTEMP);

	char *temp;
	int fdesc;
	BACKTRACK(MKSTEMP_DONE);
}

static void
m4_shift(void)
{
	SET_JMP(ID_SHIFT);

	if (STATE.argc < SHIFT_LEAST_ARGC)
		REJECT(ERRID_SHIFT, NO_ARGC);

	size_t shfnum = 2;	
	while (shfnum < STATE.argc)
		fputs(&STATE.argv[shfnum++], OUTSTREAM);
	
	BACKTRACK(SHIFT_DONE);
}

static void
m4_sinclude(void)
{
	SET_JMP(ID_SINCLUDE);

	if (STATE.argc < SINCL_LEAST_ARGC)
		REJECT(ERRID_SINCLUDE, NO_ARGC);
			
	char *file = ARGV_nth(1);
	currincl = fopen(file, "r"); !currincl 
						? BACKTRACK(SINCL_SILENT_FAIL)
						: None;
	while ((getline(&currline, &currlinelen, currincl)) > 0)
	{
		fprintf(OUTSTREAM, "%s\n",  currline);
	}
	fclose(currincl); currincl = NULL;

	BACKTRACK(SINCLUDE_DONE);
}

static void
m4_include(void)
{
	SET_JMP(ID_INCLUDE);

	if (STATE.argc < SINCL_LEAST_ARGC)
		REJECT(ERRID_INCLUDE, NO_ARGC);
			
	char *file = ARGV_nth(1);
	currincl = fopen(file, "r"); !currincl 
					? REJECT(ERRID_IO, errno)
					: None;
	while ((getline(&currline, &currlinelen, currincl)) > 0)
	{
		fprintf(OUTSTREAM, "%s\n", currline);
	}
	fclose(currincl); currincl = NULL;

	BACKTRACK(INCLUDE_DONE);
}

static void
m4_substr(void)
{
	SET_JMP(ID_SUBSTR);

	if (STATE.argc < SUBSTR_LEAST_ARGC)
		REJECT(ERRID_SUBSTR, NO_ARGC);

	uint8_t *string = ARGV_nth(1);
	ssize_t offset = atol(ARGV_nth(2)), numchr = STATE.argc == 3
							? atol(STATE.argv[3])
							: strlen(string);
	string = &string[offset]; string[numchr - 1] = '\0';
	fprintf(OUTSTREAM, string);

	BACKTRACK(SUBSTR_DONE);
}

static void
m4_syscmd(void)
{
	SET_JMP(ID_SYSCMD);

	if (STATE.argc < SYSCMD_LEAST_ARGC)
		REJECT(ERRID_SYSCMD, NO_ARGC);

	uint8_t *cmd = ARGV_nth(1); shexcode = system(cmd);

	BACKTRACK(SYSCMD_DONE);
}

static void
m4_sysval(void)
{
	SET_JMP(ID_SYSVAL);

	fprintf(OUTSTREAM, "%ld", shexcode);

	BACKTRACK(SYSVAL_DONE);
}


static void
m4_translit(void)
{
	SET_JMP(ID_TRANSLIT);

	if (STATE.argc < TRANSLIT_LEAST_ARGC)
		REJECT(ERRID_TRANSLIT, NO_ARGC);

	uint8_t *corp = ARGV_nth(1),	\
		*this = ARGV_nth(2),	\
		*with = STATE.argv[3], *rem, lchr;
	size_t  lencorp = STATE.argvlen[1],	\
		lenthis = STATE.argvlen[2],	\
	      	lenwith = STATE.argvlen[3];

	lenwith < lencorp
		? (corp[lenwith - 1] = '\0', lencorp = lenwith)
		: (lenthis < lencorp
			? (rem = strndupa(corp, lencorp), 
				rem[lenthis - 1] = '\0',
				corp = &corp[lenthis - 1], 
				lencorp = lenthis)
			: (with[lencorp - 1] = '\0', this[lencorp - 1] = '\0'),
		  );

	uint8_t TRANTBL[UINT8_MAX];
	while ((lchr = *this++)) 
		TRANTBL[lchr] = *with++;
	for (int i = 0; i < lencorp; i++)
		corp[i] = TRANTBL[corp[i]];
	if (rem) fputs(rem, OUTSTREAM);
	fprintf(OUTSTREAM, "%s", corp);
	
	BACKTRACK(TRANSLIT_DONE);
}


static void
m4_errprint(void)
{
	SET_JMP(ID_ERRPRINT);

	if (STATE.argc < ERRPRINT_LEAST_ARGC)
		REJECT(ERRID_ERRPRINT, NO_ARGC);

	uint8_t *errmsg = ARGV_nth(1);
	fprintf(PRISTREAM,"%s", errmsg);

	BACKTRACK(ERRPRINT_DONE);
}

static void
m4_define(void)
{
	SET_JMP(ID_DEFINE);

	if (STATE.argc < DEFINE_LEAST_ARGC)
		REJECT(ERRID_DEFINE, NO_ARGC);

	uint8_t *name = ARGV_nth(1);
	uint8_t *defn = ARGV_nth(2);

	check_pool_capacity();
	umacro_t *new_def = define_pool(name, defn);

	if (!gl_hash_nx_getput(SYMTABLE, (void*)name, (void*)new_def))
		error_out(ERRMSG_SYMTABLE_ERR);

	BACKTRACK(DEFINE_DONE);
}

static void
m4_undefine(void)
{
	SET_JMP(ID_UNDEFINE);

	if (STATE.argc < UNDEFINE_LEAST_ARGC)
		REJECT(ERRID_UNDEFINE, NO_ARGC);

	uint8_t *name = ARGV_nth(1);

	gl_hash_nx_getremove(SYMTABLE, name, NULL);

	BACKTRACK(UNDEFINE_DONE);
}

static void
m4_defn(void)
{
	SET_JMP(ID_DEFN);

	if (STATE.argc < DEFN_LEAST_ARGC)
		REJECT(ERRID_DEFN, NO_ARGC);

	uint8_t *name = ARGV_nth(1);
	uint8_t *defn = ARGV_nth(2);

	uint8_t qdefn[strlen(defn) + strlen(&lquote[0]) + strlen(&rquote[0])];

	memmove(&qdefn[0], &lquote[0], strlen(&lquote[0]));
	memmove(&qdefn[0], &defn[0], strlen(&defn[0]));
	memmove(&qdefn[0], &rquote[0], strlen(&rquote[0]));

	call_builtin(ID_DEFINE, 2, BUILTIN_NAMES[ID_DEFINE], name, defn);
}

static void
m4_pushdef(void)
{
	SET_JMP(ID_PUSHDEF);

	if (STATE.argc < PUSHDEF_LEAST_ARGC)
		REJECT(ERRID_PUSHDEF, NO_ARGC);

	uint8_t *name = ARGV_nth(1);
	uint8_t *defn = ARGV_nth(2);

	check_pool_capacity();
	umacro_t *newdef = define_pool(name, defn);

	stack_push(DEFSTACK, new_def);

	BACKTRACK(PUSHDEF_DONE);
}

static void
m4_popdef(void)
{
	SET_JMP(ID_POPDEF);

	if (STATE.argc < POPDEF_LEAST_ARGC)
		REJECT(ERRID_POPDEF, NO_ARGC);

	umacro_t *lastdef = stack_pop(DEFSTACK);
	undefine_pool(lastdef);

	BACKTRACK(POPDEF_DONE);
}

static void
m4_traceon(void)
{
	SET_JMP(ID_TRACEON);

	STATE.traceon = true;
}


static void
m4_traceoff(void)
{
	SET_JMP(ID_TRACEOFF);

	STATE.traceoff = false;
}

static void
call_builtin(int id, size_t nargs, ...)
{
	if (nargs > MAX_BUILTIN_ARGV)
		error_out(ERRMSG_MAX_BUILTIN_ARGV);

	va_list argls;
	va_start(argls, nargs);
		
	
	clear_state();
	STATE.builtin_id = id;
	STATE.argc = nargs;

	size_t arglen, nnargs = 0;
	uint8_t *arg;

	while (--nargs)
	{
		arg = va_arg(ap, uint8_t*);
		arglen = strlen(arg);
		if (arglen > MAX_ARGV_LEN)
			error_out(ERRMSG_LONGARG);
		memmove(&STATE.argv[nnargs], arg);
		memmove(&STATE.argvlen[nnargs++], arglen);
	}

	eval_state();
	trace_calls();
}

static inline void
clear_state(void) { memset(&STATE, 0, sizeof(struct State)); }

static inline void
eval_state(void) { longjmp(BUILTIN_JBUF[STATE.builtin_id], true); }

static inline void
check_pool_capacity(void)
{
	if (((POOL.allocnum + 1) % POOL_ALLOC_STEP) == 0) {
		void *newptr = realloc(
			POOL.macros,
			(POOL.allocnum + POOL_ALLOC_STEP) * sizeof(uintptr_t));
		if (!newptr)
			error_out(ERRMSG_POOL_REALLOC);
		POOL.macros = (umacro_t**)newptr;
	}

}

static inline void
init_pool(void)
{
	void *newptr = calloc(POOL.macros, POOL_ALLOC_STEP * sizeof(uintptr_t));
	if (!newptr)
		error_out(ERRMSG_POOL_ALLOC);
	POOL.macros = (umacro_t**)newptr;
}

static inline void
kill_pool(void) {  free(POOL.macros);   }

static inline void
trace_calls(void)
{
	if (!STATE.traceon) return;

	fputs(PRISTREAM, M4_TRACE_QUIP);
	fprintf(PRISTREAM, " -%lu- ", STATE.argc);
	fprintf(PRISTREAM, "%s -> %s%s%s\n", 
			&STATE.argv[0],
			&STATE.lquote[0],
			&STATE.lastexpand[0],
			&STATE.rquote[0],
		);
	
}

static void
stream_walker(void)
{
	uint8_t *current_line;
	size_t line_length;
	while (getline(&current_line, &line_length, INSTREAM) > 0)
	{
			
	
	}
}

static void
trapped_macro(void)
{
	
}
