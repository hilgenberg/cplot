#include "GL_BlendMode.h"
#include "GL_Color.h"
#include <GL/gl.h>
#include <iostream>
#include <cassert>
#include <vector>

static const std::vector<GL_BlendMode> &Modes_14()
{
	static std::vector<GL_BlendMode> m;
	if (m.empty())
	{
		m.resize(5);
		
		// GT_Off = 0
		m[0].off = true;
		
		// GT_Blend = 1
		m[1].fB = GL_SRC_ALPHA;
		m[1].fA = GL_ONE_MINUS_SRC_ALPHA;
		
		// GT_Add = 2
		m[2].fB = GL_SRC_ALPHA;
		m[2].fA = GL_DST_ALPHA;
		
		// GT_Subtract = 3
		m[3].fB = GL_ONE_MINUS_DST_COLOR;
		m[3].fA = GL_ONE_MINUS_DST_ALPHA;
		
		// GT_Multiply = 4
		m[4].fB = GL_CONSTANT_COLOR;
		m[4].fA = GL_SRC_COLOR;
		m[4].C  = GL_Color(0, 0, 0, 1.0f);
	}
	return m;
}

const std::vector<GL_DefaultBlendMode> &DefaultBlendModes()
{
	static std::vector<GL_DefaultBlendMode> m;
	if (m.empty())
	{
		m.resize(6);
		size_t i = 0;
		
		// GL_DST = screen = A = 1
		// GL_SRC = pixel  = B = 2

		m[i].name = "Off";
		m[i].mode.off = true;
		++i;
		
		m[i].name = "Blending";
		m[i].mode.fA = GL_ONE_MINUS_SRC_ALPHA;
		m[i].mode.fB = GL_SRC_ALPHA;
		++i;
		
		m[i].name = "Additive";
		m[i].mode.fA = GL_DST_ALPHA;
		m[i].mode.fB = GL_SRC_ALPHA;
		++i;
		
		m[i].name = "Subtractive";
		m[i].mode.fA = GL_ONE_MINUS_SRC_COLOR;
		m[i].mode.fB = GL_ONE_MINUS_SRC_ALPHA;
		++i;
		
		m[i].name = "Multiply";
		m[i].mode.fA = GL_SRC_COLOR;
		m[i].mode.fB = GL_CONSTANT_COLOR;
		m[i].mode.C  = GL_Color(0, 0, 0, 1.0f);
		++i;

		m[i].name = "Glass";
		m[i].mode.fA = GL_ONE_MINUS_SRC_ALPHA;
		m[i].mode.fB = GL_DST_COLOR;
		++i;
		
		assert(i == m.size());
	}
	return m;
}


GL_BlendMode::GL_BlendMode()
: fA(GL_ONE_MINUS_SRC_ALPHA)
, fB(GL_SRC_ALPHA)
, C(0.0, 0.75, 0.25, 1.0)
, off(false)
{
}

void GL_BlendMode::save(Serializer &s) const
{
	if (s.version() < FILE_VERSION_1_5)
	{
		const std::vector<GL_BlendMode> &m0 = Modes_14();
		bool found = false;
		for (int i = 0, n = (int)m0.size(); i < n; ++i)
		{
			if (m0[i] != *this) continue;
			found = true;
			s._enum(i, 0, n-1);
			break;
		}
		if (!found) NEEDS_VERSION(FILE_VERSION_1_5, "custom transparency");
	}
	else
	{
		s._bool(off);
		s._int32(fA);
		s._int32(fB);
		C.save(s);
	}
}
void GL_BlendMode::load(Deserializer &s)
{
	if (s.version() < FILE_VERSION_1_5)
	{
		const std::vector<GL_BlendMode> &m0 = Modes_14();
		int i, n = (int)m0.size();
		s._enum(i, 0, n-1);
		*this = m0[i];
	}
	else
	{
		s._bool(off);
		s._int32(fA);
		s._int32(fB);
		C.load(s);
	}
}

void GL_BlendMode::set() const
{
	if (off)
	{
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		return;
	}
	glEnable(GL_BLEND);
	if (usesConstAlpha()) glBlendColor(C.r, C.g, C.b, C.a);
	glBlendFunc(fB, fA);
}

//----------------------------------------------------------------------------------------------------------------------
// SmallMono: Monomials over {A,B,C,X} with exponents < 16
//----------------------------------------------------------------------------------------------------------------------

