#include "Parameter.h"
#include "Expression.h"

void Parameter::save(Serializer &s) const
{
	Element::save(s);
	s.enum_(m_type, Real, Integer);
	s.cnum_(v);
	s.double_(m_min);
	s.double_(m_max);
	s.double_(m_imin);
	s.double_(m_imax);
	s.double_(m_rmax);
	s.bool_(radians);
}
void Parameter::load(Deserializer &s)
{
	Element::load(s);
	s.enum_(m_type, Real, Integer);
	s.cnum_(v);
	s.double_(m_min);
	s.double_(m_max);
	s.double_(m_imin);
	s.double_(m_imax);
	s.double_(m_rmax);
	s.bool_(radians);
}
Parameter &Parameter::operator=(const Parameter &p0)
{
	type(p0.type());
	angle_in_radians(p0.angle_in_radians());
	min (p0.min ());
	max (p0.max ());
	imin(p0.imin());
	imax(p0.imax());
	rmax(p0.rmax());
	value(p0.value());
	rename(p0.name());
	return *this;
}

Parameter::Parameter(const std::string &n, ParameterType type, const cnum &v)
: Element(n), m_type(type), v(v),
  m_min(UNDEFINED), m_max(UNDEFINED), m_imin(UNDEFINED), m_imax(UNDEFINED), m_rmax(UNDEFINED),
  radians(true)
{
}

Element *Parameter::copy() const
{
	Parameter *p = new Parameter(name(), m_type, v);
	p->m_min = m_min;
	p->m_max = m_max;
	p->m_imin = m_imin;
	p->m_imax = m_rmax;
	p->radians = radians;
	return p;
}

void Parameter::type(ParameterType t)
{
	if (t == m_type) return;
	m_type = t;
	cnum v0(v);
	v = UNDEFINED; // force update
	value(v0);
	redefine();
}

static inline double sqr(double x){ return x*x; }

void Parameter::value(const cnum &val)
{
	switch(m_type)
	{
		case Angle:
			v = val.real(); break; // maybe we should normalize here

		case Integer:
		{
			double x = round(val.real());
			if (defined(m_min))
			{
				if (x < m_min) x = ceil(m_min-EPSILON);
			}
			if (defined(m_max))
			{
				if (x > m_max) x = floor(m_max+EPSILON);
				if (defined(m_min) && x < m_min) x = UNDEFINED;
			}
			v = x;
			break;
		}
		case Real:
		{
			double x = val.real(); // projection to R is the nearest value
			if (defined(m_min) && x < m_min)
			{
				x = m_min;
			}
			if (defined(m_max) && x > m_max)
			{
				x = m_max;

				if (defined(m_min) && x < m_min)
				{
					x = UNDEFINED;
				}
			}
			v = x;
			break;
		}	
		case ComplexAngle:
			v = val;
			to_unit(v);
			if (!defined(v)) v = 1.0;
			break;
			
		case Complex:
		{
			v = val;
			if (defined(m_rmax))
			{
				double r = abs(v);
				if (r > m_rmax) v *= m_rmax / r;
				// v is now the closest match on the disc
				
				bool r1 = (defined(m_min)  && v.real() < m_min);
				bool r2 = (defined(m_max)  && v.real() > m_max);
				bool i1 = (defined(m_imin) && v.imag() < m_imin);
				bool i2 = (defined(m_imax) && v.imag() > m_imax);
				if (r1 || r2 || i1 || i2)
				{
					cnum w = val;
					if (defined(m_min)  && w.real() < m_min)  w.real(m_min);
					if (defined(m_max)  && w.real() > m_max)  w.real(m_max);
					if (defined(m_imin) && w.imag() < m_imin) w.imag(m_imin);
					if (defined(m_imax) && w.imag() > m_imax) w.imag(m_imax);
					// w is the closest match in the rect
					
					if (abs(w) <= m_rmax)
					{
						v = w; // any other point in the intersection would be farther away from val
					}
					else
					{
						// w is outside the circle, v is outside the rect => match must be a corner
						std::vector<cnum> candidates;
						#define Y(x) sqrt(sqr(m_rmax)-sqr(x))
						if (defined(m_min) && fabs(m_min) <= m_rmax)
						{
							candidates.emplace_back(m_min,  Y(m_min));
							candidates.emplace_back(m_min, -Y(m_min));
						}
						if (defined(m_max) && fabs(m_max) <= m_rmax)
						{
							candidates.emplace_back(m_max,  Y(m_max));
							candidates.emplace_back(m_max, -Y(m_max));
						}
						if (defined(m_imin) && fabs(m_imin) <= m_rmax)
						{
							candidates.emplace_back( Y(m_imin), m_imin);
							candidates.emplace_back(-Y(m_imin), m_imin);
						}
						if (defined(m_imax) && fabs(m_imax) <= m_rmax)
						{
							candidates.emplace_back( Y(m_imax), m_imax);
							candidates.emplace_back(-Y(m_imax), m_imax);
						}
						#undef Y
						
						double best = -1.0;
						
						for (cnum z : candidates)
						{
							if (defined(m_min)  && z.real() < m_min)  continue;
							if (defined(m_max)  && z.real() > m_max)  continue;
							if (defined(m_imin) && z.imag() < m_imin) continue;
							if (defined(m_imax) && z.imag() > m_imax) continue;
							double d = absq(z-val);
							if (best < 0.0 || d < best){ best = d; v = z; }
						}
						if (best < 0.0) v = UNDEFINED;
					}
				}
			}
			else
			{
				if ((defined(m_min)  && defined(m_max)  && m_min  > m_max)
				||  (defined(m_imin) && defined(m_imax) && m_imin > m_imax))
				{
					v = UNDEFINED;
				}
				else
				{
					if (defined(m_min)  && v.real() < m_min)  v.real(m_min);
					if (defined(m_max)  && v.real() > m_max)  v.real(m_max);
					if (defined(m_imin) && v.imag() < m_imin) v.imag(m_imin);
					if (defined(m_imax) && v.imag() > m_imax) v.imag(m_imax);
				}
			}
			break;
		}
	}
}

bool Parameter::value_ok(const cnum &val) const
{
	if (!defined(val)) return false;
	
	switch(m_type)
	{
		case Angle:
			return ::is_real(val);
			
		case Integer:
		{
			if (!::is_real(val)) return false;
			double x = round(val.real());
			if (fabs(x-val.real()) > 1e-12) return false;
			return (!defined(m_min) || x >= m_min) && (!defined(m_max) || x <= m_max);
		}
		case Real:
		{
			if (!::is_real(val)) return false;
			double x = val.real();
			return (!defined(m_min) || x >= m_min) && (!defined(m_max) || x <= m_max);
		}
		case ComplexAngle:
		{
			if (::is_real(val)) return true;
			cnum x = val; to_unit(x);
			return defined(x) && abs(x-val) < 1e-12;
		}
		case Complex:
		{
			return (!defined(m_rmax) || abs(val) <= m_rmax+1e-12)
			    && (!defined(m_min ) || val.real() >= m_min ) && (!defined(m_max ) || val.real() <= m_max )
			    && (!defined(m_imin) || val.imag() >= m_imin) && (!defined(m_imax) || val.imag() <= m_imax);
		}
	}
	assert(false); throw std::logic_error("Can't happen");
}

