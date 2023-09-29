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
		FILE *current_include_file;
		uint8_t *current_include_line;
		size_t current_include_linelen;
		int shell_exit_code;
		uint32_t *translit_map;
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
#define ARGV_nth(n)	&STATE.INVOKATION.ARGV[n][0]
#define ARGV_star	&STATE.INVOKATION.argv_comma_joined[0]
#define ARGV_atsign	&STATE.INVOKATION.argv_space_joined[0]
#define ARGVLEN_name	STATE.INVOKATION.ARGVLEN[0]
#define ARGVLEN_nth(n)  sTATE.INVOKATIOn.ARGVLEN[n]	

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
	DIVERTNUM = strtoll(buff_num, NULL, BASE_DECIMAL);
	
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
	UNDIVERTNUM = strtoll(buff_num, NULL, BASE_DECIMAL);
	
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

	fscanf(INSTREAM, "%*s\n", NULL);

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
	size_t ifdef_len = ARGVLEN_nth(2);
	size_t ifndef_len = ARGVLEN_nth(3);

	SYMBOL_EXISTS(focal_symbol) 
		? OUTPUT_FMT(OUTSTREAM, "%e%n%q", ifdef_len, ifdef)
		: OUTPUT_FMT(OUTSTREAM, "%e%n%q", ifndef_len, ifndef);

	BACKTRACK(IFDEF_DONE);
}

static void
m4_ifelse(void)
{
	SET_JMP(ID_IFELSE);

	if (ARGC == 3) 
		goto case_three_args;
	else if (ARGC == 4 || ARGC == 5)
		goto case_four_or_five_args;
	else if (ARGC == 6)
		goto case_six_args;
	else
		// todo error
case_three_args:
	uint8_t *compare_a = ARGV_nth(1);
	uint8_t *compare_b = ARGV_nth(2);
	uint8_t *print_ifeq = ARGV_nth(3);

	!u8_strcmp(compare_a, compare_b)
		? OUTPUT_FMT(OUTSTREAM, "%s", print_ifeq)
		: OUTPUT_FMT(OUTSTREAM, NULL, NULL);
       goto done;

case_four_or_five_args:
	uint8_t *compare_a = ARGV_nth(1);
	uint8_t *compare_b = ARGV_nth(2);
	uint8_t *print_ifeq = ARGV_nth(3);
	uint8_t *print_ifne = ARGV_nth(4);

	!u8_strcmp(compare_a, compare_b)
		? OUTPUT_FMT(OUTSTREAM, "%s", print_ifeq)
		: OUTPUT_FMT(OUTSTREAM, "%s", print_ifne);
	goto done;

case_six_args:
	uint8_t *compare_a = ARGV_nth(4);
	uint8_t *compare_b = ARGV_nth(5);
	uint8_t *print_ifeq = ARGV_nth(6);

	!u8_strcmp(compare_a, compare_b)
		? OUTPUT_FMT(OUTSTREAM, "%s", print_ifeq)
		: OUTPUT_FMT(OUTSTREAM, NULL, NULL);
	goto done;

done:
	BACKTRACK(IFELSE_DONE);
}



static void
m4_index(void)
{
	SET_JMP(ID_INDEX);


	uint8_t *haystack = ARGV_nth(1);
	uint8_t	*needle = ARGV_nth(2);
	size_t substr_index = u8_strspn(haystack, needle);
	
	OUTPUT_FMT(OUTSTREAM, "%i", idx);

	BACKTRACK(INDEX_DONE);
}

static void
m4_len(void)
{
	SET_JMP(ID_LEN);

	uint8_t *subject = ARGV_nth(1);
	size_t subject_len = u8_strlen(subject);
	OUTPUT_FMT(OUTSTREAM, "%i", subject_len);
	
	BACKTRACK(LEN_DONE);
}

static void
m4_mkstemp(void)
{
	SET_JMP(ID_MKSTEMP);

	char *template = ARGV_nth(1);
	int fdesc;
	
	if ((fdesc = mkstemp(template)) < 0)
		// todo error
	close(fdesc);
	OUTPUT_FMT(OUTSTREAM, "%s", template);

	BACKTRACK(MKSTEMP_DONE);
}