struct SmallMono
{
	union
	{
		struct
		{
			unsigned A:4, B:4, C:4, X:4, aA:4, aB:4, aC:4, aX:4;
		};
		uint32_t data;
	};
	
	SmallMono() : data(0){}
	SmallMono(uint8_t A, uint8_t B, uint8_t C, uint8_t X = 0) : data(0)
	{
		assert(sizeof(SmallMono) == sizeof(uint32_t));
		this->A = A;
		this->B = B;
		this->C = C;
		this->X = X;
	}
	
	SmallMono operator*(SmallMono b) const{ SmallMono x; x.data = data + b.data; return x; }
	
	SmallMono alpha() const
	{
		SmallMono x; x.data = data;
		x.aA += x.A; x.A = 0;
		x.aB += x.B; x.B = 0;
		x.aC += x.C; x.C = 0;
		x.aX += x.X; x.X = 0;
		return x;
	}
	
	bool operator<(const SmallMono &b) const{ return data < b.data; }
	
	void swap_AB()
	{
		uint8_t tmp;
		tmp =  A;  A =  B;  B = tmp;
		tmp = aA; aA = aB; aB = tmp;
	}
	void swap_BC()
	{
		uint8_t tmp;
		tmp =  B;  B =  C;  C = tmp;
		tmp = aB; aB = aC; aC = tmp;
	}
	
	enum Generator
	{
		GEN_A  = 0,
		GEN_B  = 1,
		GEN_C  = 2,
		GEN_X  = 3,
		GEN_aA = 4,
		GEN_aB = 5,
		GEN_aC = 6,
		GEN_aX = 7
	};

	unsigned clear(Generator generator)
	{
		// set generator to 1, return its power
		unsigned p = 0;
		switch(generator)
		{
			case GEN_A:  p =  A;  A = 0; break;
			case GEN_B:  p =  B;  B = 0; break;
			case GEN_C:  p =  C;  C = 0; break;
			case GEN_X:  p =  X;  X = 0; break;
			case GEN_aA: p = aA; aA = 0; break;
			case GEN_aB: p = aB; aB = 0; break;
			case GEN_aC: p = aC; aC = 0; break;
			case GEN_aX: p = aX; aX = 0; break;
			default: assert(false); break;
		}
		return p;
	}
};

std::ostream &operator<<(std::ostream &o, SmallMono m)
{
	static const char* const e[16] = {"", "", "²", "³", "⁴", "⁵", "⁶", "⁷", "⁸", "⁹", "¹⁰", "¹¹", "¹²", "¹³", "¹⁴", "¹⁵"};
	if (!m.data) return o << "1";
	bool alphas = (m.aA || m.aB || m.aC || m.aX);
	const char *sep = alphas ? "⋅" : "", *s = "";
	
	assert(m.A  <= 15); if (m.A ){ o << s << "A"  << e[m.A ]; s = sep; }
	assert(m.B  <= 15); if (m.B ){ o << s << "B"  << e[m.B ]; s = sep; }
	assert(m.C  <= 15); if (m.C ){ o << s << "C"  << e[m.C ]; s = sep; }
	assert(m.X  <= 15); if (m.X ){ o << s << "X"  << e[m.X ]; s = sep; }
	assert(m.aA <= 15); if (m.aA){ o << s << "αA" << e[m.aA]; s = sep; }
	assert(m.aB <= 15); if (m.aB){ o << s << "αB" << e[m.aB]; s = sep; }
	assert(m.aC <= 15); if (m.aC){ o << s << "αC" << e[m.aC]; s = sep; }
	assert(m.aX <= 15); if (m.aX){ o << s << "αX" << e[m.aX];          }
	return o;
}

//----------------------------------------------------------------------------------------------------------------------
// SmallPoly: Polynomials over {A,B,C,X} with exponents < 16, signed 8-bit coefficients
//----------------------------------------------------------------------------------------------------------------------

struct SmallPoly
{
	SmallPoly(){}
	SmallPoly(SmallMono x){ m[x] = 1; }
	SmallPoly(int8_t    x){ if (x) m[SmallMono()] = x; }
	
	bool zero() const{ return m.empty(); }
	
	SmallPoly operator+(SmallMono b) const{ SmallPoly x(*this); if (++x.m[b] == 0) x.m.erase(b); return x; }
	SmallPoly operator-(SmallMono b) const{ SmallPoly x(*this); if (--x.m[b] == 0) x.m.erase(b); return x; }
	
