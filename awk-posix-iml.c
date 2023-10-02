awk_atan2(awk_numeric_t y, awk_numeric_t x)
{
	return atan2l((long double)y, (long double)x);
}

awk_cos(awk_numeric_t x)
{
	return cosl((long double)x);	
}

awk_sin(awk_numeric_t x)
{
	return sinl((long double)x);
}

awk_exp(awk_numeric_t x)
{
	return expl((long double)x);
}

awk_log(awk_numeric_t x)
{
	return logl((long double)x);
}

awk_sqrt(awk_numeric_t x)
{
	return sqrtl((long double)x);
}


awk_int(awk_numeric_t x)
{
	return (intmax_t)((long double)x);
}

awk_rand(void)
{
	return rand();
}

awk_srand(awk_numeric_t seed)
{
	return srand((uintmax_t)seed);
}


