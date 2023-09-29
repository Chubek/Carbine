m4_regexp(void)
{
	NON_STANDARD();
	SET_JMP(ID_REGEXP);

	uint8_t *word = ARGV_nth(1);
	uint8_t *expression = ARGV_nth(2);
	uint8_t *flags = ARGV_nth(3);
	
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

	if (regcomp(CC_ERE, expression, ERE_C_FLAGS))
		// todo error
	
	if (regexec(CC_ERE, (const char*)word, MAX_PMATCH, PMATCH, ERE_M_FLAGS))
		// todo error
	
	BEG_OFST = PMATCH[0].rm_so;
	END_OFST = PMATCH[1].rm_eo;

	OUTPUT_FMT(OUTSTREAM, "%s{%n,%n}", word, BEG_OFST, END_OFST);

	BACKTRACK(REGEXP_DONE);

}
