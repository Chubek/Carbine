#include "_eval.yy.c"

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
               
	uint8_t *error_message = ARGV_nth(1);
	OUTPUT_FMT(PRISTREAM,"%s", error_message);

	BACKTRACK(ERRPRINT_DONE);
}

static void
m4_define(void)
{
	SET_JMP(ID_DEFINE);

	uint8_t *macro_name = ARGV_nth(1);
	uint8_t *macro_definition = ARGV_nth(2);

	if (!SYMTABLE_INSERT(macro_name, macro_definition))
		// todo error

	BACKTRACK(DEFINE_DONE);
}

static void
m4_undefine(void)
{
	SET_JMP(ID_UNDEFINE);

	uint8_t *macro_name = ARGV_nth(1);

	if (!SYMTABLE_DELETE(macro_name))
		// todo error

	BACKTRACK(UNDEFINE_DONE);
}

static void
m4_defn(void)
{
	SET_JMP(ID_DEFN);

	uint8_t *text = ARGV_nth(1);
	truncate
	DEFN_strlen = u8_strlen(text);
	OUTPUT_FMT(DEFN_stream, "%s%s%s", LQUOTE, text, RQUOTE);

	BACKTRACK(DEFN_DONE);
}

static void
m4_pushdef(void)
{
	SET_JMP(ID_PUSHDEF);

	if (STATE.argc < PUSHDEF_LEAST_ARGC)
		REJECT(ERRID_PUSHDEF, NO_ARGC);

	uint8_t *macro_name = ARGV_nth(1);
	uint8_t *macro_definition = ARGV_nth(2);
	
	if (!SYMTABLE_INSERT(macro_name, macro_definition))
		// todo error
	if (!SYMSTACK_PUSHDEF(macro_name))
		// todo error

	BACKTRACK(PUSHDEF_DONE);
}

static void
m4_popdef(void)
{
	SET_JMP(ID_POPDEF);
	
	uint8_t *defstack_top;
        if (!(defstack_top = SYMSTACK_POP()))
		//todo error
	if (!SYMTABLE_DELETE(defstack_top))
		//todo error
	
	BACKTRACK(POPDEF_DONE);
}

static void
m4_dumpdef(void)
{
	SET_JMP(iD_DUMPDEF);

	uint8_t *req_name;
	uint8_t *req_definition;
	size_t num_req = ARGC;

	while (--num_req)
	{
		req_name = ARGV_nth(num_req);
		req_definition = SYMTABLE_LOOKUP(req_name);
		OUTPUT_FMT(
				PRISTREAM, 
				"%s%s%s -> %s%s%s\n",
				LQUOTE, req_name, RQUOTE,
				LQUOTE, req_definition, RQUOTE
			);	
	}

	BACKTRACK(DUMPDEF_DONE);
}

static void
m4_traceon(void)
{
	SET_JMP(ID_TRACEON);
	
	TRACE(ON);

	BACKTRACK(TRACEON_DONE);
}


static void
m4_traceoff(void)
{
	SET_JMP(ID_TRACEOFF);

	TRACE(OFF);

	BACKTRACK(TRACEOFF_DONE);
}

