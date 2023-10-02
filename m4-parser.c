static void
m4_compile_fmtstr(void)
{
	if (FORCE_PREFIX_ENABLED) 
	{
		if (sprintf(M4_FORMAT_STR, "%%*sm4_%%ms[a-zA-Z0-9](%s", LQUOTE) < 0)
		// todo error
	}
	else
	{
		if (sprintf(M4_FORMAT_STR, "%%*s%%ms[a-zA-Z0-9]%s", LQUOTE) < 0)
			// todo error
	}
}
	
static void
m4_eval(void)
{
	uint8_t *ident;
	bool engaged;
	for (uint8_t token = strtok(M4_LINE); token; token = strtok(NULL))
	{
		if (sscanf(token, M4_FORMAT_STR, &ident) < 0)
			BYPASS_TOKEN(token);
		else
	}
}

