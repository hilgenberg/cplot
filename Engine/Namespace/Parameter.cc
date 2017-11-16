#include "Parameter.h"
#include "Expression.h"
#include "../../Utility/System.h"

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

//--- Animation ---------------------------------------------------------------------------

cnum Parameter::Animation::operator()(double t) const
{
	double dt = (t - t0) / T, r;
	if (!repeat && dt > 1.0) { running = false; dt = 1.0; }
	switch (type)
	{
		case Saw:      if (repeat) dt = modf(dt, &r); break;
		case Linear:   break;
		case PingPong: dt = modf(dt / 2.0, &r) * 2.0; if (dt > 1.0) dt = 2.0 - dt; break;
		case Sine:     dt = (1.0 - cos(M_PI*dt)) / 2.0; break;
	}
	return v0 + dt * (v1 - v0);
}

void Parameter::Animation::save(Serializer &s) const
{
	if (s.version() < FILE_VERSION_1_11) return;
	s.double_(T);
	s.cnum_(v0);
	s.cnum_(v1);
	s.bool_(repeat);
}
void Parameter::Animation::load(Deserializer &s)
{
	if (s.version() < FILE_VERSION_1_11)
	{
		T = UNDEFINED;
		v0 = v1 = UNDEFINED;
		repeat = true;
		return;
	}

	s.double_(T);
	s.cnum_(v0);
	s.cnum_(v1);
	s.bool_(repeat);
}

bool Parameter::Animation::match()
{
	cnum cdt = (v00 - v0) / (v1 - v0);
	if (!::is_real(cdt)) return false;
	double dt = cdt.real();

	if (type != Linear)
	{
		if (dt <      -EPSILON) return false;
		if (dt > 1.0 + EPSILON) return false;
		if (dt < 0.0) dt = 0.0;
		if (dt > 1.0) dt = 1.0;
	}

	if (type == Sine)
	{
		dt = acos(1.0 - dt*2.0) / M_PI;
		assert(defined(dt));
	}

	t0 -= dt * T;
	return true;
}

bool Parameter::anim_start()
{
	if (anim.running) return false;

	switch (type())
	{
		case ComplexAngle: anim.v0 = 0.0; anim.v1 = 2.0*M_PI; anim.T = 2.0; anim.type = Animation::Saw; break;
		case Angle: anim.v0 = 0.0; anim.v1 = radians ? 2.0*M_PI : 360.0; anim.T = 2.0; anim.type = Animation::Saw; break;
		case Integer:
			anim.v0 = defined(m_min) ? m_min : 0.0;
			anim.v1 = defined(m_max) ? m_max : 1.0;
			anim.T  = 1.0;
			anim.type = (defined(m_min) && defined(m_max)) ? Animation::PingPong : Animation::Linear;
			break;
		case Real:
		case Complex:
			anim.v0 = defined(m_min) ? m_min : 0.0;
			anim.v1 = defined(m_max) ? m_max : 1.0;
			anim.T = 1.0;
			anim.type = (defined(m_min) && defined(m_max)) ? Animation::Sine : Animation::Linear;
			break;
	}
	anim.repeat = true;// (defined(m_min) && defined(m_max));

	if (!defined(anim.v0) || !defined(anim.v1)) return false;
	// at least one of the reference points should be in range
	if (!value_ok(anim.v0) && !value_ok(anim.v1)) return false;
	if (eq(anim.v0, anim.v1)) switch (type())
	{
		case Angle:        anim.v1 += radians ? 2.0*M_PI : 360.0; break;
		case ComplexAngle: anim.v1 += 2.0*M_PI; break;
		default:           value(anim.v0); return false;
	}

	if (!defined(anim.T) || fabs(anim.T < 0.001)) return false;

	if (anim.T < 0.0)
	{
		anim.T *= -1.0;
		std::swap(anim.v0, anim.v1);
	}

	anim.v00 = value();
	anim.running = true;
	anim.t0 = now();
	if (anim.repeat) anim.match();
	return true;
}

void Parameter::anim_stop()
{
	if (anim.running)
	{
		// reset to original value iff user hit the stop button
		value(anim.v00);
		anim.running = false;
	}
}

bool Parameter::animate(double t)
{
	cnum v0 = value();
	if (type() == ComplexAngle)
	{
		rvalue(anim(t).real());
	}
	else
	{
		value(anim(t));
	}
	return value() != v0;
}
