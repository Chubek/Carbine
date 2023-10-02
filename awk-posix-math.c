typedef union AwkNumeric
{
	int64_t i;
	long double f;
}
awknum_t;
	
static inline awknum_t
awk_atan2(awknum_t y, awknum_t x)
{
	awknum_t res; res.f = atan2l(y.f, x.f); return res;
}

static inline awknum_t
awk_cos(awknum_t x)
{
	awknum_t res; res.f = cosl(x.f); return res;	
}

static inline awknum_t
awk_sin(awknum_t x)
{
	awknum_t res; res.f = sinl(x.f); return res;
}

static inline awknum_t
awk_exp(awknum_t x)
{
	awknum_t res; res.f = expl(x.f); return res;
}

static inline awknum_t
awk_log(awknum_t x)
{
	awknum_t res; res.f = logl(x.f); return res;
}

static inline awknum_t
awk_sqrt(awknum_t x)
{
	awknum_t res; res.f = sqrtl(x.f); return res;
}


static inline awknum_t
awk_int(awknum_t x)
{
	awknum_t res; res.i = (intmax_t)(x.f); return res;
}

static inline awknum_t
awk_rand(void)
{
	awknum_t res; res.i = rand(); return res;
}

static inline awknum_t
awk_srand(awknum_t seed)
{
	awknum_t res; res.i = srand((uintmax_t)seed); return res;
}


