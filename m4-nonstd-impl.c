static void
m4_regexp(void)
{
	NON_STANDARD();
	SET_JMP(ID_REGEXP);

	uint8_t *word = ARGV_nth(1);
	uint8_t *expression = ARGV_nth(2);
	uint8_t *flags = ARGV_nth(3);
	uint8_t *api = ARGV_nth(4);

	!u8_strncmp(api, PRCRE, PCRE_LEN)
		? SET_REGEXP_API(PCRE_API)
		: SET_REGEXP_API(ERE_API);
	
	int FLAGS[2];
	struct RegexFlag *re_flag;
	for (
		(uint8_t *token = 
		 strtok(flags, SEP_PIPE), size_t toklen);
		token;
		(token = strtok(NULL, SEP_PIPE), 
		 toklen = u8_strlen(token))
	)
	{
		re_flag = LOOKUP_REFLAG_PERF(token, toklen);
		if (re_flag) {
			FLAGS[re_flag->idx] |= re_flag->value;	
		}
	}

	if (RE_COMPILE(CC_ERE, expression, ERE_C_FLAGS))
		// todo error
	
	if (RE_EXEC(CC_ERE, (const char*)word, MAX_PMATCH, PMATCH, ERE_M_FLAGS))
		// todo error
	
	BEG_OFST = PMATCH[0].rm_so;
	END_OFST = PMATCH[1].rm_eo;

	OUTPUT_FMT(OUTSTREAM, "%s{%n,%n}", word, BEG_OFST, END_OFST);

	RE_FREE(CC_RE);
	REVERT_STANDARD();
	BACKTRACK(REGEXP_DONE);

}

static void
m4_exec(void)
{
	NON_STANDARD();
	SET_JMP(ID_EXEC);

	uint8_t *command = ARGV_nth(1);

	OPEN_PIPE();
	EXEC_PIPE(command);
	OUTPUT_FMT(OUTSTREAM, "%s", LAST_EXEC_STRING);
	CLOSE_PIPE();

	REVERT_STANDARD();
	BACKTRACK(EXEC_DONE);
}

static void
m4_date(void)
{
	NON_STANDARD();
	SET_JMP(ID_DATE);  
	
	uint8_t *format = ARGV_nth(1);

	TIME_EPOCH = time(NULL);
	TIME_BDOWN = localtime(&TIME_EPOCH);

	if (strftime(TIME_FMTBUFF, MAX_TIMEFMTBUFF, format, TIME_BDOWN) < 0)
		// todo error 
	OUTPUT_FMT(OUTSTREAM, "%s", TIME_FMTBUFF);

	REVERT_STANDARD();
	BACKTRACK(DATE_DONE);
}


static void
m4_printf(void)
{
	NON_STANDARD();
	SET_JMP(ID_PRINTF);

	size_t num_args = ARGC - 1;
	uint8_t *format = ARGV_nth(1);
	uint8_t *va_args[num_args];
	for (int i = 2; i < num_args; i++)
		va_args[i] = ARGV_nth(i);

	OUTPUT_FMT(OUTSTREAM, ARGS_ARE_VA, format, va_args, num_args);

	REVERT_STANDARD();
	BACKTRACK(PRINTF_DONE);	
}
