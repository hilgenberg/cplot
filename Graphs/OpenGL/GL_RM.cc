#include <GL/glew.h>
#include "GL_RM.h"
#include "GL_Image.h"
#include "GL_Mask.h"
#include "../Geometry/Camera.h"

#ifdef TXDEBUG
#define DEBUG_TEXTURES(x) std::cerr << x << std::endl
#else
#define DEBUG_TEXTURES(x)
#endif

//----------------------------------------------------------------------------------------------------------------------
// Antialiasing
//----------------------------------------------------------------------------------------------------------------------

bool GL_RM::draw_subframe(const Camera &camera)
{
	assert(drawing);
	assert(aa_pass <= aa_passes);
	
	if (aa_pass > 0) end_subframe();
	
	if (aa_pass >= aa_passes)
	{
		return false;
	}
	
	if (aa_pass == 0)
	{
		if (!aa_use_accum)
		{
			if (aa_passes > 1 && !aa_accum. init(context, camera.screen_w(), camera.screen_h(), 16, false)) aa_passes = 1;
			if (aa_passes > 1 && !aa_buffer.init(context, camera.screen_w(), camera.screen_h(),  8, true )) aa_passes = 1;
		}
		if (aa_passes == 1 || aa_use_accum)
		{
			aa_buffer.clear(context);
			aa_accum. clear(context);

			GLboolean b;
			glGetBooleanv(GL_DOUBLEBUFFER, &b);
			GLenum buf = b ? GL_BACK : GL_FRONT;
			glDrawBuffer(buf);
			glReadBuffer(buf);
		}
		else
		{
			aa_buffer.bind();
		}
	}
	
	camera.set(aa_pass, aa_passes);
	
	++aa_pass;
	return true;
}

void GL_RM::end_subframe()
{
	glFlush();
	if (aa_passes > 1)
	{
		if (aa_use_accum)
		{
			if (aa_pass == 1)
			{
				glAccum(GL_LOAD, 1.0f/aa_passes);
			}
			else if (aa_pass <= aa_passes)
			{
				glAccum(GL_ACCUM, 1.0f/aa_passes);
			}
			
			if (aa_pass == aa_passes)
			{
				glAccum(GL_RETURN, 1.0);
			}
		}
		else
		{
			if (aa_pass == 1)
			{
				aa_accum.acc_load(aa_buffer, 1.0f/aa_passes);
			}
			else if (aa_pass <= aa_passes)
			{
				aa_accum.acc_add(aa_buffer, 1.0f/aa_passes);
			}
			
			if (aa_pass == aa_passes)
			{
				glBindFramebuffer(GL_FRAMEBUFFER, 0);
				aa_accum.draw(false);
			}
		}
	}
}

//----------------------------------------------------------------------------------------------------------------------
// Resource Management
//----------------------------------------------------------------------------------------------------------------------

void GL_RM::modified(const GL_Resource *r)
{
	auto i = resources.find(const_cast<GL_Resource*>(r));
	if (i == resources.end()){ assert(false); return; }
	
	for (auto &j : i->second)
	{
		j.modified = true;
	}
}

void GL_RM::deleted(const GL_Resource *r)
{
	auto i = resources.find(const_cast<GL_Resource*>(r));
	if (i == resources.end()){ assert(false); return; }
	
	orphans.insert(orphans.end(), i->second.begin(), i->second.end());
	resources.erase(const_cast<GL_Resource*>(r));
}

void GL_RM::clear_unused()
{
	for (auto i = resources.begin(); i != resources.end(); )
	{
		for (auto j = i->second.begin(); j != i->second.end(); )
		{
			if (!j->unused){ ++j; continue; }
			
			switch(j->type)
			{
				case ResourceInfo::Texture:
				case ResourceInfo::Mask:
					DEBUG_TEXTURES("Releasing tex_ID " << j->handle);
					context.delete_texture(j->handle);
					break;
			}
			
			j = i->second.erase(j);
		}
		if (i->second.empty())
		{
			i->first->managers.erase(this);
			i = resources.erase(i);
		}
		else
		{
			++i;
		}
	}
	for (auto j = orphans.begin(); j != orphans.end(); ++j)
	{
		switch(j->type)
		{
			case ResourceInfo::Texture:
			case ResourceInfo::Mask:
				DEBUG_TEXTURES("Releasing tex_ID " << j->handle);
				context.delete_texture(j->handle);
				break;
		}
	}
	orphans.clear();
}

