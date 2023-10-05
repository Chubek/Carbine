typedef union M4_Token
{
	M4_NEWLINE,
	M4_IDENT,
	M4_BYPASS,
}
M4_Token;

	
static M4_Token
yylex(uint8_t *YYCURSOR)
{
	/*!re2c
		re2c:yyfill:enable = 0;
		re2c:define:YYCTYPE = uint8_t;

		ident = [a-zA-Z_][a-zA-Z_0-9]*;
		
		[\r\n]+		{ return M4_NEWLINE;  }
		ident 		{ return M4_IDENT;    }
		*         	{ return M4_BYPASS;   }
	*/
	
}