	SmallPoly operator+(const SmallPoly &b) const
	{
		SmallPoly x(*this);
		for (auto i : b.m) if ((x.m[i.first] += i.second) == 0) x.m.erase(i.first);
		return x;
	}
	SmallPoly operator-(const SmallPoly &b) const
	{
		SmallPoly x(*this);
		for (auto i : b.m) if ((x.m[i.first] -= i.second) == 0) x.m.erase(i.first);
		return x;
	}
	SmallPoly operator*(SmallMono b) const
	{
		SmallPoly x;
		for (auto i : m) x.m[i.first*b] = i.second;
		return x;
	}
	SmallPoly operator*(const SmallPoly &b) const
	{
		SmallPoly x;
		for (auto i : m)
		{
			for (auto j : b.m)
			{
				x.m[i.first*j.first] += i.second*j.second;
			}
		}
		return x.normalize();
	}
	
	SmallPoly &operator+=(const SmallPoly &b)
	{
		for (auto i : b.m) if ((m[i.first] += i.second) == 0) m.erase(i.first);
		return *this;
	}
	SmallPoly &operator-=(const SmallPoly &b)
	{
		for (auto i : b.m) if ((m[i.first] -= i.second) == 0) m.erase(i.first);
		return *this;
	}
	SmallPoly &operator*=(SmallMono b)
	{
		std::map<SmallMono, int8_t> tmp;
		for (auto i : m) tmp[i.first*b] = i.second;
		std::swap(m, tmp);
		return *this;
	}
	SmallPoly &operator*=(const SmallPoly &b)
	{
		std::map<SmallMono, int8_t> tmp;
		for (auto i : m)
		{
			for (auto j : b.m)
			{
				tmp[i.first*j.first] += i.second*j.second;
			}
		}
		std::swap(m, tmp);
		return normalize();
	}
	
	SmallPoly alpha() const
	{
		SmallPoly x;
		for (auto i : m)
		{
			x.m[i.first.alpha()] += i.second;
		}
		return x;
	}
	
	SmallPoly swap_AB() const
	{
		SmallPoly x;
		for (auto i : m)
		{
			SmallMono a = i.first; a.swap_AB();
			x.m[a] = i.second;
		}
		return x;
	}
	SmallPoly swap_BC() const
	{
		SmallPoly x;
		for (auto i : m)
		{
			SmallMono a = i.first; a.swap_BC();
			x.m[a] = i.second;
		}
		return x;
	}
	
	void set(SmallMono::Generator generator, int8_t v)
	{
		std::map<SmallMono, int8_t> tmp;
		if (v == 0)
		{
			for (auto i : m)
			{
				SmallMono a = i.first;
				unsigned p = a.clear(generator);
				if (!p) tmp[a] += i.second;
			}
			// actually needs no normalize() call
		}
		else if (v == 1)
		{
			for (auto i : m)
			{
				SmallMono a = i.first;
				a.clear(generator);
				tmp[a] += i.second;
			}
		}
		else
		{
			for (auto i : m)
			{
				SmallMono a = i.first;
				unsigned p = a.clear(generator);
				int vp = 1;
				for (int j = 0; j < (int)p; ++j) vp *= v;
				assert(abs(vp * i.second) < 100);
				tmp[a] += vp * i.second;
			}
		}
		std::swap(m, tmp);
		normalize();
	}

private:
	std::map<SmallMono, int8_t> m;
	
	SmallPoly &normalize()
	{
		auto i = m.begin();
		while (i != m.end())
		{
			if (i->second == 0)
			{
				m.erase(i++);
			}
			else
			{
				++i;
			}
		}
		return *this;
	}
	
	friend std::ostream &operator<<(std::ostream &o, const SmallPoly &p);
	
};

std::ostream &operator<<(std::ostream &o, const SmallPoly &p)
{
	if (p.zero()) return o << "0";
	bool first = true;
	for (auto i : p.m)
	{
		int c = i.second;
		if (!c) continue;
		o << (c < 0 ? (first ? "-" : " - ") : (first ? "" : " + "));
		first = false;
		c = abs(c);
		bool unit = !i.first.data;
		if (c > 1 || unit) o << c;
		if (!unit) o << i.first;
	}
	return o;
}

