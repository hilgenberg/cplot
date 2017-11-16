#pragma once

#include "Namespace.h"

/**
 * @addtogroup Namespace
 * @{
 */

enum ParameterType
{
	Real         = 0,
	Complex      = 1,
	Angle        = 2,
	ComplexAngle = 3,
	Integer      = 4
};

/**
 * Symbol with a changeable value.
 */

class Parameter : public Element, public IDCarrier
{
public:
	Parameter(const std::string &name = "", ParameterType type = Real, const cnum &value = 0.0);
	Parameter &operator=(const Parameter &p);
	
	virtual void save(Serializer   &s) const;
	virtual void load(Deserializer &s);
	virtual TypeCode type_code() const{ return TypeCode::PARAMETER; }

	virtual int  arity() const{ return -1; }
	virtual bool builtin() const{ return false; }
	virtual bool isParameter() const{ return true; }
	
	ParameterType type() const{ return m_type; }
	void type(ParameterType t);
	bool is_real() const{ return m_type != Complex && m_type != ComplexAngle; }
	inline Range range() const
	{
		Range r = R_Complex;
		switch (m_type)
		{
			case Complex:
				if (m_rmax <= 1.0 ||
					defined(m_imin) && defined(m_imax) && defined(m_min) && defined(m_max) &&
					hypot(m_min, m_imin) <= 1.0 && hypot(m_max, m_imin) <= 1.0 &&
					hypot(m_min, m_imax) <= 1.0 && hypot(m_max, m_imax) <= 1.0) r |= R_Unit;
				break;
			case Angle:        return R_Real;
			case ComplexAngle: return R_Unit;
			case Integer:
				r = R_Integer;
				if (m_min > -1.0) r |= R_NonNegative;
				if (m_min >  0.0) r |= R_Positive;
				break;
			case Real:
				r = R_Real;
				if (m_min >= 0.0) r |= R_NonNegative;
				if (m_min >  0.0) r |= R_Positive;
				if (m_min >= -1.0 && m_max <= 1.0) r |= R_Unit;
				break;
		}
		return r;
	}
	
	// the value setters set to the nearest value that is within constraints or
	// to NAN if there are no valid values (min > max and such)

	cnum  value() const{ return v; }
	void  value(const cnum &v);
	bool  value_ok(const cnum &v) const; // in constraints?

	// value = rvalue + i*ivalue
	double rvalue() const{ return m_type == ComplexAngle ? atan2(v.imag(), v.real()) : v.real(); }
	double ivalue() const{ return m_type == ComplexAngle ? 0.0 : v.imag(); }
	void rvalue(double x){ value(m_type == ComplexAngle ? cnum(cos(x), sin(x)) : cnum(x, v.imag())); }
	void ivalue(double x){ if (m_type != ComplexAngle) value(cnum(v.real(), x)); }
	
	double  min() const{ return m_min;  }
	double  max() const{ return m_max;  }
	double imin() const{ return m_imin; }
	double imax() const{ return m_imax; }
	double rmax() const{ return m_rmax; }
	void  min(double x){ m_min  = x; value(value()); }
	void  max(double x){ m_max  = x; value(value()); }
	void imin(double x){ m_imin = x; value(value()); }
	void imax(double x){ m_imax = x; value(value()); }
	void rmax(double x){ m_rmax = x; value(value()); }

	bool angle_in_radians() const{ assert(radians || m_type != ComplexAngle); return radians; }
	void angle_in_radians(bool f){ if (radians != f){ radians = f; } }

	struct Animation: public Serializable
	{
		enum Type
		{
			Linear,   // v0 --> v0 + (v1-v0) --> v0 + 2(v1-v0) --> ...
			Saw,      // v0 --> v1, v0 --> v1, ...
			PingPong, // v0 --> v1 --> v0 --> v1 --> ...
			Sine      // PingPong with smooth reverses
		};
		Type type;

		double t0;     // reference time, value(t0) == v0
		double T;      // when is value(t0 + T) == v1 for the first time?
		cnum   v0, v1; // reference values (min,max f.e. or 0, 2pi)
		cnum   v00;    // value to reset to when animation stops
		bool   repeat; // don't stop at v1?
		mutable bool running;

		Animation() : T(UNDEFINED) , v0(UNDEFINED), v1(UNDEFINED), repeat(true) {}

		cnum operator()(double t) const;
		operator bool() const { return running; }
		bool match(); // try to find t0' with value(t0) == v00

		virtual void save(Serializer   &s) const;
		virtual void load(Deserializer &s);
	};
	Animation anim;
	bool anim_start();  // returns false if anim contains garbage
	void anim_stop();
	bool animate(double t); // returns true if value changed (can be false for int params)

#ifdef DEBUG
	virtual void dump(std::ostream &o) const{ o << "Parameter " << name(); }
#endif

protected:
	virtual Element *copy() const;

	cnum v;
	double m_min, m_max, m_imin, m_imax, m_rmax;
	ParameterType m_type;
	bool radians;
};

/** @} */

