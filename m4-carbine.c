#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <string.h>
#include <limits.h>
#include <unistd.h>
#include <setjmp.h>
#include <regexp.h>
#include <errno.h>
#include <sys/types.h>
#include <signal.h>

#include "_eval.yy.c"
#include "gl_hash_map.h"
#include "stack.h"

#ifndef DEV_NULL
#define DEV_NULL "/dev/null"
#endif

static 
struct BuiltinData
{ 
	uint8_t name[NAMELEN_MAX];
	size_t nargs;
	jmp_buf jbuf, jdfl;
} 
BUILTINS[BUILTIN_NUM];

static 
struct UserDefined
{
	uint8_t name[NAMELEN_MAX];
	uint8_t defn[MAX_DEFN];
}
USER_DEFN[MAX_USER_DEFN], DEFN_STACK[MAX_DEFN_STACK];

static 
struct State
{
	uint8_t   name[NAMELEN_MAX];
	uint8_t   argv[ARGVLEN_MAX][ARGV_MAX];
	size_t    argc, argvlen[ARGV_MAX];
	char 	  outfile[FILENAME_MAX], infile[FILENAME_MAX];
}  
STATE;

static
jmp_buf	jbacktrack, jbuf, jif, ERROR_JMP[NUM_ERROR];

static
struct ohash *SYMTABLE;
Stack_T defstack;

static 
FILE  *finp, *foutp, *fpri, *foutphold,  		\
      *currincl, *nulldivert, *wrapspace,			\
      *DIVERTS[DIVERT_NUM], *currdivert;

static 
uint8_t lquote[SYNT_NUM], rquote[SYNT_NUM], comment[SYNT_NUM], \
	*wrapstr, DIVERT_STRS[DIVERT_NUM]	       \
	*focalsym, *ifdef, *ifndef, *currline;

static
struct IfEqElse
{
	uint8_t *lhs, *rhs, *ifeq, *ifne;
}
IFEQ_STACK[IFEQ_MAX];

static 
int64_t shexcode, divnum, defntop, stacktop;

static 
size_t ifeqtop, currlinelen, DIVERT_LENS[DIVERT_NUM];

#define None
#define Some(V) V & 1

#define SET_JMP(ID) int jres = setjmp(BUILTINS[ID].jbuf); 	\
			!jres ? longjmp(BUILTINS[ID].jdfl, ID) : jres;

#define BACKTRACK(MSG) longjmp(jbacktrack, MSG)

#define ENTRY_FIND(ENT) hsearch((ENTRY){ .key = ENT }, FIND)

static void
m4_incr(void)
{
	SET_JMP(ID_INCR);

	uint8_t *num = &STATE.argv[1];
	int64_t inum = strtoll(num, NULL, 10);
	fprintf(foutp, "%ld\n", ++inum);

	BACKTRACK(INCR_DONE);
}
 
static void
m4_decr(void)
{
 	SET_JMP(ID_DECR);

	uint8_t *num = &STATE.argv[1];
	int64_t inum = strtoll(num, NULL, 10);
	fprintf(foutp, "%ld\n", --inum);

	BACKTRACK(DECR_DONE);
}


static void
m4_m4wrap(void)
{	
	SET_JMP(ID_M4WRAP);
	
	uint8_t wraptext = &STATE.argv[1];
	fputs(wraptext, wrapspace);

	BACKTRACK(M4WRAP_DONE);
}


static void
m4_divert(void)
{
	SET_JMP(ID_DIVERT);
	
	char *nbuf = &STATE.argv[1];
	
	if (!(*nbuf))
		REJECT(ERRID_DIVERT, NO_NBUF);
	
	int8_t yynbuf = atoi(nbuf);
	
	if (yynbuf > DIVERT_NUM) 
		REJECT(ERRID_DIVERT, DIVERT_INSUFF);
	else if (yynbuf < 0) 
	{
		foutphold = foutp; foutp = nulldivert;
	}
	else
	{
		foutphold = foutp; foutp = DIVERTS[yynbuf]; 
		divnum = fileno(foutp);
	}
	
	BACKTRACK(DIVERT_DONE);
}


static void
m4_undivert(void)
{
	SET_JMP(ID_UNDIVERT);
	
	char *nbuf = &STATE.argv[1];
	
	if (!(*nbuf))
		longjmp(ERR_JMP[ERRID_UNDIVERT], NO_NBUF);

	int8_t yynbuf = atoi(nbuf);
	
       	foutp = foutphold; divnum = fileno(foutp);
	if (yynbuf < 0) return;

	fputs(DIVERT_STRS[yynbuf], foutp);
	ftruncte(fileno(DIVERTS[yynbuf]), 0);

	BACKTRACK(UNDIVERT_DONE);
}


static void
m4_divnum(void)
{
	SET_JMP(ID_DIVNUM);

	fprintf(foutp, "%ld", divnum);

	BACKTRACK(DIVNUM_DONE);
}


static void
m4_dnl(void)
{
	SET_JMP(ID_DNL);

	while (fgetc(finp) != '\n'); fgetc(finp);

	BACKTRACK(DNL_DONE);
}