GL_Handle GL_RM::upload_texture(const GL_Image &im, float alpha, const GL_Color *base, bool temporary)
{
	assert(alpha >= 0.0f || !base);
	if (im.empty()) return 0;
	
	GL_CHECK;
	
	auto i = resources.find((GL_Resource*)(&im));
	if (i != resources.end())
	{
		assert(im.managers.count(this));

		// (1) check for exact matches
		
		for (auto &j : i->second)
		{
			if (j.type == ResourceInfo::Texture && !j.modified && (j.alpha < 0.0f && alpha < 0.0f || j.alpha == alpha && (!j.baseUsed && !base || j.baseUsed && base && j.base == *base)))
			{
				//DEBUG_TEXTURES("Texture (" << im.w << " x " << im.h << ") cached: " << j.handle);
				assert(j.w == im.w() && j.h == im.h());
				if (!temporary) j.unused = false;
				glBindTexture(GL_TEXTURE_2D, j.handle);
				GL_CHECK;
				return j.handle;
			}
		}
		
		// (2) check for modified entries with the same size
		
		for (auto &j : i->second)
		{
			if (j.type == ResourceInfo::Texture && j.modified && j.w == im.w() && j.h == im.h())
			{
				//DEBUG_TEXTURES("Texture (" << im.w() << " x " << im.h() << ") reused: " << j.handle);
				
				assert(j.unused);
				j.alpha = alpha; if (base) j.base = *base;
				j.baseUsed = base;
				j.unused = temporary;
				j.modified = false;
				j.type = ResourceInfo::Texture;
				
				// upload new data
				glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
				glBindTexture(GL_TEXTURE_2D, j.handle);
				if (alpha < 0.0f)
				{
					glTexSubImage2D(GL_TEXTURE_2D, 0, 0,0, im.w(), im.h(), GL_RGBA, GL_UNSIGNED_BYTE, im.data().data());
				}
				else
				{
					std::vector<unsigned char> data;
					if (base) im.mix(*base, alpha, data); else im.mix(alpha, data);
					assert(data.size() == (size_t)im.w() * im.h() * 4);
					
					glTexSubImage2D(GL_TEXTURE_2D, 0, 0,0, im.w(), im.h(), GL_RGBA, GL_UNSIGNED_BYTE, data.data());
				}
				
				GL_CHECK;
				return j.handle;
			}
		}
	}

	// (3) add new item

	auto &v = resources[(GL_Resource*)(&im)];
	v.emplace_back();
	auto &j = v.back();
	im.managers.insert(this);
	
	glGenTextures(1, &j.handle);
	DEBUG_TEXTURES("Allocating tex_ID for " << &im << " (" << im.w() << " x " << im.h() << "): " << j.handle);
	if (!j.handle){ assert(false); return 0; }

	j.w = im.w(); j.h = im.h(); j.alpha = alpha; if (base) j.base = *base; j.baseUsed = base;
	j.unused = temporary;
	j.modified = false;
	j.type = ResourceInfo::Texture;

	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	glBindTexture(GL_TEXTURE_2D, j.handle);
	if (alpha < 0.0f)
	{
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, im.w(), im.h(), 0, GL_RGBA, GL_UNSIGNED_BYTE, im.data().data());
	}
	else
	{
		std::vector<unsigned char> data;
		if (base) im.mix(*base, alpha, data); else im.mix(alpha, data);
		assert(data.size() == (size_t)im.w() * im.h() * 4);
		
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, im.w(), im.h(), 0, GL_RGBA, GL_UNSIGNED_BYTE, data.data());
	}
	
	GL_CHECK;
	return j.handle;
}

