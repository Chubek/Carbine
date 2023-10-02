static inline awk_numeric_t
awk_atan2(awk_numeric_t y, awk_numeric_t x)
{
	return (awk_numeric_t) atan2l((long double)y, (long double)x);
}

static inline awk_numeric_t
awk_cos(awk_numeric_t x)
{
	return (awk_numeric_t) cosl((long double)x);	
}

static inline awk_numeric_t
awk_sin(awk_numeric_t x)
{
	return (awk_numeric_t) sinl((long double)x);
}

static inline awk_numeric_t
awk_exp(awk_numeric_t x)
{
	return (awk_numeric_t) expl((long double)x);
}

static inline awk_numeric_t
awk_log(awk_numeric_t x)
{
	return (awk_numeric_t) logl((long double)x);
}

static inline awk_numeric_t
awk_sqrt(awk_numeric_t x)
{
	return (awk_numeric_t) sqrtl((long double)x);
}


static inline awk_numeric_t
awk_int(awk_numeric_t x)
{
	return (awk_numeric_t) (intmax_t)((long double)x);
}

static inline awk_numeric_t
awk_rand(void)
{
	return (awk_numeric_t) rand();
}

static inline awk_numeric_t
awk_srand(awk_numeric_t seed)
{
	return (awk_numeric_t) srand((uintmax_t)seed);
}


