m4_posixregxp(void)
{
	NON_STANDARD();
	SET_JMP(ID_POSIXREGEXP);

	uint8_t *word = ARGV_nth(1);
	uint8_t *expression = ARGV_nth(2);
	uint8_t *compile_flags = ARGV_nth(3);
	uint8_t *match_flags = ARGV[4];

	int cflags, mflags;

	cflags = !compile_flags
			? REG_EXTENDED
			: strto
}
