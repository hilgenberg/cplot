#include "GL_FBO.h"
#include <cassert>

#ifdef TXDEBUG
#define DEBUG_TEXTURES(x) std::cerr << x << std::endl
#else
#define DEBUG_TEXTURES(x)
#endif

bool GL_FBO::init(GL_Context context, unsigned w_, unsigned h_, int bpp, bool depth_buffer)
{
	assert(context);
	GL_CHECK;

	bool ok = (w_ > 0 && h_ > 0);
	
	if (ok && !fbo)
	{
		glGenFramebuffers(1, &fbo);
		if (!fbo) ok = false;
		DEBUG_TEXTURES("Allocating FBO: " << fbo);
	}

	if (w > 0 && h > 0)
	{
		if (depth && (!ok || !depth_buffer || w != w_ || h != h_))
		{
			DEBUG_TEXTURES("Releasing FBO depth (" << w << " x " << h << ")");
			context.delete_texture(depth);
			depth = 0;
		}
		if (texture && (!ok || w != w_ || h != h_))
		{
			DEBUG_TEXTURES("Releasing FBO texture (" << w << " x " << h << ")");
			context.delete_texture(texture);
			texture = 0;
			w = h = 0;
		}
	}
	assert(texture || !depth); // if texture was released, depth buffer was too
	
	if (ok && !texture)
	{
		glGenTextures(1, &texture);
		if (!texture) ok = false;
		DEBUG_TEXTURES("Allocating FBO texture id: " << texture);
	}
	if (ok && depth_buffer && !depth)
	{
		glGenTextures(1, &depth);
		if (!depth) ok = false;
		DEBUG_TEXTURES("Allocating FBO depth id: " << depth);
	}
	
	GL_CHECK;
	if (ok && (w != w_ || h != h_))
	{
		assert(w == 0 && h == 0);
		
		w = w_;
		h = h_;
		DEBUG_TEXTURES("Allocating FBO texture data (" << w << " x " << h << ")");
		
		glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
		glBindTexture(GL_TEXTURE_2D, texture);
		switch (bpp)
		{
			case 8:  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8,  w, h, 0, GL_BGRA, GL_UNSIGNED_BYTE,  NULL); break;
			case 16: glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F_ARB, w, h, 0, GL_BGRA, GL_HALF_FLOAT_ARB, NULL); break;
			//case 16: glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16, w, h, 0, GL_BGRA, GL_UNSIGNED_SHORT, NULL); break;
			case 32: glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F_ARB, w, h, 0, GL_BGRA, GL_FLOAT, NULL); break;
			default: assert(false); ok = false; break;
		}
		if (ok)
		{
			glBindTexture  (GL_TEXTURE_2D, texture);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		
			glBindFramebuffer(GL_FRAMEBUFFER, fbo);
			glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texture, 0);
			glBindFramebuffer(GL_FRAMEBUFFER, 0);

			GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
			if (status != GL_FRAMEBUFFER_COMPLETE)
			{
				ok = false;
				DEBUG_TEXTURES("Failed with code " << status);
			}
		}
		
		if (ok && depth_buffer)
		{
			DEBUG_TEXTURES("Allocating FBO depth data (" << w << " x " << h << ")");
			GL_CHECK;

			glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
			glBindTexture(GL_TEXTURE_2D, depth);
			glTexImage2D (GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT32, w, h, 0, GL_DEPTH_COMPONENT, GL_FLOAT, 0);
			GL_CHECK;

			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
			GL_CHECK;
			
			glBindFramebuffer(GL_FRAMEBUFFER, fbo);
			glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depth, 0);
			glBindFramebuffer(GL_FRAMEBUFFER, 0);
			GL_CHECK;
			
			GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
			if (status != GL_FRAMEBUFFER_COMPLETE)
			{
				ok = false;
				DEBUG_TEXTURES("Failed with code " << status);
			}
		}
	}
	GL_CHECK;
	
	if (!ok)
	{
		if (fbo)
		{
			DEBUG_TEXTURES("Releasing FBO: " << fbo);
			glBindFramebuffer(GL_FRAMEBUFFER, fbo);
			glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, 0, 0);
			glBindFramebuffer(GL_FRAMEBUFFER, 0);
			glDeleteFramebuffers(1, &fbo);
			fbo = 0;
		}
		if (depth)
		{
			DEBUG_TEXTURES("Releasing FBO depth data (" << w << " x " << h << ")");
			context.delete_texture(depth);
			depth = 0;
		}
		if (texture)
		{
			DEBUG_TEXTURES("Releasing FBO texture data (" << w << " x " << h << ")");
			context.delete_texture(texture);
			texture = 0;
			w = h = 0;
		}
	}
	GL_CHECK;
	return ok || (w_ == 0 || h_ == 0);
}

void GL_FBO::bind() const
{
	assert(fbo);
	glBindFramebuffer(GL_FRAMEBUFFER, fbo);
	glDrawBuffer(GL_COLOR_ATTACHMENT0);
}

void GL_FBO::draw(bool blend, float factor) const
{
	assert(fbo && texture);
	assert(factor <= 1.0f+1e-8f);
	assert(factor > 0.0f);
	assert(blend || fabs(factor-1.0f) < 1e-8f);

	if (!fbo || !texture){ assert(false); return; }
	
	glMatrixMode(GL_TEXTURE);    glLoadIdentity();
	glMatrixMode(GL_MODELVIEW);  glLoadIdentity();
	glMatrixMode(GL_PROJECTION); glLoadIdentity();
	glOrtho(0.0, 1.0, 0.0, 1.0, -1.0, 1.0);
	glDisable(GL_DEPTH_TEST);

	if (blend)
	{
		glEnable(GL_BLEND);
		if (factor < 1.0f-1e-8)
		{
			glBlendColor(0.0f, 0.0f, 0.0f, factor);
			glBlendFunc(GL_CONSTANT_ALPHA, GL_ONE);
		}
		else
		{
			glBlendFunc(GL_ONE, GL_ONE);
		}
	}
	else if (factor < 1.0f-1e-8)
	{
		assert(false);
		glEnable(GL_BLEND);
		glBlendColor(0.0f, 0.0f, 0.0f, factor);
		glBlendFunc(GL_CONSTANT_ALPHA, GL_ZERO);
	}
	else
	{
		glDisable(GL_BLEND);
	}
	glDisable(GL_LIGHTING);
	
	glActiveTexture(GL_TEXTURE0);
	glEnable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, texture);
	glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
	
	glBegin(GL_QUADS);
	glTexCoord2f(0.0f, 0.0f); glVertex3f(0.0f, 0.0f, 0.0f);
	glTexCoord2f(1.0f, 0.0f); glVertex3f(1.0f, 0.0f, 0.0f);
	glTexCoord2f(1.0f, 1.0f); glVertex3f(1.0f, 1.0f, 0.0f);
	glTexCoord2f(0.0f, 1.0f); glVertex3f(0.0f, 1.0f, 0.0f);
	glEnd();
	
	glDisable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, 0);
}

void GL_FBO::acc_load(const GL_FBO &other, float factor)
{
	if (factor < 1.0f-1e-8)
	{
		bind();
		glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
		glClear(GL_COLOR_BUFFER_BIT);
		acc_add(other, factor);
	}
	else
	{
		bind();
		glBindTexture(GL_TEXTURE_2D, texture);
		other.bind();
		glCopyTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 0, 0, w, h);
		glBindTexture(GL_TEXTURE_2D, 0);
	}
}

void GL_FBO::acc_add(const GL_FBO &other, float factor)
{
	bind();
	other.draw(true, factor);
	other.bind();
}
