#pragma once
#include <string>
#include <ostream>
#include "GL_Color.h"

struct GL_Font : public Serializable
{
	GL_Font(const std::string &name, float size) : name(name), size(size), color(0,0,0,1){}
	
	std::string name;
	float       size;
	GL_Color    color;
	
	virtual void save(Serializer   &s) const;
	virtual void load(Deserializer &s);
	bool visible() const{ return color.visible() && size > 1e-12f; }
	bool operator== (const GL_Font &f) const{ return name==f.name && color==f.color && fabs(size-f.size) < 1e-12f; }
};
	
std::ostream &operator<<(std::ostream &os, const GL_Font &f);
