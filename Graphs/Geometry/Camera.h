#pragma once
#include "Matrix.h"
#include "Rotation.h"
#include "../../Persistence/Serializer.h"
#include "Axis.h"

#define DEGREES (M_PI/180.0)

class Camera : public Serializable
{
public:
	Camera() : fov(45*DEGREES), z(1.5), upright(false), ortho(false){ }

	void save(Serializer &s) const;
	void load(Deserializer &s);

	void viewport(int w, int h, Axis &axis); // set view port size in pixels
	void set(int antialias_pass = 0, int num_passes = 1) const; // sets the current GLView's projection matrix

	int  screen_w() const{ return w; }
	int  screen_h() const{ return h; }
	double aspect() const{ return hr; }
	double pixel_size(const Axis &axis) const{ return 2.0*axis.range(0) / w; } // => w*pixelsize = 2*xrange

	void reset();
	void zoom(double f);
	void rotate(double dx, double dy, double dz, bool free=false);
	void move(Axis &axis, double dx, double dy, double dz, bool free=false);

	bool orthogonal() const{ return ortho; }
	void orthogonal(bool o){ ortho = o; }
	void aligned(bool keep_upright){ upright = keep_upright; }

	double    dz() const{ return z*zf; }
	double  zoom() const{ return ortho ? 1.0 : z; }
	double   phi() const{ if (ortho) return 0.0; double ph,ps,th; rot.get(ph, ps, th); return ph/DEGREES; }
	double   psi() const{ if (ortho) return 0.0; double ph,ps,th; rot.get(ph, ps, th); return ps/DEGREES; }
	double theta() const{ if (ortho) return 0.0; double ph,ps,th; rot.get(ph, ps, th); return th/DEGREES; }
	void set_zoom (double x){ z = x; normalize(); }
	void set_phi  (double x){ if (!ortho){ double ph,ps,th; rot.get(ph, ps, th); rot.set(x*DEGREES, ps, th); } }
	void set_psi  (double x){ if (!ortho){ double ph,ps,th; rot.get(ph, ps, th); rot.set(ph, x*DEGREES, th); } }
	void set_theta(double x){ if (!ortho){ double ph,ps,th; rot.get(ph, ps, th); rot.set(ph, ps, x*DEGREES); } }
	void set_angles(double ph, double ps, double th){ if (!ortho){ rot.set(ph*DEGREES, ps*DEGREES, th*DEGREES); } }
	
	Quaternion quat() const { return rot.quat(); }
	void quat(const Quaternion &q) { rot.quat(q); }

	// project a point in axis-coordinates the same way that GL will, minus the pixel step, so [-1,1] fills the screen width
	// returns true if the point is not before the near clipping plane
	inline bool project(const P3f &p, P2f &pp) const;
	inline void rotate(const P3f &p, P3f &pp) const;
	inline bool scalefactor(float pz, float &f) const; // return true if pz is inside the z-clipping range
	inline P3f view_vector() const; // pointing from the eye into the scene
	
private:
	Rotation       rot;
	double         z;       // distance to 0
	double         fov;     // in degrees=45.0;
	int            w, h;    // current display port size in pixels
	double         hr;      // aspect ratio: h / w
	bool           upright;
	bool           ortho;
	mutable double zf;
	static const double znear, zfar;
	
	void normalize();
	
	void accFrustum(double left, double right, double bottom, double top, double near, double far,
					double pixdx, double pixdy, double eyedx, double eyedy, double focus) const;
	void accPerspective(double fovy, double aspect, double near, double far,
						double pixdx, double pixdy, double eyedx, double eyedy, double focus) const;
	void accOrtho(double left, double right, double bottom, double top, double near, double far, double pixdx, double pixdy) const;
};

inline bool Camera::project(const P3f &p, P2f &pp) const
{
	if (ortho)
	{
		pp.set(p.x, p.z);
		return true; // no z clipping
	}
	else
	{
		P3f q = rot(p);
		// next we apply the 90° rotation (which inverts y, then swaps y and z), translate and project in one step
		double qz = q.y + z*zf;
		bool visible = (qz > znear && qz < zfar);
		double f = zf / qz;
		pp.set((float)(q.x*f), (float)(q.z*f));
		return visible;
	}
}
inline void Camera::rotate(const P3f &p, P3f &pp) const // p = pp is ok
{
	if (ortho)
	{
		pp.set(p.x, p.z, -p.y);
	}
	else
	{
		P3f q = rot(p);
		// next we apply the 90° rotation (which inverts y, then swaps y and z)
		pp.set(q.x, q.z, -q.y);
	}
}
inline bool Camera::scalefactor(float pz, float &f) const
{
	if (ortho){ f = 1.0f; return true; }
	double qz = -pz + z*zf;
	f = (float)(zf / qz);
	return (qz > znear && qz < zfar);
}

P3f Camera::view_vector() const
{
	return !ortho ? (P3f)rot[P3d(0.0, 1.0, 0.0)] : P3f(0.0f, 1.0f, 0.0f);
}
