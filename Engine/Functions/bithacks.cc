#include "Functions.h"
#ifdef _WINDOWS
#pragma warning(disable:4554) // precedence warnings
#endif

union Bits
{
	double d; uint64_t i;
	inline Bits(double   x) : d(x){}
	inline Bits(uint64_t x) : i(x){}
};

#define BITS(x) (Bits(x).i)
#define RET(x) return Bits(x).d
#define CPX(X) do{ ret.real(X(z.real(), w.real())); ret.imag(X(z.imag(), w.imag())); }while(0)
#define CP1(X) do{ ret.real(X(z.real())); ret.imag(X(z.imag())); }while(0)
static inline double FMOD(double x,double y){ return x - y*floor(x/y); }

//----------------------------------------------------------------------------------------------------------------------
// operations on raw IEEE doubles
//----------------------------------------------------------------------------------------------------------------------

inline double XOR(double z, double w){ uint64_t c = BITS(z) ^ BITS(w); RET(c); }
inline double AND(double z, double w){ uint64_t c = BITS(z) & BITS(w); RET(c); }
inline double  OR(double z, double w){ uint64_t c = BITS(z) | BITS(w); RET(c); }
inline double NOT(double z)          { uint64_t c = ~BITS(z); RET(c); }

void XOR(const cnum &z, const cnum &w, cnum &ret){ CPX(XOR); }
void AND(const cnum &z, const cnum &w, cnum &ret){ CPX(AND); }
void  OR(const cnum &z, const cnum &w, cnum &ret){ CPX( OR); }
void NOT(const cnum &z,                cnum &ret){ CP1(NOT); }

inline double SHL(double z, double w)
{
	if (w < 0.0) return SHR(z, -w);
	if (!is_int(w)) return UNDEFINED;
	int k = (int)w; if (k > 63) return 0.0;
	uint64_t c = BITS(z) << k; RET(c);
}
inline double SHR(double z, double w)
{
	if (w < 0.0) return SHL(z, -w);
	if (!is_int(w)) return UNDEFINED;
	int k = (int)w; if (k > 63) return 0.0;
	uint64_t c = BITS(z) >> k; RET(c);
}
inline double ROL(double z, double w)
{
	if (w < 0.0) return ROR(z, -w);
	if (!is_int(w)) return UNDEFINED;
	int k = (int)FMOD(w, 64.0); if (k == 0) return z;
	uint64_t c = BITS(z) << k | BITS(z) >> (64-k); RET(c);
}
inline double ROR(double z, double w)
{
	if (w < 0.0) return ROL(z, -w);
	if (!is_int(w)) return UNDEFINED;
	if (w < 0.0) return SHR(z, -w);
	int k = (int)FMOD(w, 64.0); if (k == 0) return z;
	uint64_t c = BITS(z) >> k | BITS(z) << (64-k); RET(c);
}

void SHL(const cnum &z, const cnum &w, cnum &ret){ if (!is_int(w)) ret = UNDEFINED; else CPX(SHL); }
void SHR(const cnum &z, const cnum &w, cnum &ret){ if (!is_int(w)) ret = UNDEFINED; else CPX(SHR); }
void ROL(const cnum &z, const cnum &w, cnum &ret){ if (!is_int(w)) ret = UNDEFINED; else CPX(ROL); }
void ROR(const cnum &z, const cnum &w, cnum &ret){ if (!is_int(w)) ret = UNDEFINED; else CPX(ROR); }

double MANTISSA(double x)
{
	if (x == 0.0) return 0.0;
	uint64_t c = (BITS(x) << 12 >> 12) | (1023ULL << 52); RET(c);
}
double EXPONENT(double x)
{
	if (x == 0.0) return 0.0;
	uint64_t c = BITS(x) << 1 >> 53; return (double)c - 1023;
}
void MANTISSA(const cnum &z, cnum &ret){ CP1(MANTISSA); }
void EXPONENT(const cnum &z, cnum &ret){ CP1(EXPONENT); }

double DBIT(double x, double i0)
{
	if (!is_int(i0)) return UNDEFINED;
	if (i0 > 63.5 || i0 < -0.5) return 0.0;
	int i = (int)i0; assert(i >= 0 && i < 64);
	return (BITS(x) >> i) & 1 ? 1.0 : 0.0;
}
void DBIT(const cnum &z, const cnum &w, cnum &ret){ CPX(DBIT); }

//----------------------------------------------------------------------------------------------------------------------
// Binary versions - using a two's-complement that flips every bit, which makes x + (-x) = all ones = -0
//----------------------------------------------------------------------------------------------------------------------

