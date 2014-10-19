// the source code is from Minix3: lib/libc/gen/modf_ieee754.c

typedef	unsigned int	u_int;

typedef	unsigned long long int __uint64_t;
#define uint64_t __uint64_t
typedef	uint64_t u_int64_t;

#define _LITTLE_ENDIAN 1234	/* LSB first: i386, vax */
#define _BIG_ENDIAN 4321	/* MSB first: 68000, ibm, net */
#define _PDP_ENDIAN 3412	/* LSB first in word, MSW first in long */

#define _BYTE_ORDER _LITTLE_ENDIAN

#define DBL_EXP_INFNAN 2047
#define DBL_EXP_BIAS 1023

#define DBL_EXPBITS 11
#define DBL_FRACHBITS 20
#define DBL_FRACLBITS 32
#define DBL_FRACBITS (DBL_FRACHBITS + DBL_FRACLBITS)

#define SNG_EXPBITS 8
#define SNG_FRACBITS 23

struct ieee_single {
#if _BYTE_ORDER == _BIG_ENDIAN
	u_int	sng_sign:1;
	u_int	sng_exp:SNG_EXPBITS;
	u_int	sng_frac:SNG_FRACBITS;
#else
	u_int	sng_frac:SNG_FRACBITS;
	u_int	sng_exp:SNG_EXPBITS;
	u_int	sng_sign:1;
#endif
};

struct ieee_double {
#if _BYTE_ORDER == _BIG_ENDIAN
	u_int	dbl_sign:1;
	u_int	dbl_exp:DBL_EXPBITS;
	u_int	dbl_frach:DBL_FRACHBITS;
	u_int	dbl_fracl:DBL_FRACLBITS;
#else
	u_int	dbl_fracl:DBL_FRACLBITS;
	u_int	dbl_frach:DBL_FRACHBITS;
	u_int	dbl_exp:DBL_EXPBITS;
	u_int	dbl_sign:1;
#endif
};

union ieee_double_u {
	double			dblu_d;
	struct ieee_double	dblu_dbl;
};

/*
 * double modf(double val, double *iptr)
 * returns: f and i such that |f| < 1.0, (f + i) = val, and
 *	sign(f) == sign(i) == sign(val).
 *
 * Beware signedness when doing subtraction, and also operand size!
 */
double modf(double val, double *iptr)
{
	union ieee_double_u u, v;
	u_int64_t frac;

	/*
	 * If input is +/-Inf or NaN, return +/-0 or NaN.
	 */
	u.dblu_d = val;
	if (u.dblu_dbl.dbl_exp == DBL_EXP_INFNAN) {
		*iptr = u.dblu_d;
		return (0.0 / u.dblu_d);
	}

	/*
	 * If input can't have a fractional part, return
	 * (appropriately signed) zero, and make i be the input.
	 */
	if ((int)u.dblu_dbl.dbl_exp - DBL_EXP_BIAS > DBL_FRACBITS - 1) {
		*iptr = u.dblu_d;
		v.dblu_d = 0.0;
		v.dblu_dbl.dbl_sign = u.dblu_dbl.dbl_sign;
		return (v.dblu_d);
	}

	/*
	 * If |input| < 1.0, return it, and set i to the appropriately
	 * signed zero.
	 */
	if (u.dblu_dbl.dbl_exp < DBL_EXP_BIAS) {
		v.dblu_d = 0.0;
		v.dblu_dbl.dbl_sign = u.dblu_dbl.dbl_sign;
		*iptr = v.dblu_d;
		return (u.dblu_d);
	}

	/*
	 * There can be a fractional part of the input.
	 * If you look at the math involved for a few seconds, it's
	 * plain to see that the integral part is the input, with the
	 * low (DBL_FRACBITS - (exponent - DBL_EXP_BIAS)) bits zeroed,
	 * the fractional part is the part with the rest of the
	 * bits zeroed.  Just zeroing the high bits to get the
	 * fractional part would yield a fraction in need of
	 * normalization.  Therefore, we take the easy way out, and
	 * just use subtraction to get the fractional part.
	 */
	v.dblu_d = u.dblu_d;
	/* Zero the low bits of the fraction, the sleazy way. */
	frac = ((u_int64_t)v.dblu_dbl.dbl_frach << 32) + v.dblu_dbl.dbl_fracl;
	frac >>= DBL_FRACBITS - (u.dblu_dbl.dbl_exp - DBL_EXP_BIAS);
	frac <<= DBL_FRACBITS - (u.dblu_dbl.dbl_exp - DBL_EXP_BIAS);
	v.dblu_dbl.dbl_fracl = (u_int) (frac & 0xffffffffULL);
	v.dblu_dbl.dbl_frach = (u_int) (frac >> 32);
	*iptr = v.dblu_d;

	u.dblu_d -= v.dblu_d;
	u.dblu_dbl.dbl_sign = v.dblu_dbl.dbl_sign;
	return (u.dblu_d);
}

