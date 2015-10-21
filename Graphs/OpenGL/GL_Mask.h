#pragma once
#include "../../Persistence/Serializer.h"
#include "GL_RM.h"
#include <vector>

enum MaskStyle
{
	Mask_Custom       = -1,
	Mask_Off          =  0,
	Mask_Circles      =  1,
	Mask_Squares      =  2,
	Mask_Triangles    =  3,
	Mask_Rounded_Rect =  4,
	Mask_Chessboard   =  5,
	Mask_HLines       =  6,
	Mask_VLines       =  7,
	Mask_Rings        =  8,
	Mask_Static       =  9,
	Mask_Fan          = 10,
	Mask_Hexagon      = 11
};
#define CUSTOM_MASK_BUTTON_INDEX 12 /* in the settings box popup button */

struct GL_MaskScale
{
	GL_MaskScale() : sx(1.0), sy(1.0), dx(0.0), dy(0.0){ }
	
	void set(double sx_, double sy_, double dx_, double dy_)
	{
		sx = sx_;
		sy = sy_;
		dx = dx_;
		dy = dy_;
	}
	
	void reset(){ set(1.0, 1.0, 0.0, 0.0); }
	
	double sx, sy, dx, dy;
};

class GL_Mask : public Serializable, public GL_Resource
{
public:
	GL_Mask() : _style(Mask_Off), _density(0.5), _update(false), _w(0), _h(0){ }
	
	explicit GL_Mask(const GL_Mask &m)
	: _style(m._style), _density(m._density), _update(m._update), _w(m._w), _h(m._h)
	, GL_Resource()
	{
		if (custom()) _data = m._data; else _update = true;
	}
	explicit GL_Mask(GL_Mask &&m) : _style(m._style), _density(m._density), _update(m._update), _w(m._w), _h(m._h),
	_data(std::move(m._data))
	{
		m._style = Mask_Off;
		m._w = m._h = 0;
		m.modify();
	}
	GL_Mask & operator= (const GL_Mask &m)
	{
		if (&m == this) return *this;
		_style = m._style;
		_density = m._density;
		_update = m._update;
		_w = m._w; _h = m._h;
		_data = m._data;
		modify();
		return *this;
	}

	bool operator== (const GL_Mask &x) const
	{
		if (_style != x._style || fabs(_density - x._density) > 1e-12) return false;
		if (custom()) return _w == x._w && _h == x._h && _data == x._data;
		return true;
	}

	virtual void save(Serializer   &s) const;
	virtual void load(Deserializer &s);

	void upload(GL_RM &rm, const GL_MaskScale &scale) const; // transfer to GPU
	
	double  density() const{ return _density; }
	MaskStyle style() const{ return _style;   }

	unsigned  w() const{ return _w; }
	unsigned  h() const{ return _h; }
	bool  empty() const{ return _w == 0 || _h == 0; }
	bool custom() const{ return _style == Mask_Custom; }
	const std::vector<unsigned char> &data() const;

	void density(double d)
	{
		if (fabs(d - _density) <= 1e-12) return;
		_density = d;
		if (!custom())
		{
			_update = true;
			update_size();
		}
		modify();
	}

	void style(MaskStyle s)
	{
		if (s == _style) return;
		assert(s != Mask_Custom);
		_style = s;
		_update = true;
		update_size();
		modify();
	}

	unsigned char *redim(unsigned w, unsigned h)
	{
		_w = w; _h = h;
		_data.resize(_w * _h);
		_style = Mask_Custom;
		_update = false;
		modify(); // even if w==w_ and h==h_ !
		return _data.data();
	}

private:
	friend class GL_RM;
	void mix(std::vector<unsigned char> &dst) const;

	double    _density;
	MaskStyle _style;

	mutable std::vector<unsigned char> _data; // alpha map
	mutable unsigned _w, _h;
	mutable bool _update;
	void update_size();
	inline void check_data() const
	{
		assert(_data.size() == (size_t)_w * _h);
	}

};
