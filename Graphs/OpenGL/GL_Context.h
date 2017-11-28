#pragma once
#include <cassert>
#include <GL/gl.h>

#ifdef DEBUG
#include <GL/glu.h>
#include <iostream>

#define GL_CHECK do{\
	GLenum err = glGetError();\
	if (err) std::cerr << "glERROR: " << gluErrorString(err) << std::endl;\
	assert(err == GL_NO_ERROR);\
}while(0)

#else
#define GL_CHECK
#endif


class GL_Context
{
public:
	explicit GL_Context(void *c) : ctx(c){ }
	explicit operator void* () const{ return ctx; }
	operator bool() const{ return ctx != NULL; }
	bool operator== (const void *c) const{ return ctx == c; }
	inline void delete_texture(GLuint &texture)
	{
		if (ctx && texture) glDeleteTextures(1, &texture);
		texture = 0;
	}

private:
	void *ctx;
};