GL_Handle GL_RM::upload_mask(const GL_Mask &im)
{
	if (im.empty()) return 0;
	
	GL_CHECK;
	auto i = resources.find((GL_Resource*)(&im));
	if (i != resources.end())
	{
		// (1) check for exact matches
		
		for (auto &j : i->second)
		{
			if (j.type == ResourceInfo::Mask && !j.modified && j.alpha == (float)im.density())
			{
				//DEBUG_TEXTURES("Mask (" << im.w << " x " << im.w << ") cached: " << j.handle);
				assert(j.w == im.w() && j.h == im.h());
				j.unused = false;
				glBindTexture(GL_TEXTURE_2D, j.handle);
				GL_CHECK;
				return j.handle;
			}
		}
		
		// (2) check for modified entries with the same size
		
		for (auto &j : i->second)
		{
			if (j.type == ResourceInfo::Mask && j.modified && j.w == im.w() && j.h == im.h())
			{
				//DEBUG_TEXTURES("Mask (" << im.w() << " x " << im.h() << ") reused: " << j.handle);
				
				assert(j.unused);
				j.unused = j.modified = false;
				j.type = ResourceInfo::Mask;
				j.alpha = (float)im.density();
				
				// upload new data
				glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
				glBindTexture(GL_TEXTURE_2D, j.handle);
				if (im.custom())
				{
					std::vector<unsigned char> data;
					im.mix(data);
					assert(data.size() == (size_t)im.w() * im.h());

					glTexSubImage2D(GL_TEXTURE_2D, 0, 0,0, im.w(), im.h(), GL_ALPHA, GL_UNSIGNED_BYTE, data.data());
				}
				else
				{
					glTexSubImage2D(GL_TEXTURE_2D, 0, 0,0, im.w(), im.h(), GL_ALPHA, GL_UNSIGNED_BYTE, im.data().data());
				}
				
				GL_CHECK;
				return j.handle;
			}
		}
	}
	
	// (3) add new item
	
	auto &v = resources[(GL_Resource*)(&im)];
	v.emplace_back();
	auto &j = v.back();
	im.managers.insert(this);
	
	glGenTextures(1, &j.handle);
	DEBUG_TEXTURES("Allocating tex_ID for " << &im << ": " << j.handle);
	if (!j.handle){ assert(false); return 0; }

	j.w = im.w(); j.h = im.h();
	j.unused = j.modified = false;
	j.type = ResourceInfo::Mask;
	j.alpha = (float)im.density();

	GL_CHECK;
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	glBindTexture(GL_TEXTURE_2D, j.handle);
	if (im.custom())
	{
		std::vector<unsigned char> data;
		im.mix(data);
		assert(data.size() == (size_t)im.w() * im.h());
		
		glTexImage2D(GL_TEXTURE_2D, 0, GL_ALPHA, im.w(), im.h(), 0, GL_ALPHA, GL_UNSIGNED_BYTE, data.data());
	}
	else
	{
		glTexImage2D(GL_TEXTURE_2D, 0, GL_ALPHA, im.w(), im.h(), 0, GL_ALPHA, GL_UNSIGNED_BYTE, im.data().data());
	}
	GL_CHECK;
	
	return j.handle;
}

void GL_RM::setup(bool repeat, bool interpolate, bool blend) const
{
	GL_CHECK;
	GLenum x = repeat ? GL_REPEAT : GL_CLAMP_TO_EDGE;
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, x);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, x);
	x = interpolate ? GL_LINEAR : GL_NEAREST;
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, x);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, x);
	glDisable(GL_TEXTURE_GEN_S);
	glDisable(GL_TEXTURE_GEN_T);
	x = blend ? GL_MODULATE : GL_REPLACE;
	glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, x);

	glMatrixMode(GL_TEXTURE);
	glLoadIdentity();
	GL_CHECK;
}


