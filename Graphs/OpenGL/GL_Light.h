#pragma once
#include "../Geometry/Vector.h"

class GL_Light
{
public:
	GL_Light();

	void setup(bool enable) const; // setup position, but do not turn lighting on
	
	void on(float shinyness) const;
	void off() const;
	
	void direction(const P3f &w){ v.set(w, 0.0f); }
	void position (const P3f &w){ v.set(w, 1.0f); }

private:
	P4f v; // position or direction
	mutable bool enabled; // if false, on() does nothing
};
