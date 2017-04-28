#include "Axis.h"
#include <cassert>
#include <stdexcept>

void AxisOptions::save(Serializer &s) const
{
	s.bool_(hidden);
	axis_color.save(s);
	background_color.save(s);
	label_font.save(s);
	s.enum_(axis_grid, AG_Off, AG_Polar);
}

void AxisOptions::load(Deserializer &s)
{
	s.bool_(hidden);
	axis_color.load(s);
	background_color.load(s);
	label_font.load(s);
	s.enum_(axis_grid, AG_Off, AG_Polar);
}

Axis::Axis() : m_type(Rect)
{
	m_center[0] = m_center[1] = m_center[2] = 0.0;
	m_range [0] = m_range [1] = m_range [2] = 1.0;
	m_in_center[0] = m_in_center[1] = 0.0;
	m_in_range [0] = m_in_range [1] = 1.0;
	m_win_w = m_win_h = 100;
	update();
}
void Axis::save(Serializer &s) const
{
	s.double_(m_center[0]);
	s.double_(m_center[1]);
	s.double_(m_center[2]);
	s.double_(m_range[0]);
	s.double_(m_range[1]);
	s.double_(m_range[2]);
	bool dummy = false;
	s.bool_(dummy);
	s.bool_(dummy);
	s.bool_(dummy);
	s.double_(m_in_center[0]);
	s.double_(m_in_center[1]);
	s.double_(m_in_range[0]);
	s.double_(m_in_range[1]);
	s.enum_(m_type, Invalid, Sphere);
	options.save(s);
}
void Axis::load(Deserializer &s)
{
	s.double_(m_center[0]);
	s.double_(m_center[1]);
	s.double_(m_center[2]);
	s.double_(m_range[0]);
	s.double_(m_range[1]);
	s.double_(m_range[2]);
	bool dummy = false;
	s.bool_(dummy);
	s.bool_(dummy);
	s.bool_(dummy);
	s.double_(m_in_center[0]);
	s.double_(m_in_center[1]);
	s.double_(m_in_range[0]);
	s.double_(m_in_range[1]);
	s.enum_(m_type, Invalid, Sphere);
	options.load(s);
	update();
}

void Axis::zoom(double d)
{
	const double zmin = 1.0e-12, zmax = 1.0e12;
	m_range[0] *= d;
	m_range[1] *= d;
	m_range[2] *= d;
	if (m_range[0] < zmin)
	{
		double f = zmin / m_range[0];
		m_range[0] = zmin;
		m_range[1] *= f;
		m_range[2] *= f;
	}
	else if (m_range[0] > zmax)
	{
		double f = zmax / m_range[0];
		m_range[0] = zmax;
		m_range[1] *= f;
		m_range[2] *= f;
	}
	update();
}
void Axis::move(double dx, double dy, double dz)
{
	m_center[0] += dx;
	m_center[1] += dy;
	m_center[2] += dz;
	update();
}

void Axis::in_zoom(double d)
{
	const double zmin = 1.0e-12, zmax = 1.0e12;
	m_in_range[0] *= d;
	m_in_range[1] *= d;
	if (m_in_range[0] < zmin)
	{
		double f = zmin / m_in_range[0];
		m_in_range[0] = zmin;
		m_in_range[1] *= f;
	}
	else if (m_in_range[0] > zmax)
	{
		double f = zmax / m_in_range[0];
		m_in_range[0] = zmax;
		m_in_range[1] *= f;
	}
	// no need for update
}
void Axis::in_move(double dx, double dy)
{
	m_in_center[0] += dx;
	m_in_center[1] += dy;
	// no need for update
}


int Axis::dimension() const
{
	switch(m_type)
	{
		case Invalid:     return -1;
		case Rect:        return  2;
		case Box:         return  3;
		case Sphere:      return  2;
	}
	assert(false); throw std::logic_error("type corrupted");
}
int Axis::embedding_dimension() const
{
	switch(m_type)
	{
		case Invalid:     return -1;
		case Rect:        return  2;
		case Box:         return  3;
		case Sphere:      return  3;
	}
	assert(false); throw std::logic_error("type corrupted");
}

void Axis::update()
{
	switch(m_type)
	{
		case Invalid:
		case Rect:
			m_effective_center[0] = m_center[0];
			m_effective_center[1] = m_center[1];
			m_effective_center[2] = 0.0;
			m_effective_range[0] = m_range[0];
			m_effective_range[1] = m_range[0] * (m_win_w > 0 ? (double)m_win_h / m_win_w : 1.0);
			m_effective_range[2] = 1.0;
			break;

		case Box:
			memcpy(m_effective_center, m_center, 3*sizeof(double));
			memcpy(m_effective_range,  m_range,  3*sizeof(double));
			break;

		case Sphere:
			for (int i = 0; i < 3; ++i)
			{
				m_effective_center[i] = 0.0;
				m_effective_range [i] = 1.0;
			}
			break;
	}
}
