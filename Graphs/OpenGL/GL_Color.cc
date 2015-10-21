#include "GL_Color.h"
#include <GL/gl.h>

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
GL_Color& GL_Color::operator= (const std::string &S)
{
	// empty string -> empty color
	if (S.empty()){ r=g=b=a=0.0f; return *this; }

	// [#]RRGGBB[AA] | [#]RGB[A]
	const char *s = S.c_str();
	if (read_hex(s, *this)) return *this;

	// 0..15 (terminal colors)

	// "red", "green", ...
	auto &C = named_colors();
	auto i = C.find(S);
	if (i != C.end()){ return *this = i->second; }

	// (r, g, b[, a]), floats in [0,1]

	// hsl(h,s,l), rgb(r,g,b)

	// y(level)

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