//----------------------------------------------------------------------------------------------------------------------
// Symmetry info methods
//
// GL_DST = screen = A = 1
// GL_SRC = pixel  = B = 2
//----------------------------------------------------------------------------------------------------------------------

bool GL_BlendMode::usesConstColor() const
{
	return
	fA == GL_CONSTANT_COLOR || fA == GL_ONE_MINUS_CONSTANT_COLOR ||
	fB == GL_CONSTANT_COLOR || fB == GL_ONE_MINUS_CONSTANT_COLOR;
}
bool GL_BlendMode::usesConstAlpha() const
{
	return usesConstColor() ||
	fA == GL_CONSTANT_ALPHA || fA == GL_ONE_MINUS_CONSTANT_ALPHA ||
	fB == GL_CONSTANT_ALPHA || fB == GL_ONE_MINUS_CONSTANT_ALPHA;
}

bool GL_BlendMode::forces_transparency() const
{
	// will the background come through even if B.alpha=1?
	
	if (off) return false;
	
	//--- find effective factors ---------------------------------------------------
	
	int eA = fA, eB = fB;
	if (fabsf(C.r-C.a) < 1e-12f && fabsf(C.g-C.a) < 1e-12f && fabsf(C.b-C.a) < 1e-12f)
	{
		if (eA == GL_CONSTANT_COLOR) eA = GL_CONSTANT_ALPHA;
		if (eB == GL_CONSTANT_COLOR) eB = GL_CONSTANT_ALPHA;
		if (eA == GL_ONE_MINUS_CONSTANT_COLOR) eA = GL_ONE_MINUS_CONSTANT_ALPHA;
		if (eB == GL_ONE_MINUS_CONSTANT_COLOR) eB = GL_ONE_MINUS_CONSTANT_ALPHA;
	}
	if (!C.visible())
	{
		if (eA == GL_CONSTANT_ALPHA) eA = GL_ZERO;
		if (eB == GL_CONSTANT_ALPHA) eB = GL_ZERO;
		if (eA == GL_ONE_MINUS_CONSTANT_ALPHA) eA = GL_ONE;
		if (eB == GL_ONE_MINUS_CONSTANT_ALPHA) eB = GL_ONE;
	}
	else if (C.opaque())
	{
		if (eA == GL_CONSTANT_ALPHA) eA = GL_ONE;
		if (eB == GL_CONSTANT_ALPHA) eB = GL_ONE;
		if (eA == GL_ONE_MINUS_CONSTANT_ALPHA) eA = GL_ZERO;
		if (eB == GL_ONE_MINUS_CONSTANT_ALPHA) eB = GL_ZERO;
	}
	
	// set B.alpha = 1
	switch(eA)
	{
		case GL_SRC_ALPHA:           eA = GL_ONE;  break;
		case GL_ONE_MINUS_SRC_ALPHA: eA = GL_ZERO; break;
		default: break;
	}
	switch(eB)
	{
		case GL_SRC_ALPHA:           eB = GL_ONE; break;
		case GL_ONE_MINUS_SRC_ALPHA: eB = GL_ZERO;break;
		case GL_SRC_ALPHA_SATURATE:  eB = GL_ONE_MINUS_DST_ALPHA; break;
		default: break;
	}
	
	// there's one case where AB cancels (A*B + B*(1-A) = B)
	if (eA == GL_SRC_COLOR && eB == GL_ONE_MINUS_DST_COLOR) return false;
	
	// otherwise A|B depends on A, if eA != 0
	if (eA != GL_ZERO) return true;
	
	switch(eB)
	{
		case GL_DST_COLOR:
		case GL_DST_ALPHA:
		case GL_ONE_MINUS_DST_COLOR:
		case GL_ONE_MINUS_DST_ALPHA: return true;
		
		default: return false;
	}
}

std::string GL_BlendMode::symmetry_info() const
{
	std::ostringstream s;
	symmetric(&s);
	return s.str();
}

