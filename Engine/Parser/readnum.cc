#include "readnum.h"
#include "utf8/utf8.h"
#include <cmath>
#include <vector>
#include <cassert>
#include <sstream>

//--- Helper functions -------------------------------------------------------------------

static int isdig02(int c){ return c == '0' || c == '1'; }
static int isdig10(int c) { return c >= '0' && c <= '9'; }
static int isdig16(int c) { return c >= '0' && c <= '9' || c >= 'a' && c <= 'z' || c >= 'A' && c <= 'Z'; }
static int  d2n_10(int c){ return c - '0'; }
static int  d2n_16(int c){ return isdigit(c) ? c - '0' : tolower(c) - 'a' + 10; }

// Format: [nnn][.[nnn]][e [+-] nnn], returns length of the number
static size_t readnum(const std::string &s, size_t i0, size_t i1, double &x, int base)
{
	if(i0 >= i1) return 0;
	
	int (*is_digit)(int);
	int (* dig2num)(int);
	switch(base)
	{
		case  2: is_digit = &isdig02;  dig2num = &d2n_10; break;
		case 10: is_digit = &isdig10;  dig2num = &d2n_10; break;
		case 16: is_digit = &isdig16; dig2num = &d2n_16; break;
		default: return 0;
	}
	const char exponent_char = (base == 16 ? 'q' : 'e');
	
	x = 0.0;
	
	size_t i = i0;
	bool saw_digit = false;
	
	while (is_digit(s[i]))
	{ 
		x *= base;
		x += dig2num(s[i++]);
		saw_digit = true;
	}		
	if (s[i] == '.')
	{ 
		++i;
		double p = 1.0;
		while (is_digit(s[i]))
		{ 
			p /= base;
			x += p * dig2num(s[i++]);
			saw_digit = true;
		}
	}
	if (tolower(s[i]) == exponent_char && saw_digit &&
	    (i+1 < i1 && is_digit(s[i+1]) ||
	     i+2 < i1 && (s[i+1] == '+' || s[i+1] == '-') && is_digit(s[i+2])))
	{
		++i;
		bool neg = false;
		if (s[i] == '-')
		{
			++i;
			neg = true;
		}
		else if (s[i] == '+')
		{
			++i;
		}
		int e = 0;
		while (is_digit(s[i]))
		{
			e *= base;
			e += dig2num(s[i++]);
		}
		if(neg) e = -e;
		x *= pow((double)base, e);
	}
	return saw_digit && (i >= i1 || !isdig10(s[i]) && s[i] != '.') ? i-i0 : 0;
}

//----------------------------------------------------------------------------------------------------------------------

static uint32_t D[10] = {8304, 185, 178, 179, 8308, 8309, 8310, 8311, 8312, 8313}, // "⁰¹²³⁴⁵⁶⁷⁸⁹⁺⁻ⁱ"
                PLUS = 8314, MINUS = 8315, I = 8305;

static inline int isd(uint32_t c)
{
	switch (c)
	{
		case 8304: return 0;
		case  185: return 1;
		case  178: return 2;
		case  179: return 3;
		default:
			if (c >= 8308 && c <= 8313) return 4 + c - 8308;
			return -1;
	}
}

//----------------------------------------------------------------------------------------------------------------------

// Format: [0x | 0b]<number>
// Returns length of the number, or 0 if garbage
size_t readnum(const std::string &s, size_t i0, size_t i1, double &x)
{
	if (i1 <= i0 || !isdig10(s[i0]) && s[i0] != '.') return 0;
	
	if (s[i0] == '0' && i0+2 < i1 && (s[i0+1]=='b' || s[i0+1]=='x'))
	{
		int base = (s[i0+1] == 'b' ? 2 : 16);
		size_t len = ::readnum(s, i0+2, i1, x, base);
		if (len > 0)
		{
			return len+2;
		}
		else
		{
			x = 0.0;
			return 1;
		}
	}
	
	return ::readnum(s, i0, i1, x, 10);
}

