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
USER_DEFN[MAX_USER_DEFN];

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
jmp_buf	 jbuf, jif, ERROR_JMP[NUM_ERROR];


static 
ENTRY *currsym;

static 
FILE  *finp, *foutp, *fpri, *foutphold, *wrapspace,  		\
      *currincl, *nulldivert, *defstack,			\
      *DIVERTS[DIVERT_NUM], *currdivert;

static 
uint8_t lquote[SYNT_NUM], rquote[SYNT_NUM], comment[SYNT_NUM], \
	*wrapstr, *defstackstr, DIVERT_STRS[DIVERT_NUM]	       \
	*focalsym, *ifdef, *ifndef;

static
struct IfEqElse
{
	uint8_t *lhs, *rhs, *ifeq, *ifne;
}
IFEQ_STACK[IFEQ_MAX];

static 
int64_t shexcode, divnum, defntop;

static 
size_t defstacklen, ifeqtop, DIVERT_LENS[DIVERT_NUM];

#define SET_JMP(ID) int jres = setjmp(BUILTINS[ID].jbuf); 	\
			!jres ? longjmp(BUILTINS[ID].jdfl, ID) : jres;

#define ENTRY_FIND(ENT) hsearch((ENTRY){ .key = ENT }, FIND)

static void
m4_incr(void)
{
	SET_JMP(ID_INCR);

	uint8_t *num = &STATE.argv[1];
	int64_t inum = strtoll(num, NULL, 10);
	fprintf(foutp, "%ld\n", ++inum);
}

static void
m4_decr(void)
{
	SET_JMP(ID_DECR);

	uint8_t *num = &STATE.argv[1];
	int64_t inum = strtoll(num, NULL, 10);
	fprintf(foutp, "%ld\n", --inum);
}


static void
m4_m4wrap(void)
{	
	SET_JMP(ID_M4WRAP);
	
	uint8_t wraptext = &STATE.argv[1];
	fputs(wraptext, wrapspace);
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
}


static void
m4_divnum(void)
{
	SET_JMP(ID_DIVNUM);

	fprintf(foutp, "%ld", divnum);
}


static void
m4_dnl(void)
{
	SET_JMP(ID_DNL);

	while (fgetc(finp) != '\n'); fgetc(finp);
}




static void
m4_eval(void)
{	
	SET_JMP(ID_EVAL);
	
	yyfinal = 0; eval_yy_init(&STATE.argv[1]); eval_yy_parse();
	
	printf(foutp, "%ld\n", yyfinal);

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
		? longjmp(jif, IF_DEF)
	        : longjmp(jif, IF_NDEF);	
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
			longjmp(jifelse, IFELSE_DONE);
			break;
	}
}


static void
m4_index(void)
{
	SET_JMP(ID_INDEX);


}
