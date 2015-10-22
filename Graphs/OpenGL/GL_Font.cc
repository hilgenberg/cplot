#include "GL_Font.h"
#include "../../Utility/StringFormatting.h"

void GL_Font::save(Serializer &s) const
{
	s._string(name);
	s._float(size);
	color.save(s);
}
void GL_Font::load(Deserializer &s)
{
	s._string(name);
	s._float(size);
	color.load(s);
}

std::ostream &operator<<(std::ostream &os, const GL_Font &f)
{
	os << '(' << f.name << ", " << f.size << "pt, " << f.color << ')';
	return os;
}

std::string GL_Font::to_string() const
{
	return name + format(" %d", (int)round(size));
}

GL_Font& GL_Font::operator= (const std::string &s)
{
	size_t n = s.length();
	if (!n)
	{
		// TODO: set some default font?
		throw error("Not a valid font", s);
	}

	// split into name and size
	size_t i = n;
	while (i > 0 && isdigit(s[i-1])) --i;
	if (i < n && i > 0 && (s[i-1]=='+' || s[i-1]=='-')) --i;
	if (i < n)
	{
		int k; is_int(s.c_str()+i, k);
		size = k;
		while (i > 0 && isspace(s[i-1])) --i;
		if (i > 0) name = s.substr(0, i);
	}
	else
	{
		name = s;
	}
	// TODO: if name = "bold" and such, modify current font
	return *this;
}
