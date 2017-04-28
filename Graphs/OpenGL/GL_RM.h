#pragma once
#include <GL/gl.h>
#include <set>
#include <vector>
#include <cassert>
#include "GL_Color.h"
#include "GL_AAMode.h"
#include "GL_Context.h"
#include "GL_FBO.h"

/*----------------------------------------------------------------------------------------------------------------------
 Resource Handling:
 - there is one GL_RM per GL-context/window
 - every GL_RM keeps track of which resources it has uploaded to its context (and with which parameters, such as alpha)
 - every GL_Resource knows its managers so it can tell them about its modifications and deletion
 ---------------------------------------------------------------------------------------------------------------------*/

class GL_RM;

class GL_Resource ///< Something the Resource Manager can upload to OpenGL
{
protected:
	friend class GL_RM;
	
	GL_Resource(){ }
	
	GL_Resource(const GL_Resource &)
	{
		// do not copy the managers!
	}
	
	GL_Resource &operator=(const GL_Resource &)
	{
		// do not copy the managers!
		return *this;
	}

	inline ~GL_Resource();
	inline void modify();

private:
	mutable std::set<GL_RM*> managers;
};

typedef GLuint GL_Handle;

struct GL_Image;
class GL_Mask;
class Camera;

class GL_RM ///< OpenGL Resource Manager
{
public:
	GL_RM(GL_Context context) : context(context), drawing(false){ }
	~GL_RM()
	{
		assert(!drawing);
		for (auto &i : resources)
		{
			i.first->managers.erase(this);
		}
	}
	
	bool matches(GL_Context c) const{ return context == c; }
	
	void start_drawing(AntialiasMode aa_mode)
	{
		assert(!drawing);
		
		for (auto &i : resources)
		{
			for (auto &j : i.second)
			{
				j.unused = true;
			}
		}
		
		switch (aa_mode)
		{
			case AA_4x:     aa_passes = 4; aa_use_accum = false; break;
			case AA_8x:     aa_passes = 8; aa_use_accum = false; break;
			case AA_4x_Acc: aa_passes = 4; aa_use_accum =  true; break;
			case AA_8x_Acc: aa_passes = 8; aa_use_accum =  true; break;
			default:        aa_passes = 1; aa_use_accum = false; break;
		}

		aa_pass = 0;
		drawing = true;
	}
	
	bool draw_subframe(const Camera &camera);
	
	void end_drawing()
	{
		assert(drawing && aa_pass == aa_passes);
		drawing = false;
		clear_unused();
	}
	
	//GL_Handle upload_private_texture(const GL_Image &im, float alpha, GL_Color *base){ return upload_texture(im, alpha, base, true); }
	//GL_Handle upload_private_texture(const GL_Image &im){ return upload_texture(im, -1.0f, NULL, true); }
	GL_Handle upload_texture(const GL_Image &im){ return upload_texture(im, -1.0f, NULL, false); }
	GL_Handle upload_texture(const GL_Image &im, float alpha, const GL_Color &base)
	{
		assert(alpha >= 0.0f && alpha <= 1.0f);
		return upload_texture(im, alpha, &base, false);
	}
	GL_Handle upload_texture(const GL_Image &im, float alpha)
	{
		assert(alpha >= 0.0f && alpha <= 1.0f);
		return upload_texture(im, alpha, NULL, false);
	}

	GL_Handle upload_mask(const GL_Mask &mask);
	
	void setup(bool repeat, bool interpolate, bool blend) const;
	
	void modified(const GL_Resource *r);
	void deleted(const GL_Resource *r);

private:
	void clear_unused();
	GL_Handle upload_texture(const GL_Image &im, float alpha, const GL_Color *base, bool temporary);

	GL_Context context;
	
	bool          drawing;
	int           aa_passes, aa_pass;
	GL_FBO        aa_accum, aa_buffer;
	bool          aa_use_accum; // use the real accumulation buffer

	struct ResourceInfo
	{
		GL_Handle handle;
		bool      unused, modified;
		unsigned  w, h;
		enum
		{
			Texture,
			Mask
		}
		type;
		
		float    alpha;
		GL_Color base;
		bool     baseUsed;
	};
	std::map<GL_Resource*, std::vector<ResourceInfo>> resources;
	std::vector<ResourceInfo> orphans;
	
	void end_subframe(); // called by draw_subframe to end the previous subframe
};


inline GL_Resource::~GL_Resource()
{
	for (GL_RM *m : managers) m->deleted(this);
}

inline void GL_Resource::modify()
{
	for (GL_RM *m : managers) m->modified(this);
}
