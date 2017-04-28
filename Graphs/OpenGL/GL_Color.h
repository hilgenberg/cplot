#pragma once
#include "../../Persistence/Serializer.h"
#include <ostream>
#include <string>
#include <map>

struct GL_Color : public Serializable
{
	GL_Color(float r, float g, float b, float a = 1.0f) : r(r), g(g), b(b), a(a){ }
	GL_Color(float y = 1.0f, float a = 1.0f) : r(y), g(y), b(y), a(a){ }
	GL_Color(const GL_Color &) = default;

	virtual void save(Serializer   &s) const;
	virtual void load(Deserializer &s);
	
	bool     visible() const{ return a >= 1e-8f; }
	bool      opaque() const{ return a >= 1.0f-1e-8f; }
	float  lightness() const{ return 0.299f*r + 0.587f*g + 0.114f*b; }

	union
	{
		struct{ float r, g, b, a; };
		float v[4];
	};
	
	#ifdef _WINDOWS
	operator COLORREF() const { return RGB(int(r*255.0f), int(g*255.0f), int(b*255.0f)); }
	GL_Color &operator= (COLORREF c)
	{
		r = GetRValue(c) / 255.0f;
		g = GetGValue(c) / 255.0f;
		b = GetBValue(c) / 255.0f;
		// alpha is not changed!
	}
	#endif

	void clamp()
	{
		if (r < 0.0f) r = 0.0f; if (r > 1.0f) r = 1.0f;
		if (g < 0.0f) g = 0.0f; if (g > 1.0f) g = 1.0f;
		if (b < 0.0f) b = 0.0f; if (b > 1.0f) b = 1.0f;
		if (a < 0.0f) a = 0.0f; if (a > 1.0f) a = 1.0f;
	}
	
	void set() const;
	void set_clear() const;
	
	bool operator== (const GL_Color &c) const{ return fabsf(r-c.r) < 1e-8f && fabsf(g-c.g) < 1e-8f && fabsf(b-c.b) < 1e-8f && fabsf(a-c.a) < 1e-8f; }
	bool operator!= (const GL_Color &c) const{ return !operator==(c); }
	
	GL_Color operator* (float alpha) const{ assert(alpha >= 0.0f && alpha <= 1.0f); return GL_Color(r, g, b, a * alpha); }

	std::string to_string() const;
	GL_Color &operator= (const std::string &s); // throws if parsing fails
	void hsl(double h, double s, double l); // leaves a as is

	static const std::map<std::string, GL_Color> named_colors(); // red, green, ...
};

std::ostream &operator<<(std::ostream &os, const GL_Color &c);
