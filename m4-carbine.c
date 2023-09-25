#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <string.h>
#include <limits.h>
#include <unistd.h>
#include <search.h>
#include <setjmp.h>
#include <regexp.h>
#include <errno.h>
#include <sys/types.h>
#include <hs.h>


#include "_eval.yy.c"

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
	size_t    argc;
	char 	  outfile[FILENAME_MAX], infile[FILENAME_MAX];
}  
STATE;

static
jmp_buf	jbacktrack, jbuf, jif, ERROR_JMP[NUM_ERROR];


static 
ENTRY *currsym;

static 
FILE  *finp, *foutp, *fpri, *foutphold,  		\
      *currincl, *nulldivert, *wrapspace,			\
      *DIVERTS[DIVERT_NUM], *currdivert;

static 
uint8_t lquote[SYNT_NUM], rquote[SYNT_NUM], comment[SYNT_NUM], \
	*wrapstr, DIVERT_STRS[DIVERT_NUM]	       \
	*focalsym, *ifdef, *ifndef;

static
struct IfEqElse
{
	uint8_t *lhs, *rhs, *ifeq, *ifne;
}
IFEQ_STACK[IFEQ_MAX];

static 
int64_t shexcode, divnum, defntop, stacktop;


static 
size_t ifeqtop, DIVERT_LENS[DIVERT_NUM];

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
		longjmp(ERR_JMP[ERRID_DIVERT], NO_NBUF);
	
	int8_t yynbuf = atoi(nbuf);
	
	if (yynbuf > DIVERT_NUM) 
		longjmp(ERROR_JMP[ERRID_DIVERT], DIVERT_INSUFF);
	else if (yynbuf < 0) 
	{
		foutphold = foutp; foutp = nulldivert;
	}
	else
	{
		foutphold = foutp; foutp = DIVERTS[yynbuf]; ++divnum;
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
	
       	foutp = foutphold; --divnum;
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
		longjmp(ERROR_JMP[ERRID_IFDEF], NO_IFSYM);
	else if (!(*ifdef))
		longjmp(ERROR_JMP[ERRID_IFDEF], NO_DEFSYM);

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
			longjmp(ERROR_JMP[ERRID_IFELSE], ARGC_INSUFF);
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

	if (STATE.argc < 2)
		longjmp(ERROR_JMP[ERRID_INDEX], NO_CHAR);

	uint8_t *haystack = &STATE.argv[1], *needle = &STATE.argv[2];
	size_t idx = strspn(haystack, needle);
	
	fprintf(foutp, "%lu", idx);

	BACKTRACK(INDEX_DONE);
}

static void
m4_len(void)
{
	SET_JMP(ID_LEN);

	if (STATE.argc < 1)
		longjmp(ERROR_JMP[ERRID_LEN], NO_STR);

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
	if (STATE.argc < 1)
		temp = DEFAULT_TMP_TEMP;
	else
		temp = &STATE.argv[1];
	if ((fdesc = mkstemp(temp)) < 0)
		longjmp(ERROR_JMP[ERRID_MKSTEMP], errno);
	else
		close(fdesc);

	BACKTRACK(MKSTEMP_DONE);
}

