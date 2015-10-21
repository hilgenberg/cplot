#pragma once
#include "../../Persistence/Serializer.h"
#include "GL_Color.h"

struct GL_BlendMode : public Serializable
{
	GL_BlendMode();
	
	virtual void save(Serializer   &s) const;
	virtual void load(Deserializer &s);

	void set() const;

	int fA, fB;
	GL_Color C;
	bool off;
	
	bool usesConstColor() const; // false if only const alpha is used! but if true, usesConstAlpha is true too
	bool usesConstAlpha() const;
	bool symmetric() const{ return symmetric(NULL); }
	std::string symmetry_info() const;
	bool forces_transparency() const; // will the background come through even if alpha=1?
	
	bool operator== (const GL_BlendMode &m) const
	{
		if (off && m.off) return true;
		if (off != m.off) return false;
		assert(!off && !m.off);
		if (fA != m.fA || fB != m.fB) return false;
		if (usesConstColor()) return C == m.C;
		if (usesConstAlpha()) return fabsf(C.a-m.C.a) < 1e-12f;
		return true;
	}
	bool operator!= (const GL_BlendMode &m) const
	{
		return !operator==(m);
	}
	
private:
	bool symmetric(std::ostream *os) const;
};

struct GL_DefaultBlendMode
{
	std::string  name;
	GL_BlendMode mode;
};

const std::vector<GL_DefaultBlendMode> &DefaultBlendModes();
