#pragma once

#include "Vector.h"
#include "../OpenGL/GL_Color.h"
#include "../OpenGL/GL_Font.h"
#include "../OpenGL/GL_Light.h"
#include "../../Persistence/Serializer.h"

#include <cmath>
#include <cassert>

struct AxisOptions : public Serializable
{
	AxisOptions()
	: axis_color(0.25f, 0.5f, 0.85f)
	, background_color(1.0f, 0.0f)
	#ifdef _WINDOWS
	, label_font("Arial", 9.0)
	#else
	, label_font("Lucida Grande", 9.0)
	#endif
	, hidden(false), axis_grid(AG_Cartesian)
	{ }
	
	virtual void save(Serializer   &s) const;
	virtual void load(Deserializer &s);
	
	GL_Color axis_color, background_color;
	GL_Font  label_font;
	GL_Light light;
	bool     hidden;
	
	enum AxisGridMode
	{
		AG_Off       = 0,
		AG_Cartesian = 1,
		AG_Polar     = 2
	};
	
	AxisGridMode axis_grid;
};

// Description of a space we can plot into
class Axis : public Serializable
{
public:
	enum Type
	{
		Invalid = -1,
		Rect    =  0, // in R^2
		Box     =  1, // in R^3
		Sphere  =  2  // always unit sphere
	};
	
	Axis();
	void save(Serializer &s) const;
	void load(Deserializer &s);

	inline Type type() const{ return m_type; }
	inline void type(Type t){ if (m_type == t) return; m_type = t; update(); }
	bool valid() const{ return m_type != Invalid; }
	int  dimension() const; // as real manifold
	int  embedding_dimension() const;

	inline double center(int i) const{ assert(i>=0 && i < 3); return m_effective_center[i]; }
	inline double  range(int i) const{ assert(i>=0 && i < 3); return m_effective_range[i]; }
	inline double    min(int i) const{ assert(i>=0 && i < 3); return m_effective_center[i] - m_effective_range[i]; }
	inline double    max(int i) const{ assert(i>=0 && i < 3); return m_effective_center[i] + m_effective_range[i]; }
	inline void center(int i, double x){ assert(i>=0 && i < 3); m_center[i] = x; update(); }
	inline void  range(int i, double x){ assert(i>=0 && i < 3); m_range [i] = x; update(); }

	inline double in_center(int i) const{ assert(i>=0 && i < 2); return m_in_center[i]; }
	inline double  in_range(int i) const{ assert(i>=0 && i < 2); return m_in_range[i]; }
	inline double    in_min(int i) const{ assert(i>=0 && i < 2); return m_in_center[i] - m_in_range[i]; }
	inline double    in_max(int i) const{ assert(i>=0 && i < 2); return m_in_center[i] + m_in_range[i]; }
	inline void in_center(int i, double x){ assert(i>=0 && i < 2); m_in_center[i] = x; }
	inline void  in_range(int i, double x){ assert(i>=0 && i < 2); m_in_range [i] = x; }
	
	AxisOptions options;
	
	void zoom(double factor); // multiplies range with factor
	void move(double dx, double dy, double dz); // adds these to center
	void reset_center(){ for (int i = 0; i < 3; ++i) m_center[i] = 0.0; update(); }
	void equal_ranges()
	{
		if (m_type != Box) return;
		double r = std::max(std::max(m_range[0], m_range[1]), m_range[2]);
		for (int i = 0; i < 3; ++i) m_range[i] = r;
		update();
	}
	
	void in_zoom(double factor); // multiplies in_range with factor
	void in_move(double dx, double dy); // adds these to in_center

	void window(int w, int h)
	{
		m_win_w = w;
		m_win_h = h;
		update();
	}
	
	#define COORD_MAX 100.0
	inline void map(const P3d &p, P3f &q) const
	{
		double rxi = 1.0 / m_effective_range[0];
		double x = (p.x - m_effective_center[0]) * rxi;
		double y = (p.y - m_effective_center[1]) * rxi;
		double z = (p.z - m_effective_center[2]) * rxi;

		q.x = (float)(fabs(x) < COORD_MAX ? x : x < 0 ? -COORD_MAX : COORD_MAX);
		q.y = (float)(fabs(y) < COORD_MAX ? y : y < 0 ? -COORD_MAX : COORD_MAX);
		q.z = (float)(fabs(z) < COORD_MAX ? z : z < 0 ? -COORD_MAX : COORD_MAX);
	}
	#undef COORD_MAX
	
private:
	Type m_type;
	double m_center[3], m_range[3];       // image: [c-r, c+r] = [min_i, max_i]
	double m_effective_center[3], m_effective_range[3];
	double m_in_center[2], m_in_range[2]; // preimage
	int m_win_w, m_win_h;
	
	void update();
};