bool GL_BlendMode::symmetric(std::ostream *os) const
{
	if (off) return true; // always uses depth-test
	
	//--- find effective factors ---------------------------------------------------

	int eA = fA, eB = fB;
	if (fabsf(C.r-C.a) < 1e-12f && fabsf(C.g-C.a) < 1e-12f && fabsf(C.b-C.a) < 1e-12f)
	{
		if (eA == GL_CONSTANT_COLOR) eA = GL_CONSTANT_ALPHA;
		if (eB == GL_CONSTANT_COLOR) eB = GL_CONSTANT_ALPHA;
		if (eA == GL_ONE_MINUS_CONSTANT_COLOR) eA = GL_ONE_MINUS_CONSTANT_ALPHA;
		if (eB == GL_ONE_MINUS_CONSTANT_COLOR) eB = GL_ONE_MINUS_CONSTANT_ALPHA;
	}
	if (!C.visible())
	{
		if (eA == GL_CONSTANT_ALPHA) eA = GL_ZERO;
		if (eB == GL_CONSTANT_ALPHA) eB = GL_ZERO;
		if (eA == GL_ONE_MINUS_CONSTANT_ALPHA) eA = GL_ONE;
		if (eB == GL_ONE_MINUS_CONSTANT_ALPHA) eB = GL_ONE;
	}
	else if (C.opaque())
	{
		if (eA == GL_CONSTANT_ALPHA) eA = GL_ONE;
		if (eB == GL_CONSTANT_ALPHA) eB = GL_ONE;
		if (eA == GL_ONE_MINUS_CONSTANT_ALPHA) eA = GL_ZERO;
		if (eB == GL_ONE_MINUS_CONSTANT_ALPHA) eB = GL_ZERO;
	}
	
	bool grey = ((eA == GL_CONSTANT_COLOR || eA == GL_ONE_MINUS_CONSTANT_COLOR ||
				  eB == GL_CONSTANT_COLOR || eB == GL_ONE_MINUS_CONSTANT_COLOR) &&
				 fabsf(C.g-C.r) < 1e-12f && fabsf(C.b-C.r) < 1e-12f);
	bool white  = (grey && fabsf(C.r-1.0f) < 1e-12f);
	bool black  = (grey && fabsf(C.r)      < 1e-12f);
	bool opaque = (grey &&  C.opaque());
	bool trans  = (grey && !C.visible());
	
	//--- handle alpha_saturate ----------------------------------------------------
	
	if (eB == GL_SRC_ALPHA_SATURATE)
	{
		/* first only look at the alpha channel for all values of eA:
		 
		 Note: ~ means (B <-> C)-symmetric parts were removed
		
		 ABC.a = (aA*f1+aB)*f2+aC = aA*f1*f2 + aB*f2 + aC, where fi are the eA-factors
		 
		 GL_ZERO: f1=f2=0 => aC
		 GL_ONE:  f1=f2=1 => aA+aB+aC, alpha is ok
		 GL_SRC_COLOR: f1 = aB, f2 = aC => aA*aB*aC + aB*aC + aC ~ aC
		 GL_DST_ALPHA: f1 = aA, f2 = aA^2+aB => aA^2*(aA^2+aB) + aB*(aA^2+aB) + aC ~ 2aA^2*aB + aB^2 + aC
		 GL_SRC_ALPHA: f1 = aB, f2 = aC => aA*aB*aC + aB*aC + aC ~ aC
		 GL_(ONE_MINUS)_CONSTANT_(COLOR|ALPHA): f1=f2=x => aA*x^2 + aB*x + aC ~ aB*x + aC
			
		 GL_ONE_MINUS_SRC_COLOR: f1 = 1-aB, f2 = 1-aC => aA*(1-aB)*(1-aC) + aB*(1-aC) + aC ~ aA*(1-aB)*(1-aC), alpha ok
		 GL_ONE_MINUS_DST_ALPHA: f1 = 1-aA, f2 = 1-aA+aA^2-aB
		   => aA*(1-aA)*(1-aA+aA^2-aB) + aB - aBaA + aBaA^2 - aB^2 + aC
		    ~ aA*(1-aA)*(-aB) - aBaA + aBaA^2 - aB^2
		    = -2(aA-aA^2)*aB - aB^2
		 GL_ONE_MINUS_SRC_ALPHA: f1 = 1-aB, f2 = 1-aC => aA*(1-aB)*(1-aC) + aB*(1-aC) + aC ~ aA*(1-aB)*(1-aC), alpha ok

		 Three passed:

		 ABC = (A*f1+B*f2)*f3+C*f4, where fi are the eA- and eB-factors
		 f2 = (min(aA, 1-aB), 1)
		 
		 GL_ONE:
		  f1 = f3 = 1,
		  ABC = A + (min(aA, 1-aB),1)*B + (min(aA+aB, 1-aC),1)*C
		  aB = 0, aC = 1: ABC.color = A + aA*B
		  aC = 0, aB = 1: ABC.color = A + C

		 GL_ONE_MINUS_SRC_COLOR:
		  f1 = 1-B, f3 = 1-C,
		  ABC = (A*(1-B)+B*(min(aA, 1-aB), 1))*(1-C)+C*(min(aA*(1-aB)+aB, 1-aC),1)
		  ABC.color = (A*(1-B)+B*min(aA, 1-aB))*(1-C)+C*min(aA*(1-aB)+aB, 1-aC)
		  C = 1: ABC.color = 0
		  B = 1: ABC.color = C*(1-aC)
		 
		 GL_ONE_MINUS_SRC_ALPHA:
		  f1 = 1-aB, f3 = 1-aC,
		  ABC = (A*(1-aB)+B*(min(aA, 1-aB), 1))*(1-aC)+C*(min(aA*(1-aB)+aB, 1-aC),1)
		  ABC.color = (A*(1-aB)+B*min(aA, 1-aB))*(1-aC)+C*min(aA*(1-aB)+aB, 1-aC)
		  C = 1: ABC.color = 0
		  B = 1: ABC.color = C*(1-aC)
		
		 => always needs depth-sorting!
		*/

		if (os) *os << "Saturation always needs depth-sorting";
		return false;
	}

	//--- check for symmetry -------------------------------------------------------

	const SmallMono A(1,0,0), B(0,1,0), C(0,0,1), X(0,0,0,1);
	SmallPoly P;
	using std::endl;
	
	switch(eA)
	{
		case GL_ZERO:                               break;
		case GL_ONE:            P += A;             break;
		case GL_SRC_COLOR:      P += A * B;         break;
		case GL_DST_ALPHA:      P += A * A.alpha(); break;
		case GL_SRC_ALPHA:      P += A * B.alpha(); break;
		case GL_CONSTANT_COLOR: P += A * X;         break;
		case GL_CONSTANT_ALPHA: P += A * X.alpha(); break;
			
		case GL_ONE_MINUS_SRC_COLOR:      P += A; P -= A * B;         break;
		case GL_ONE_MINUS_DST_ALPHA:      P += A; P -= A * A.alpha(); break;
		case GL_ONE_MINUS_SRC_ALPHA:      P += A; P -= A * B.alpha(); break;
		case GL_ONE_MINUS_CONSTANT_COLOR: P += A; P -= A * X;         break;
		case GL_ONE_MINUS_CONSTANT_ALPHA: P += A; P -= A * X.alpha(); break;
			
		default: assert(false); return false;
	}
	switch(eB)
	{
		case GL_ZERO:                               break;
		case GL_ONE:            P += B;             break;
		case GL_DST_COLOR:      P += B * A;         break;
		case GL_DST_ALPHA:      P += B * A.alpha(); break;
		case GL_SRC_ALPHA:      P += B * B.alpha(); break;
		case GL_CONSTANT_COLOR: P += B * X;         break;
		case GL_CONSTANT_ALPHA: P += B * X.alpha(); break;
			
		case GL_ONE_MINUS_DST_COLOR:      P += B; P -= B * A;         break;
		case GL_ONE_MINUS_DST_ALPHA:      P += B; P -= B * A.alpha(); break;
		case GL_ONE_MINUS_SRC_ALPHA:      P += B; P -= B * B.alpha(); break;
		case GL_ONE_MINUS_CONSTANT_COLOR: P += B; P -= B * X;         break;
		case GL_ONE_MINUS_CONSTANT_ALPHA: P += B; P -= B * X.alpha(); break;
			
		default: assert(false); return false;
	}
	
	if ((P-P.swap_AB()).zero())
	{
		if (os) *os << "[A|B] = [B|A] = " << P << endl;
		return true;
	}
	if (os) *os << "[A|B] = " << P << endl << "[A|B]-[B|A] = " << (P-P.swap_AB()) << endl;
	
	//--- check constant color -----------------------------------------------------

	if (grey)
	{
		// check alpha separately
		auto aP = P.alpha();
		if (opaque)
		{
			aP.set(SmallMono::GEN_aX, 1);
		}
		else if (trans)
		{
			aP.set(SmallMono::GEN_aX, 0);
		}
		bool alpha_ok = (aP-aP.swap_AB()).zero();
		
		// check color channels separately
		if (alpha_ok)
		{
			auto cP = P;
			bool color_ok;
			if (white)
			{
				cP.set(SmallMono::GEN_X, 1);
				color_ok = (cP-cP.swap_AB()).zero();
			}
			else if (black)
			{
				cP.set(SmallMono::GEN_X, 0);
				color_ok = (cP-cP.swap_AB()).zero();
			}
			else
			{
				// C remains C
				color_ok = false;
			}
			if (color_ok)
			{
				if (os) *os << "[A|B].α = [B|A].α = " << aP << endl
				            << "[A|B].c = [B|A].c = " << cP << endl;
				return true;
			}
		}
	}

	//--- check depth-sorting invariance -------------------------------------------

	SmallPoly Q(P);
	switch(eA)
	{
		case GL_ZERO:           Q = SmallPoly(); break;
		case GL_ONE:                             break;
		case GL_SRC_COLOR:      Q *= C;          break;
		case GL_DST_ALPHA:      Q *= P.alpha();  break;
		case GL_SRC_ALPHA:      Q *= C.alpha();  break;
		case GL_CONSTANT_COLOR: Q *= X;          break;
		case GL_CONSTANT_ALPHA: Q *= X.alpha();  break;
			
		case GL_ONE_MINUS_SRC_COLOR:      Q -= P * C;         break;
		case GL_ONE_MINUS_DST_ALPHA:      Q -= P * P.alpha(); break;
		case GL_ONE_MINUS_SRC_ALPHA:      Q -= P * C.alpha(); break;
		case GL_ONE_MINUS_CONSTANT_COLOR: Q -= P * X;         break;
		case GL_ONE_MINUS_CONSTANT_ALPHA: Q -= P * X.alpha(); break;
			
		default: assert(false); return false;
	}
	switch(eB)
	{
		case GL_ZERO:                               break;
		case GL_ONE:            Q += C;             break;
		case GL_DST_COLOR:      Q += P * C;         break;
		case GL_DST_ALPHA:      Q += P.alpha() * C; break;
		case GL_SRC_ALPHA:      Q += C * C.alpha(); break;
		case GL_CONSTANT_COLOR: Q += C * X;         break;
		case GL_CONSTANT_ALPHA: Q += C * X.alpha(); break;
			
		case GL_ONE_MINUS_DST_COLOR:      Q += C; Q -= P * C;         break;
		case GL_ONE_MINUS_DST_ALPHA:      Q += C; Q -= P.alpha() * C; break;
		case GL_ONE_MINUS_SRC_ALPHA:      Q += C; Q -= C.alpha() * C; break;
		case GL_ONE_MINUS_CONSTANT_COLOR: Q += C; Q -= X * C;         break;
		case GL_ONE_MINUS_CONSTANT_ALPHA: Q += C; Q -= X.alpha() * C; break;
			
		default: assert(false); return false;
	}
	
	if ((Q-Q.swap_BC()).zero())
	{
		if (os) *os << "[A|B|C] = [A|C|B] = " << Q << endl;
		return true;
	}
	if (os) *os << "[A|B|C] = " << Q << endl
	            << "[A|B|C] - [A|C|B] = " << (Q-Q.swap_BC()) << endl;
	
	//--- check constant color -----------------------------------------------------
	
	if (grey)
	{
		// check alpha separately
		auto aQ = Q.alpha();
		if (opaque)
		{
			aQ.set(SmallMono::GEN_aX, 1);
		}
		else if (trans)
		{
			aQ.set(SmallMono::GEN_aX, 0);
		}
		bool alpha_ok = (aQ-aQ.swap_BC()).zero();
		
		// check color channels separately
		auto cQ = Q;
		bool color_ok;
		if (white)
		{
			cQ.set(SmallMono::GEN_X, 1);
			color_ok = (cQ-cQ.swap_BC()).zero();
		}
		else if (black)
		{
			cQ.set(SmallMono::GEN_X, 0);
			color_ok = (cQ-cQ.swap_BC()).zero();
		}
		else
		{
			// C remains C
			color_ok = false;
		}
		if (color_ok)
		{
			if (alpha_ok)
			{
				if (os) *os << "[A|B|C].α = [A|C|B].α = " << aQ << endl;
			}
			else
			{
				if (os) *os << "Ignoring ([A|B|C] - [A|C|B]).α = " << (aQ-aQ.swap_BC()) << endl;
			}
			if (os) *os << "[A|B|C].c = [A|C|B].c = " << cQ << endl;
			return true;
		}
	}

	return false;
}