static const char LogTable256[256] =
{
	#define LT(n) n, n, n, n, n, n, n, n, n, n, n, n, n, n, n, n
	-1, 0, 1, 1, 2, 2, 2, 2, 3, 3, 3, 3, 3, 3, 3, 3,
	LT(4), LT(5), LT(5), LT(6), LT(6), LT(6), LT(6),
	LT(7), LT(7), LT(7), LT(7), LT(7), LT(7), LT(7), LT(7)
	#undef LT
};

static inline int high_bit(uint64_t v)
{
	int k = 0;
	if (v >> 32)
	{
		k   = 32;
		v >>= 32;
	}
	if (v >> 16)
	{
		k  += 16;
		v >>= 16;
	}
	if (v >> 8)
	{
		k  += 8;
		v >>= 8;
	}
	return k + LogTable256[(unsigned)v];
}

double IBIT(double x, double i0)
{
	// IBIT(x, i) = 1 - IBIT(-x, i)
	if (x < 0.0) return 1.0 - IBIT(-x, i0);

	if (!is_int(i0)) return UNDEFINED;
	assert(x >= 0.0);

	int e = (int)(BITS(x) << 1 >> 53) - 1023; // sign bit could still have been 1 (for -0)

	// x = 1.m * 2^e
	// IBIT(x, i) = (x * 2 ^ -i) & 1 = 1.m * (2 ^ (e-i)) & 1
	// e-i < 0.0 => bit is zero
	if (i0-0.5 > e) return 0.0;
	
	// m has 52 bits, so if e-i > 52, bit is zero again
	if (i0+0.5 < e - 52) return 0.0;
	
	int i = (int)std::round(i0); // safe to cast now

	// if e-i == 0, this is the leading 1 (or 0 for subnormals)
	if (i == e) return e == -1023 ? 0.0 : 1.0;
	
	uint64_t m = BITS(x); // no need to clear the exponent bits

	assert(52-e+i >= 0 && 52-e+i < 52);
	return (m >> 52-e+i & 1) ? 1.0 : 0.0;
}

double I_XOR(double x, double y)
{
	if (x == 0.0) return y;
	if (y == 0.0) return x;

	bool s = false;
	if (x < 0.0){ s = !s; x = -x; }
	if (y < 0.0){ s = !s; y = -y; }
	if (x < y) std::swap(x,y);
	
	// now 0 < y < x
	
	int e1 = (int)(BITS(x) << 1 >> 53); // sign bit could still have been 1 (for -0)
	int e2 = (int)(BITS(y) << 1 >> 53); // sign bit could still have been 1 (for -0)
	assert(e1 >= e2);
	// unsigned 11 bit exponent, which means a range of [0, 2^11-1]
	assert(e1 < 1 << 11);
	assert(e2 < 1 << 11);
	if (e2 == 0) return x; // treat denormalized as 0

	if (e1 > e2)
	{
		// let e = e1-1023, e1 = e2+d (d > 0), so e = e2+d-1023 and e2-1023 = e-d
		// (1.m1 << e) ^ (1.m2 << (e2-1023)) = (1.m1 << e) ^ (1.m2 << (e-d))
		// = (1.m1 << e) ^ (1.m2 >> d << e)
		// = (1.m1 ^ (1.m2 >> d)) << e = 1.(m1 ^ (1.m2 >> d)) << e
		uint64_t m  = BITS(x) << 12 >> 12;
		m ^= (BITS(y) << 12 >> 12 | 1ULL << 52) >> e1-e2;

		m |= (uint64_t)e1 << 52; // add the exponent
		
		if (s) m |= 1ULL << 63;
		RET(m);
	}
	else
	{
		// let e = e1-1023 = e2-1023
		// (1.m1 << e) ^ (1.m2 << e) = 0.(m1 ^ m2) << e, which is denormalized
		uint64_t m = (BITS(x) ^ BITS(y)) << 12 >> 12;
		// now shift the first 1-bit to position 52
		int k = high_bit(m);
		assert(k < 52);
		if (k < 0) return 0.0;
		e1 -= 52-k;
		m <<= 52-k;
		assert((m >> 52) & 1);
		m ^= 1ULL << 52; // clear the bit
		
		if (e1 <= 0) return 0.0;
		assert(e1 < (1 << 11)); // we have only subtracted from e1
		m |= (uint64_t)e1 << 52; // add the exponent

		if (s) m |= 1ULL << 63;
		RET(m);
	}
}