static void
m4_eval(void)
{	
	SET_JMP(ID_EVAL);
	
	yyfinal = 0; eval_yy_init(&STATE.argv[1]); eval_yy_parse();
	
	printf(foutp, "%ld\n", yyfinal);

	BACKTRACK(EVAL_DONE);

}




static void
m4_ifdef(void)
{
	SET_JMP(ID_IFDEF);

	focalsym = &STATE.argv[1]; 
	ifdef = &STATE.argv[2]; 
	ifndef = &STATE.argv[3];
	
	if (!(*focalsym))
		REJECT(ERRID_IFDEF, NO_IFSYM);
	else if (!(*ifdef))
		REJECT(ERRID_IFDEF, NO_DEFSYM);

	cursym = ENTRY_FIND(focalsym);
	cursym 	
		? BACKTRACK(IF_DEF)
	        : BACKTRACK(IF_NDEF);	
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

	if (STATE.argc < )
		REJECT(ERRID_INDEX, NO_CHAR);

	uint8_t *haystack = &STATE.argv[1], *needle = &STATE.argv[2];
	size_t idx = strspn(haystack, needle);
	
	fprintf(foutp, "%lu", idx);

	BACKTRACK(INDEX_DONE);
}

static void
m4_len(void)
{
	SET_JMP(ID_LEN);

	if (STATE.argc < LEN_LEAST_ARGC)
		REJECT(ERRID_LEN, NO_STR);

	uint8_t *subject = &STATE.argv[1];
	size_t lensubj = strlen(subject);

	fprintf(foutp, "%lu", lensubj);
	
	BACKTRACK(LEN_DONE);
}

static void
m4_mkstemp(void)
{
	SET_JMP(ID_MKSTEMP);

	char *temp;
	int fdesc;
	if (STATE.argc < MKSTEMP_LEAST_ARGC)
		temp = DEFAULT_TMP_TEMP;
	else
		temp = &STATE.argv[1];
	if ((fdesc = mkstemp(temp)) < 0)
		REJECT(ERRID_MKSTEMP, errno);
	else
		close(fdesc);

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
		fputs(&STATE.argv[shfnum++], foutp);
	
	BACKTRACK(SHIFT_DONE);
}

static void
m4_sinclude(void)
{
	SET_JMP(ID_SINCLUDE);

	if (STATE.argc < SINCL_LEAST_ARGC)
		REJECT(ERRID_SINCLUDE, NO_ARGC);
			
	char *file = &STATE.argv[1];
	currincl = fopen(file, "r"); !currincl 
						? BACKTRACK(SINCL_SILENT_FAIL)
						: None;
	while ((getline(&currline, &currlinelen, currincl)) > 0)
	{
		fputs(currline, foutp);
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
			
	char *file = &STATE.argv[1];
	currincl = fopen(file, "r"); !currincl 
					? REJECT(ERRID_IO, errno)
					: None;
	while ((getline(&currline, &currlinelen, currincl)) > 0)
	{
		fputs(currline, foutp);
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

	uint8_t *string = STATE.argv[1];
	ssize_t offset = atol(STATE.argv[2]), numchr = STATE.argc == 3
							? atol(STATE.argv[3])
							: strlen(string);
	string = &string[offset]; string[numchr - 1] = '\0';
	fputs(string, foutp);

	BACKTRACK(SUBSTR_DONE);
}

static void
m4_syscmd(void)
{
	SET_JMP(ID_SYSCMD);

	if (STATE.argc < SYSCMD_LEAST_ARGC)
		REJECT(ERRID_SYSCMD, NO_ARGC);

	uint8_t *cmd = STATE.argv[1]; shexcode = system(cmd);

	BACKTRACK(SYSCMD_DONE);
}

static void
m4_sysval(void)
{
	SET_JMP(ID_SYSVAL);

	fprintf(foutp, "%ld", shexcode);

	BACKTRACK(SYSVAL_DONE);
}


static void
m4_translit(void)
{
	SET_JMP(ID_TRANSLIT);

	if (STATE.argc < TRANSLIT_LEAST_ARGC)
		REJECT(ERRID_TRANSLIT, NO_ARGC);

	uint8_t *corp = STATE.argv[1],	\
		*this = STATE.argv[2],	\
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
	if (rem) fputs(rem, foutp);
	fputs(corp, foutp);
	
	BACKTRACK(TRANSLIT_DONE);
}

static void
m4_traceon(void)
{
	SET_JMP(ID_TRACEON);

	if (TRACER.ison)
		REJECT(ERRID_TRACEON, TRACER_ALREADY_ON);

	TRACER.procid = fork();
	
}

static void
m4_define(void)
{
	SET_JMP(ID_DEFINE);

	if (STATE.argc < DEFINE_LEAST_ARGC)
		REJECT(ERRID_DEFINE, NO_ARGC);

	uint8_t *name = STATE.argv[1];
	uint8_t *defn = STATE.argv[2];

	ohash_insert(SYMTABLE, );

}
