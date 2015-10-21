#include "RecursiveGrid.h"
#include <queue>
#include <cstring>

//----------------------------------------------------------------------------------------------------------------------
// Constructors
//----------------------------------------------------------------------------------------------------------------------

RecursiveGrid_2D::RecursiveGrid_2D(size_t nx, size_t ny, unsigned char d)
: nx(nx), ny(ny)
, nnx((nx+(1<<d)-1)>>d)
, nny((ny+(1<<d)-1)>>d)
, depth(d)
, base(new Cells* [nnx*nny])
{
	assert(depth > 0);
	assert(nnx<<depth >= nx);
	assert(nny<<depth >= ny);
	memset(base.get(), 0, nnx*nny*sizeof(Cells*));
}

RecursiveGrid_3D::RecursiveGrid_3D(size_t nx, size_t ny, size_t nz, unsigned char d)
: nx(nx), ny(ny), nz(nz)
, nnx((nx+(1<<d)-1)>>d)
, nny((ny+(1<<d)-1)>>d)
, nnz((nz+(1<<d)-1)>>d)
, depth(d)
, base(new Cells* [nnx*nny*nnz])
{
	assert(depth > 0);
	assert(nnx<<depth >= nx);
	assert(nny<<depth >= ny);
	assert(nnz<<depth >= nz);
	memset(base.get(), 0, nnx*nny*nnz*sizeof(Cells*));
}

//----------------------------------------------------------------------------------------------------------------------
// get
//----------------------------------------------------------------------------------------------------------------------

bool RecursiveGrid_2D::get(size_t x, size_t y) const
{
	if (x >= nx || y >= ny) return false;
	Cells **b = base.get() + ((y>>depth)*nnx + (x>>depth));
	for (int d = depth-1; d > 0; --d)
	{
		if (!*b) return false;
		b = (*b)->sub + (int)(((x>>d) & 1) | (((y>>d) & 1) << 1));
	}
	int i = ((int)x & 1) | (((int)y & 1) << 1);
	return (*(unsigned*)b >> i) & 1;
}

bool RecursiveGrid_3D::get(size_t x, size_t y, size_t z) const
{
	if (x >= nx || y >= ny || z >= nz) return false;
	Cells **b = base.get() + ((z>>depth)*nnx*nny + (y>>depth)*nnx + (x>>depth));
	for (int d = depth-1; d > 0; --d)
	{
		if (!*b) return false;
		b = (*b)->sub + (int)(((x>>d) & 1) | (((y>>d) & 1) << 1) | (((z>>d) & 1) << 2));
	}
	int i = ((int)x & 1) | (((int)y & 1) << 1) | (((int)z & 1) << 2);
	return (*(unsigned*)b >> i) & 1;
}

//----------------------------------------------------------------------------------------------------------------------
// set
//----------------------------------------------------------------------------------------------------------------------

void RecursiveGrid_2D::set(size_t x, size_t y)
{
	assert(x < nx && y < ny);
	Cells **b = base.get() + ((y>>depth)*nnx + (x>>depth));
	for (int d = depth-1; d > 0; --d)
	{
		if (!*b) *b = new(pool) Cells;
		b = (*b)->sub + (int)(((x>>d) & 1) | (((y>>d) & 1) << 1));
	}
	int j = ((int)x & 1) | (((int)y & 1) << 1);
	*(size_t*)b |= 1 << j;
}


void RecursiveGrid_3D::set(size_t x, size_t y, size_t z)
{
	assert(x < nx && y < ny && z < nz);
	Cells **b = base.get() + ((z>>depth)*nnx*nny + (y>>depth)*nnx + (x>>depth));
	for (int d = depth-1; d > 0; --d)
	{
		if (!*b) *b = new(pool) Cells;
		b = (*b)->sub + (int)(((x>>d) & 1) | (((y>>d) & 1) << 1) | (((z>>d) & 1) << 2));
	}
	int j = ((int)x & 1) | (((int)y & 1) << 1) | (((int)z & 1) << 2);
	*(size_t*)b |= 1 << j;
}

//----------------------------------------------------------------------------------------------------------------------
// get_range - delegates to get_subblock for every top-level cell in the range
//----------------------------------------------------------------------------------------------------------------------

bool RecursiveGrid_2D::get_range(size_t xm, size_t ym, int range) const
{
	if (range <= 0) return range < 0 ? false : get(xm,ym);
	
	size_t x0 = xm>(size_t)range ? xm-range : 0, x1 = std::min(xm+range+1, nx);
	size_t y0 = ym>(size_t)range ? ym-range : 0, y1 = std::min(ym+range+1, ny);
	
	for (size_t x = x0, xx = ((x >> depth) << depth) + (1<<depth); x < x1; x = xx, xx += 1<<depth)
	{
		for (size_t y = y0, yy = ((y >> depth) << depth) + (1<<depth); y < y1; y = yy, yy += 1<<depth)
		{
			if (get_subblock(x,y, std::min(xx,x1),std::min(yy,y1))) return true;
		}
	}
	return false;
}

