#include "GL_Color.h"
#include <GL/gl.h>
#include <cstring>
#include "../../Utility/StringFormatting.h"

void GL_Color::save(Serializer &s) const
{
	s._float(r);
	s._float(g);
	s._float(b);
	s._float(a);
}
void GL_Color::load(Deserializer &s)
{
	s._float(r);
	s._float(g);
	s._float(b);
	s._float(a);
}

void GL_Color::set() const
{
	glColor4fv(v);
}

void GL_Color::set_clear() const
{
	glClearColor(r, g, b, a);
}

std::ostream &operator<<(std::ostream &os, const GL_Color &c)
{
	os << '(' << c.r << ", " << c.g << ", " << c.b << ", " << c.a << ')';
	return os;
}

static double hue2rgb(double p, double q, double t)
{
	if(t < 0.0) t += 6.0;
	if(t > 6.0) t -= 6.0;
	if(t < 1.0) return p + (q - p) * t;
	if(t < 3.0) return q;
	if(t < 4.0) return p + (q - p) * (4.0 - t);
	return p;
}
void GL_Color::hsl(double h, double s, double l)
{
	if (h < 0.0) h = 0.0; else if (h > 1.0) h = 1.0;
	if (s < 0.0) s = 0.0; else if (s > 1.0) s = 1.0;
	if (l < 0.0) l = 0.0; else if (l > 1.0) l = 1.0;
	if (s == 0.0) // achromatic
	{
		r = g = b = (float)l;
	}
	else
	{
		h *= 0.5*M_1_PI; if (h < 0.0) ++h;
		double q = l < 0.5 ? l * (1.0 + s) : l + s - l * s;
		double p = 2.0 * l - q;
		r = (float)hue2rgb(p, q, 6.0*h + 2.0);
		g = (float)hue2rgb(p, q, 6.0*h);
		b = (float)hue2rgb(p, q, 6.0*h - 2.0);
	}
}


const std::map<std::string, GL_Color> GL_Color::named_colors()
{
	static std::map<std::string, GL_Color> C;
	if (C.empty())
	{
		C["red"]   = GL_Color(1.0f, 0.0f, 0.0f);
		C["green"] = GL_Color(0.0f, 1.0f, 0.0f);
		C["blue"]  = GL_Color(0.0f, 0.0f, 1.0f);
		C["white"] = GL_Color(1.0f);
		C["black"] = GL_Color(0.0f);
	}
	return C;
}

