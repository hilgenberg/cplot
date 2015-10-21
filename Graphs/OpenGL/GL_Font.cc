#include "GL_Font.h"

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