size_t readexp(const std::string &S, size_t i0, size_t i1, int &re, int &im)
{
	try
	{
		auto s = S.begin() + i0, s1 = S.begin() + i1;
		size_t len = 0;
		re = im = 0;
		bool first = true;
		while (s < s1)
		{
			bool sign = false, negative = false;
			
			uint32_t c = utf8::next(s, s1);
			
			if (c == PLUS || c == MINUS)
			{
				sign = true;
				if (c == MINUS) negative = true;
				if (s >= s1) return 0;
				c = utf8::next(s, s1);
			}
			
			bool imag = false;
			if (c == I)
			{
				if (!sign && !first) return 0;
				imag = true;
				len = s - S.begin() - i0;
				c = s < s1 ? utf8::next(s, s1) : 0;
			}
			
			bool digits = false;
			int num = 0, cn;
			if ((cn = isd(c)) >= 0)
			{
				if (!sign && !first) return 0;
				num = cn;
				digits = true;
				len = s - S.begin() - i0;
				c = s < s1 ? utf8::next(s, s1) : 0;
				
				while ((cn = isd(c)) >= 0)
				{
					num *= 10; num += cn;
					len = s - S.begin() - i0;
					c = s < s1 ? utf8::next(s, s1) : 0;
				}
			}
			
			if (c == I)
			{
				if (imag) return 0;
				imag = true;
				len = s - S.begin() - i0;
				//c = s < s1 ? utf8::next(s, s1) : 0;
			}
			
			if (digits)
			{
				(imag ? im : re) += negative ? -num : num;
			}
			else if (imag)
			{
				negative ? --im : ++im;
			}
			else if (sign)
			{
				return 0;
			}
			else
			{
				break;
			}
			s = S.begin() + i0 + len;
			first = false;
		}
		
		return len;
	}
	catch(utf8::exception &)
	{
		assert(false);
		return 0;
	}
}

#ifndef INT_MIN
#define INT_MIN (-2147483647 - 1)
#endif

std::string format_exponent(int re, int im)
{
	std::vector<char> os;
	auto i = std::back_inserter(os);
	if (!re && !im)
	{
		utf8::append(D[0], i);
		return std::string(os.data(), os.size());
	}
	
	bool rmin = false, imin = false;
	if (re == INT_MIN){ rmin = true; ++re; }
	if (im == INT_MIN){ imin = true; ++im; }
	
	if (re < 0)
	{
		i = utf8::append(MINUS, i);
		re = -re;
	}
	if (re > 0)
	{
		int b = 1;
		while (re/10 >= b) b *= 10;
		while (b)
		{
			i = utf8::append(D[re/b % 10], i);
			b /= 10;
		}
	}
	if (rmin) ++os[os.size()-1];
	
	if (re != 0 && im > 0)
	{
		i = utf8::append(PLUS, i);
	}
	else if (im < 0)
	{
		i = utf8::append(MINUS, i);
		im = -im;
	}
	if (im > 1)
	{
		int b = 1;
		while (im/10 >= b) b *= 10;
		while (b)
		{
			i = utf8::append(D[im/b % 10], i);
			b /= 10;
		}
	}
	if (imin) ++os[os.size()-1];
	if (im != 0)
	{
		i = utf8::append(I, i);
	}
	
	return std::string(os.data(), os.size());
}

std::string format_subscript(int re)
{
	uint32_t S0 = 0x2080, SM = 0x208B; // subscript 0 and -
	std::vector<char> os;
	auto i = std::back_inserter(os);
	if (!re)
	{
		utf8::append(S0, i);
		return std::string(os.data(), os.size());
	}
	
	bool rmin = false;
	if (re == INT_MIN){ rmin = true; ++re; }
	
	if (re < 0)
	{
		i = utf8::append(SM, i);
		re = -re;
	}
	if (re > 0)
	{
		int b = 1;
		while (re/10 >= b) b *= 10;
		while (b)
		{
			i = utf8::append(S0 + re/b % 10, i);
			b /= 10;
		}
	}
	if (rmin) ++os[os.size()-1];
	
	return std::string(os.data(), os.size());
}
