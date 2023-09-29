%{
#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>

#define YYPARSE eval_yy_parse

#define YY_INPUT(buf, result, maxsize)			\
{							\
	int yyc = exprbuf[exprii++];			\
	result = yyc  ? (*(buf) = yyc, 1) : 0;		\
}

int8_t exprbuf[1024] = { 0 };
size_t exprii = 0;
static int64_t yyfinal;
%}


Expr = LPAREN? (i:Logical* | . ) RPAREN? EOI  { yyfinal = i; }


Logical = l:Bitwise
		( - '&&'  -  r:Bitwise { l = l && r; }
		 | - '||' - r:Bitwise { l = l || r; }
		)*               { $$ =  l;     }

Bitwise = l:Equalal 
		( - '&' - r:Equalal  { l &= r; }
		 | - '|' - r:Equalal  { l |= r; }
		 | - '^' - r:Equalal  { l ^= r; }
		)*		 { $$ =  l;    }
		  

Equalal = l:Relal
		( - '==' - r:Relal  { l  = l == r; }
	         | - '!=' - r:Relal  { l  = l != r; }
		)*		{ $$ =  l;     }


Relal = l:Shiftive
		(  -'<' - r:Shiftive {  l  = l < r; }
		 | - '>' - r:Shiftive { l  = l > r; }
		 | - '>=' - r:Shiftive { l  = l >= r; }
		 | - '<=' - r:Shiftive { l  = l <= r; }
		 )*                { $$ =  l;     }

Shiftive = l:Additive
		(  - '>>' - r:Additive  { l >>= r; }
		  | -  '<<' - r:Additive {  l <<= r; }
		)*		{ $$ =  l; }

Additive   = l:Multive
		(  - '+' - r:Multive  { l += r; }
		 | - '-' - r:Multive  { l -= r; }
		)*		 { $$ =  l; }


Multive = l:Unary
		( - '*' - r:Unary	{ l *= r;  }
		 | - '/' - r:Unary  { l /= r;  }
		)*		{ $$ =  l;  }

Unary  = i:Value	  { $$ =  i; }
	   | '++' i:Value { $$ =  i + 1; }
	   | '--' i:Value { $$ =  i - 1; }
	   | '-'  i:Value { $$ =  -1 * i; }
	   | '!' i:Value  { $$ =  i & 0; }
	   | '~' i:Value  { $$ = i ^ i; }

Value = - i:INT -			{ $$ = i; }
	| - f:FLT -			{ $$ = f; }
	| LPAREN res:Expr RPAREN	{ $$ = res; }

FLT = - < ("-"|"+")?[0-9]*("."|"e"|"E")[0-9]+ > - { $$ = strtold(yytext, NULL);;  } 
INT = - < ("-"|"+")?[1-9][0-9]* > -   {  $$ = strtoll(yytext, NULL, 10); }
	| - < ("0x"|"0X")[a-fA-F0-9]+ > - { $$ = strtoll(yytext, NULL, 16);  }
	| - < ("0o"|"0O")[0-7]+ > - { $$ = strtoll(yytext, NULL, 8); }
	| - < ("0b"|"0B")[0-1]+ > - { $$ = strtoll(yytext, NULL, 2); } 

RPAREN = - ')' -
LPAREN =  - '(' -
-   = [ \t]*
EOI = ";"


%%


static inline void
eval_yy_init(const int8_t *yystr)
{
	FILE *strbuf = fmemopen(&exprbuf[0], 1024, "w"); 
	ftruncate(fileno(strbuf), 0);
	fputs(yystr, strbuf); fputc(';', strbuf); fflush(strbuf);
	fclose(strbuf);
}