bool RecursiveGrid_3D::get_range(size_t xm, size_t ym, size_t zm, int range) const
{
	if (range <= 0) return range < 0 ? false : get(xm,ym,zm);

	size_t x0 = xm>(size_t)range ? xm-range : 0, x1 = std::min(xm+range+1, nx);
	size_t y0 = ym>(size_t)range ? ym-range : 0, y1 = std::min(ym+range+1, ny);
	size_t z0 = zm>(size_t)range ? zm-range : 0, z1 = std::min(zm+range+1, nz);
	
	for (size_t x = x0, xx = ((x >> depth) << depth) + (1<<depth); x < x1; x = xx, xx += 1<<depth)
	{
		for (size_t y = y0, yy = ((y >> depth) << depth) + (1<<depth); y < y1; y = yy, yy += 1<<depth)
		{
			for (size_t z = z0, zz = ((z >> depth) << depth) + (1<<depth); z < z1; z = zz, zz += 1<<depth)
			{
				if (get_subblock(x,y,z, std::min(xx,x1),std::min(yy,y1),std::min(zz,z1))) return true;
			}
		}
	}
	return false;
}

//----------------------------------------------------------------------------------------------------------------------
// static helper functions for get_subblock
//----------------------------------------------------------------------------------------------------------------------

static inline bool contains(size_t x, size_t y, size_t z, int w, size_t x0, size_t y0, size_t z0, size_t x1, size_t y1, size_t z1)
{
	return (x >= x0 && x+w <= x1 &&
			y >= y0 && y+w <= y1 &&
			z >= z0 && z+w <= z1);
}
static inline bool intersects(size_t x, size_t y, size_t z, int w, size_t x0, size_t y0, size_t z0, size_t x1, size_t y1, size_t z1)
{
	return (x < x1 && x0 < x+w &&
			y < y1 && y0 < y+w &&
			z < z1 && z0 < z+w);
}
static inline bool contains(size_t x, size_t y, size_t z, size_t x0, size_t y0, size_t z0, size_t x1, size_t y1, size_t z1)
{
	return (x >= x0 && x < x1 &&
			y >= y0 && y < y1 &&
			z >= z0 && z < z1);
}

static inline bool contains(size_t x, size_t y, int w, size_t x0, size_t y0, size_t x1, size_t y1)
{
	return (x >= x0 && x+w <= x1 &&
			y >= y0 && y+w <= y1);
}
static inline bool intersects(size_t x, size_t y, int w, size_t x0, size_t y0, size_t x1, size_t y1)
{
	return (x < x1 && x0 < x+w &&
			y < y1 && y0 < y+w);
}
static inline bool contains(size_t x, size_t y, size_t x0, size_t y0, size_t x1, size_t y1)
{
	return (x >= x0 && x < x1 &&
			y >= y0 && y < y1);
}

//----------------------------------------------------------------------------------------------------------------------
// get_subblock 2D
//----------------------------------------------------------------------------------------------------------------------

bool RecursiveGrid_2D::get_subblock(size_t x0, size_t y0, size_t x1, size_t y1) const
{
	assert(x0 < x1 && y0 < y1);
	assert(x1-x0 <= 1U<<depth);
	assert(y1-y0 <= 1U<<depth);
	
	const Cells *b = base[(y0>>depth)*nnx + (x0>>depth)];
	
	if (x1-x0 == 1U<<depth && y1-y0 == 1U<<depth) return b;
	
	if (!b) return false;
	
	size_t mask = (1 << depth)-1;
	x0 &= mask; x1 &= mask; if (!x1) x1 = 1 << mask;
	y0 &= mask; y1 &= mask; if (!y1) y1 = 1 << mask;
	
	struct Info
	{
		Info(size_t x, size_t y, const Cells *b, int d) : x(x), y(y), b(b), d(d){}
		size_t x,y;
		const Cells *b;
		int    d;
	};
	std::queue<Info> todo;
	for (todo.emplace(0,0,b,depth-1); !todo.empty(); todo.pop())
	{
		Info &info = todo.front();
		if (info.d > 0)
		{
			int w = 1 << info.d;
#define CHK(i,dx,dy) do{ if (info.b->sub[i]){ \
if (contains(info.x+dx, info.y+dy, w, x0,y0,x1,y1)) return true; /*since info.b->sub[i] != NULL */ \
if (intersects(info.x+dx, info.y+dy, w, x0,y0,x1,y1)) todo.emplace(info.x+dx, info.y+dy, info.b->sub[i], info.d-1); \
}}while(0)
			CHK(0,0,0);
			CHK(1,w,0);
			CHK(2,0,w);
			CHK(3,w,w);
#undef CHK
		}
		else
		{
#define CHK(i,dx,dy) if ((((size_t)info.b >> i) & 1) && \
contains(info.x+dx, info.y+dy, x0,y0,x1,y1)) return true
			CHK(0,0,0);
			CHK(1,1,0);
			CHK(2,0,1);
			CHK(3,1,1);
