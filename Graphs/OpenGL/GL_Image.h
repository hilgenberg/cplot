#pragma once
#include "../../Persistence/Serializer.h"
#include "GL_Color.h"
#include "GL_RM.h"
#include <vector>

enum GL_ImagePattern
{
	IP_CUSTOM   = 0,
	IP_COLORS   = 1,
	IP_XOR      = 2,
	IP_XOR_GREY = 3,
	IP_CHECKER  = 4,
	IP_PLASMA   = 5,
	IP_COLORS_2 = 6,
	IP_PHASE    = 7,
	IP_UNITS    = 8
};
#define MAX_VALID_IMAGE_PATTERN_ID 8

struct GL_Image : public Serializable, public GL_Resource
{
	GL_Image() : _w(0), _h(0), _pattern(IP_CUSTOM), _opacity(1), _state(0){ }
	GL_Image(const GL_Image &i)
	: GL_Resource() // not copied
	, _state(0)
	, _w(i._w), _h(i._h), _data(i._data), _pattern(i._pattern), _opacity(i._opacity)
	{
		check_data();
	}
	
	#ifdef __linux__
	bool load(const std::string &path);
	#endif
	
	GL_Image &operator=(const GL_Image &x)
	{
		_w       = x._w;
		_h       = x._h;
		_data    = x._data;
		_pattern = x._pattern;
		_opacity = x._opacity;
		check_data();
		++_state;
		modify();
		return *this;
	}
	GL_Image &operator=(GL_ImagePattern p);

	GL_Image &swap(GL_Image &x)
	{
		std::swap(_w, x._w);
		std::swap(_h, x._h);
		std::swap(_data, x._data);
		std::swap(_pattern, x._pattern);
		std::swap(_opacity, x._opacity);
		++_state; ++x._state;
		modify(); x.modify();
		return *this;
	}

	bool operator==(const GL_Image &x) const
	{
		if (_pattern != x._pattern) return false;
		if (_pattern !=  IP_CUSTOM) return true;
		return _w == x._w && _h == x._h && _data == x._data;
	}

	inline void clear() { redim(0, 0); ++_state; }
	
	virtual void save(Serializer   &s) const;
	virtual void load(Deserializer &s);
	
	unsigned char *redim(unsigned w, unsigned h)
	{
		_w = w; _h = h;
		_data.resize(_w * _h * 4);
		_pattern = IP_CUSTOM;
		_opacity = -1;
		++_state;
		modify(); // even if w==w_ and h==h_ !
		return _data.data();
	}
	
	const std::vector<unsigned char> &data() const;

	unsigned w() const{ return _w; }
	unsigned h() const{ return _h; }
	bool empty() const{ return _w == 0 || _h == 0; }
	size_t state_counter() const { return _state; }
	
	GL_ImagePattern is_pattern() const{ return _pattern; }

	bool opaque() const
	{
		if (_opacity < 0)
		{
			assert(_pattern == IP_CUSTOM);
			_opacity = 1;
			const unsigned char *d = data().data();
			for (size_t i = 3, n = data().size(); i < n; i += 4)
			{
				if ((unsigned char)(d[i]+1) > 1)
				{
					_opacity = 0; break;
				}
			}
		}
		return _opacity > 0;
	}
	
	void prettify(bool circle=false);
	
	void mix(const GL_Color &base, float alpha, std::vector<unsigned char> &dst) const;
	void mix(float alpha, std::vector<unsigned char> &dst) const;
	
private:
	unsigned                   _w, _h;   // width and height
	GL_ImagePattern            _pattern; // to save space when serializing computed patterns
	std::vector<unsigned char> _data;    // rgba, size = 4*w*h or empty for patterns
	mutable short              _opacity; // -1 = unknown, 0 = transparent, 1 = opaque
	size_t                     _state;   // incremented on every modification

	inline void check_data() const
	{
		assert(_data.size() == (_pattern == IP_CUSTOM ? (size_t)_w * _h * 4 : 0));
	}
	
	static GL_Image &pattern(GL_ImagePattern p);
};
