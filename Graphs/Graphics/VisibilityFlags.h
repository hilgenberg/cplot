#pragma once
#include "Info.h"

struct VisibilityFlags
{
	VisibilityFlags() : all(0) { }

	inline void set(const DI_Axis &ia, const P3f &p)
	{
		all = 0;
		if (ia.clipping)
		{
			x_min = (p.x < -1.0);
			x_max = (p.x >  1.0);
			y_min = (p.y < -ia.yh);
			y_max = (p.y >  ia.yh);
			z_min = (p.z < -ia.zh);
			z_max = (p.z >  ia.zh);
		}
	}

	inline void set_invalid(){ all = -1; }
	inline bool valid() const{ return all != -1; }

	inline bool operator*(VisibilityFlags b) const{ return (all & b.all) != 0; }

	static inline bool visible(VisibilityFlags a, VisibilityFlags b)
	{
		return !a.invalid && !b.invalid && !(a.all & b.all);
	}
	static inline bool visible(VisibilityFlags a, VisibilityFlags b, VisibilityFlags c)
	{
		return !a.invalid && !b.invalid && !c.invalid && !(a.all & b.all & c.all);
	}

private:
	
	union
	{
		char all; // if non-0, point is invisible, if a.all & b.all != 0, then the connecting line is invisible
		struct
		{
			bool x_min   : 1; // true if point is outside of range to the left
			bool x_max   : 1; // right
			bool y_min   : 1; // front
			bool y_max   : 1; // back
			bool z_min   : 1; // bottom
			bool z_max   : 1; // top
			bool invalid : 1; // !exists
		};
	};
};