#undef CHK
		}
	}
	return false;
}

//----------------------------------------------------------------------------------------------------------------------
// get_subblock 3D
//----------------------------------------------------------------------------------------------------------------------

bool RecursiveGrid_3D::get_subblock(size_t x0, size_t y0, size_t z0, size_t x1, size_t y1, size_t z1) const
{
	assert(x0 < x1 && y0 < y1 && z0 < z1);
	assert(x1-x0 <= 1U<<depth);
	assert(y1-y0 <= 1U<<depth);
	assert(z1-z0 <= 1U<<depth);
	
	const Cells *b = base[(z0>>depth)*nnx*nny + (y0>>depth)*nnx + (x0>>depth)];
	
	if (x1-x0 == 1U<<depth && y1-y0 == 1U<<depth && z1-z0 == 1U<<depth) return b;

	if (!b) return false;

	size_t mask = (1 << depth)-1;
	x0 &= mask; x1 &= mask; if (!x1) x1 = 1 << mask;
	y0 &= mask; y1 &= mask; if (!y1) y1 = 1 << mask;
	z0 &= mask; z1 &= mask; if (!z1) z1 = 1 << mask;
	
	struct Info
	{
		Info(size_t x, size_t y, size_t z, const Cells *b, int d) : x(x), y(y), z(z), b(b), d(d){}
		size_t x,y,z;   // top-left-front corner of this subblock
		const Cells *b; // the subblock
		int    d;       // its depth
	};
	std::queue<Info> todo;
	for (todo.emplace(0,0,0,b,depth-1); !todo.empty(); todo.pop())
	{
		Info &info = todo.front();
		if (info.d > 0)
		{
			int w = 1 << info.d;
#define CHK(i,dx,dy,dz) do{ if (info.b->sub[i]){ \
if (contains(info.x+dx, info.y+dy, info.z+dz, w, x0,y0,z0,x1,y1,z1)) return true; /*since b->sub[i] != NULL */ \
if (intersects(info.x+dx, info.y+dy, info.z+dz, w, x0,y0,z0,x1,y1,z1)) todo.emplace(info.x+dx, info.y+dy, info.z+dz, info.b->sub[i], info.d-1); \
}}while(0)
			CHK(0,0,0,0);
			CHK(1,w,0,0);
			CHK(2,0,w,0);
			CHK(3,w,w,0);
			CHK(4,0,0,w);
			CHK(5,w,0,w);
			CHK(6,0,w,w);
			CHK(7,w,w,w);
#undef CHK
		}
		else
		{
#define CHK(i,dx,dy,dz) if ((((size_t)info.b >> i) & 1) && \
contains(info.x+dx, info.y+dy, info.z+dz, x0,y0,z0,x1,y1,z1)) return true
			CHK(0,0,0,0);
			CHK(1,1,0,0);
			CHK(2,0,1,0);
			CHK(3,1,1,0);
			CHK(4,0,0,1);
			CHK(5,1,0,1);
			CHK(6,0,1,1);
			CHK(7,1,1,1);
#undef CHK
		}
	}
	return false;
}

//----------------------------------------------------------------------------------------------------------------------
// printing
// @todo way too slow, even for debugging
//----------------------------------------------------------------------------------------------------------------------

std::ostream &operator<<(std::ostream &out, const RecursiveGrid_2D &g)
{
	bool first = true;
	out << "G2D[";
	for (size_t y = 0; y < g.ny; ++y)
	{
		for (size_t x = 0; x < g.nx; ++x)
		{
			if (!g.get(x,y)) continue;
			if (!first) out << ", "; first = false;
			out << x << "-" << y;
		}
	}
	out << "]";
	return out;
}

std::ostream &operator<<(std::ostream &out, const RecursiveGrid_3D &g)
{
	bool first = true;
	out << "G3D[";
	for (size_t z = 0; z < g.nz; ++z)
	{
		for (size_t y = 0; y < g.ny; ++y)
		{
			for (size_t x = 0; x < g.nx; ++x)
			{
				if (!g.get(x,y,z)) continue;
				if (!first) out << ", "; first = false;
				out << x << "-" << y << "-" << z;
			}
		}
	}
	out << "]";
	return out;
}