static inline unsigned char d2n(char c)
{
	assert(isxdigit(c));
	return c >= '0' && c <= '9' ? c-'0' :
	       c >= 'a' && c <= 'f' ? c-'a'+10 :
	       c-'A'+10;
}
static bool read_hex(const char *s, GL_Color &c)
{
	// [#]RRGGBB[AA] | [#]RGB[A]
	if (*s == '#') ++s;
	int n = 0;
	for (; s[n]; ++n) if (!isxdigit(s[n])) return false;
	if (n < 3 || n > 8 || n == 5 || n == 7) return false;
	unsigned char r, g, b, a;
	
	switch (n)
	{
		case 3:
		case 4:
			r = d2n(*s++)*0x11;
			g = d2n(*s++)*0x11;
			b = d2n(*s++)*0x11;
			a = (n==3 ? 255 : d2n(*s)*0x11);
			break;
		case 6:
		case 8:
			r = d2n(*s)*0x10 + d2n(s[1]); s += 2;
			g = d2n(*s)*0x10 + d2n(s[1]); s += 2;
			b = d2n(*s)*0x10 + d2n(s[1]); s += 2;
			a = (n==6 ? 255 : d2n(*s)*0x10 + d2n(s[1]));
			break;

		default: return false;
	}

	c.r = (float)r / 255.0f;
	c.g = (float)g / 255.0f;
	c.b = (float)b / 255.0f;
	c.a = (float)a / 255.0f;
	return true;
}
static bool read_float(const char *s, GL_Color &c)
{
	// hsl(h,s,l,[a]), rgb(r,g,b,[a]), rgba(r,g,b,a)
	// y(y,[a])
	// (r, g, b[, a]), floats in [0,1]
	int t = 0; bool hsl = false;
	if      (has_prefix(s, "rgba", true))  t = 4;
	else if (has_prefix(s, "rgb",  true))  t = 3;
	else if (has_prefix(s, "hsl",  true)){ t = 3; hsl = true; }
	else if (has_prefix(s, "y",    true))  t = 1;
	s += t;
	while (isspace(*s)) ++s;
	if (*s != '(') return false;
	++s; while (isspace(*s)) ++s;
	int n = strlen(s);
	if (n < 2 || s[n-1] != ')') return false;
	--n;
	int pl = 0, commas[3], nc = 0;
	for (int i = 0; i < n; ++i) switch (s[i])
	{
		case '(': ++pl; break;
		case ')': --pl; if (pl < 0) return false; break;
		case ',': if (pl) break;
		          if (nc >= 3) return false;
		          commas[nc++] = i;
		          break;
	}
	if (t == 4 && nc != 3 || t == 1 && nc > 1 || t == 3 && nc < 2) return false;
	float comp[4]; // assign to c.v only if all is ok
	try
	{
		for (int i = 0; i <= nc; ++i)
		{
			int len = (i == nc ? n : i == 0 ? commas[0] : commas[i]-commas[i-1]-1);
			cnum z = evaluate(std::string(s, s+len));
			if (!defined(z) || !is_real(z)) return false;
			s += len+1;
			n -= len+1;
			comp[i] = (float)z.real();
		}
	}
	catch (...)
	{
		return false;
	}

	if (hsl)
	{
		c.hsl(comp[0], comp[1], comp[2]);
		c.a = (nc<3 ? 1.0f : comp[3]);
	}
	else switch (t)
	{
		case 1: c.r = c.g = c.b = comp[0]; c.a = (nc ? comp[1] : 1.0f); break;
		case 4: memcpy(c.v, comp, 4*sizeof(float)); break;
		case 0:
		case 3: memcpy(c.v, comp, 4*sizeof(float)); c.a = (nc<3 ? 1.0f : comp[3]); break;
	}
	c.clamp();
	return true;
}

GL_Color& GL_Color::operator= (const std::string &S)
{
	// empty string -> empty color
	if (S.empty()){ r=g=b=a=0.0f; return *this; }

	// [#]RRGGBB[AA] | [#]RGB[A]
	const char *s = S.c_str();
	if (read_hex(s, *this)) return *this;

	// 0..15 (terminal colors)
	// + xterm256 colors
	/*int n; if (is_int(S, n))
	{
		// TODO
	}*/

	// "red", "green", ...
	auto &C = named_colors();
	auto i = C.find(S);
	if (i != C.end()){ return *this = i->second; }

	// rgb(r,g,b,[a]), etc
	if (read_float(s, *this)) return *this;

	throw error("invalid color", S);
}

std::string GL_Color::to_string() const
{
	static const char d[16+1] = "0123456789ABCDEF";
	auto hex = [](float x)
	{
		int c = x*255.0f;
		if (c < 0) c = 0; else if (c > 255) c = 255;
		return (unsigned char)c;
	};
	std::string s(8, ' ');
	auto c = hex(r); s[0] = d[c >> 4]; s[1] = d[c & 15];
	c = hex(g); s[2] = d[c >> 4]; s[3] = d[c & 15];
	c = hex(b); s[4] = d[c >> 4]; s[5] = d[c & 15];
	c = hex(a);
	if (c==255)
	{
		s.erase(6);
		if (s[0]==s[1] && s[2]==s[3] && s[4]==s[5])
		{
			s.erase(5);
			s.erase(3,1);
			s.erase(1,1);
		}
	}
	else
	{
		s[6] = d[c >> 4]; s[7] = d[c & 15];
	}
	return s;
}
