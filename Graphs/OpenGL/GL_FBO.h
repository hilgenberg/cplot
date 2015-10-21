#pragma once
#include "GL_Context.h"

class GL_FBO
{
public:
	GL_FBO() : fbo(0), texture(0), depth(0), w(0), h(0){ }

	bool init(GL_Context context, unsigned w, unsigned h, int bpp, bool depth_buffer);
	bool clear(GL_Context context){ return init(context, 0, 0, 0, false); }
	
	void bind() const; // makes OpenGL output go into this buffer
	void draw(bool blend, float alpha = 1.0f) const; // blend=false, alpha<1 scales the copied pixels
	
	void acc_load(const GL_FBO &fbo, float alpha = 1.0f);
	void acc_add (const GL_FBO &fbo, float alpha = 1.0f);
	
private:
	GLuint fbo, texture, depth;
	unsigned w, h; // size of texture
};
