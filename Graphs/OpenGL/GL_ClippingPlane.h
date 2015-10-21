#pragma once
#include "../../Persistence/Serializer.h"
#include "../Geometry/Vector.h"
#include <cassert>
#include <GL/gl.h>
class Axis;

class GL_ClippingPlane : public Serializable
{
public:
	GL_ClippingPlane() : m_normal(0.0f, 1.0f, 0.0f), m_distance(0.0f), m_locked(false), m_enabled(false), m_id(0){ }
	
	virtual void save(Serializer   &s) const;
	virtual void load(Deserializer &s);
	
	bool on() const{ return m_enabled; }
	void on(bool f){ m_enabled = f; }
	
	void   set(const Axis &axis, GLenum plane_ID = GL_CLIP_PLANE0) const;
	void unset() const;
	
	void draw(const Axis &axis) const; // draws the intersections with the axis box
	
	float distance() const{ return m_distance; }
	void  distance(float d){ assert(d >= -2.0f && d <= 2.0f); m_distance = d; }

	const P3f &normal() const{ return m_normal; }
	void normal(const P3f &n){ assert(n.absq() > 0.0f); m_normal = n; m_normal.to_unit(); }
	
	bool locked() const{ return m_locked; }
	void locked(bool l){ m_locked = l; }
	
private:
	bool   m_enabled;
	P3f    m_normal;
	float  m_distance; // in +-√3, 0 = center of axis box
	bool   m_locked; // don't follow camera rotation?
	mutable GLenum m_id; // last set plane_ID
};