double I_OR (double x, double y)
{
	if (fabs(x) < fabs(y)) std::swap(x,y);
	if (y == 0.0) return x;
	
	int e1 = (int)(BITS(x) << 1 >> 53); assert(e1 >= 0 && e1 < 1 << 11);
	int e2 = (int)(BITS(y) << 1 >> 53); assert(e2 >= 0 && e2 < 1 << 11);
	assert(e1 >= e2);
	if (e2 == 0) return x; // treat denormalized as 0

	uint64_t m1 = BITS(x) << 12 >> 12;
	uint64_t m2 = BITS(y) << 12 >> 12;
	
	if (e1 == e2)
	{
		// let e = e1-1023 = e2-1023
		if (x > 0.0 && y > 0.0)
		{
			// (1.m1 << e) | (1.m2 << e) = 1.(m1 | m2) << e
			uint64_t m = (m1 | m2) | (uint64_t)e1 << 52;
			RET(m);
		}
		if (x < 0.0 && y < 0.0)
		{
			// (...1110.~m1_111... << e) | (...1110.~m2_111... << e) = ...1110.(~m1 | ~m2)_111... << e
			// = -(1.~(~m1 | ~m2)) << e = -1.(m1 & m2)
			uint64_t m = (m1 & m2) | (uint64_t)e1 << 52 | 1ULL << 63;
			RET(m);
		}

		// (...1110.~m1_111... << e) | (1.m2 << e) = ...111.(~m1 | m2)_111 << e = -(0.~(~m1 | m2) << e)
		// = -(0.(m1 & ~m2) << e)
		uint64_t m = x < 0.0 ? (m1 & ~m2) : (~m1 & m2);
		int k = high_bit(m); assert(k < 52);
		if (k < 0) return 0.0;
		e1 -= 52-k; m <<= 52-k; assert((m >> 52) & 1);
		m ^= 1ULL << 52; // clear the bit
			
		if (e1 <= 0) return 0.0;
		m |= (uint64_t)e1 << 52; // add the exponent
		m |= 1ULL << 63;
		RET(m);
	}
	else
	{
		// let e = e1-1023 = e2+d-1023, d = e1-e2 and e2-1023 = e-d
		// +-(1.m1 << e) | +-(1.m2 << (e-d)) = +-(1.m1 << e) | +-(1.m2 << e >> d)
		// = (+-1.m1 | (+-1.m2 >> d)) << e
		
		if (x > 0.0 && y > 0.0)
		{
			// (1.m1 | (1.m2 >> d)) << e = 1.(m1 | (1.m2 >> d)) << e
			if (52+e2-e1 < 0) return x;
			m2 |= 1ULL << 52;
			uint64_t m = m1 | m2 >> e1-e2 | (uint64_t)e1 << 52;
			RET(m);
		}
		if (x < 0.0 && y < 0.0)
		{
			// (...1110.~m1_111... | (...1110.~m2_111... >> d)) << e
			// = -(1.m1_000 & (...0001.m2 >> d)) << e
			// = -(0.m1_000 << d & (...0001.m2)) << e-d
			m2 |= 1ULL << 52;
			
			if (e1-e2 > 53) return 0.0;
			uint64_t m = (m1 << (e1-e2) & m2);
			if (!(m >> 52 & 1))
			{
				int k = high_bit(m); assert(k < 52);
				if (k < 0) return 0.0;
				e2 -= 52-k; m <<= 52-k; assert((m >> 52) & 1);
			}
			m ^= 1ULL << 52;
			m |= (uint64_t)e2 << 52 | 1ULL << 63;
			RET(m);
		}
		
		if (x < 0.0)
		{
			// (...1110.~m1_111... | (1.m2 >> d)) << e
			// = -(1.m1 & (..1110.~m2_111... >> d) << e
			m2 |= 1ULL << 52;
			uint64_t m = m1 & ~(m2 << 11 >> 11+(e1-e2));
			m |= (uint64_t)e1 << 52 | 1ULL << 63;
			RET(m);
		}

		assert(y < 0.0);
		m2 |= 1ULL << 52;
		// (1.m1 | (...1110.~m2_111 >> d)) << e
		// = (...111.m1 | ...1110.~m2_111 >> d) << e
		// = -(0.~m1_111 & 1.m2 >> d) << e
		// = -(0.~m1_111 << d & 1.m2) << e-d
		uint64_t m = ~(m1 << e1-e2) & m2;
		if (!(m >> 52 & 1))
		{
			int k = high_bit(m); assert(k <= 52);
			if (k < 0) return 0.0;
			e2 -= 52-k; m <<= 52-k; assert((m >> 52) & 1);
		}
		m ^= 1ULL << 52;
		m |= (uint64_t)e2 << 52 | 1ULL << 63;
		RET(m);
	}
}
double I_AND(double x, double y)
{
	if (x == 0.0 || y == 0.0) return 0.0;
	return -I_OR(-x, -y);
}

void IBIT (const cnum &z, const cnum &w, cnum &ret){ CPX(IBIT); }
void I_XOR(const cnum &z, const cnum &w, cnum &ret){ CPX(I_XOR); }
void I_OR (const cnum &z, const cnum &w, cnum &ret){ CPX(I_OR ); }
void I_AND(const cnum &z, const cnum &w, cnum &ret){ CPX(I_AND); }