static void
m4_shift(void)
{
	SET_JMP(ID_SHIFT);
	
	uint8_t *current_argv;
	size_t shift_index = SHIFT_INDEX_INIT;
	while (++shift_index < ARGC) {
		current_argv = ARGV_nth(shift_index);
		OUTPUT_FMT(OUTSTREAM, "%s,", current_argv);
	}
	
	BACKTRACK(SHIFT_DONE);
}

static void
m4_sinclude(void)
{
	SET_JMP(ID_SINCLUDE);

	uint8_t *include_file_path = ARGV_nth(1);
	current_include_file = fopen(include_file_path, "r");
	
	if (!current_include_file)
		// todo jump back
	
	ssize_t read_result;
	while ((read_result = getline(
			INCLINE_STR, 
			INCLINE_LEN, 
			current_include_file
		)) > 0) {   OUTPUT_FMT(
				OUTSTREAM, 
				"%^%n", 
				INCLINE_STR,
				INCLINE_LEN,
			);}

	BACKTRACK(SINCLUDE_DONE);
}

static void
m4_include(void)
{
	SET_JMP(ID_INCLUDE);

	uint8_t *include_file_path = ARGV_nth(1);
	current_include_file = fopen(include_file_path, "r");
	
	if (!current_include_file)
		// todo error
	
	ssize_t read_result;
	while ((read_result = getline(
			INCLINE_STR, 
			INCLINE_LEN, 
			current_include_file
		)) > 0) {   OUTPUT_FMT(
				OUTSTREAM, 
				"%^%n", 
				INCLINE_STR,
				INCLINE_LEN,
			);}


	BACKTRACK(INCLUDE_DONE);
}

static void
m4_substr(void)
{
	SET_JMP(ID_SUBSTR);

	uint8_t *string = ARGV_nth(1);
	ssize_t sub_offset = strtoll(ARGV_nth(2), NULL, BASE_DECIMAL); 
	ssize_t sub_extent = STATE.argc == 3
			? strtoll(ARGV_nth(3), NULL, BASE_DECIMAL)
			: u8_strlen(string);

	OUTPUT_FMT(
			OUTSTREAM, 
			"%&%s%n%n", 
			sub_offset, 
			sub_extent, 
			string
		);

	BACKTRACK(SUBSTR_DONE);
}

static void
m4_syscmd(void)
{
	SET_JMP(ID_SYSCMD);

	uint8_t *shell_cmd = ARGV_nth(1); 
	SHELLEXCODE = system(shell_cmd);

	BACKTRACK(SYSCMD_DONE);
}

static void
m4_sysval(void)
{
	SET_JMP(ID_SYSVAL);

	OUTPUT_FMT(OUTSTREAM, "%i", SHELLEXCODE);

	BACKTRACK(SYSVAL_DONE);
}


static void
m4_translit(void)
{
	SET_JMP(ID_TRANSLIT);
	
	uint32_t *text_biguni = u8_to_u32(ARGV_nth(1));
	uint32_t *map_key = u8_to_u32(ARGV_nth(2));
	uint32_t *map_val = u8_to_u32(ARGV_nth(3));
	size_t text_len = ARGVLEN_nth(1);
	size_t key_len = ARGVLEN_nth(2);
	size_t val_len = ARGVLEN_nth(3);
	size_t index = MINIMUM(key_len, val_len);
	
	map_key[index - 1] =
		map_val[index - 1] = U'\0';
	
	memset(TRANSLIT_MAP, 0, sizeof(uint32_t) * MAX_TRANSLIT_SIZE);

	while (--index)
		TRANSLIT_MAP[IDX_KEY] = IDX_VAL;

	while (--text_len)
		IDX_TXT = TRANSLIT_MAP[IDX_TXT];
		
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
	fprintf(PRISTREAM, " -%i- ", STATE.argc);
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
