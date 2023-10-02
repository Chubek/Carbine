static void
m4_eval(void)
{
	uint8_t *temp;
	bool engaged;
	for (uint8_t token = strtok(M4_LINE); token; token = strtok(NULL))
	{
		if (sscanf(token, M4_FORMATSTR, &temp) < 0)
	}
}

