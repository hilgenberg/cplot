#pragma once
#include <vector>
#include <cassert>
#include <cstddef>

#ifdef DEBUG
//#define PARSER_DEBUG
#endif

template<class T> class RetainTree // T is the derived class to avoid typecasting all the time
{
public:
	RetainTree() : retain_count(0){ } // creates an empty tree
	
	virtual ~RetainTree()
	{
		assert(retain_count == 0);
		for (T *c : children) c->release();
	}
	
	virtual void reset() // clear children
	{
		assert(retain_count <= 1);
		for (T *c : children) c->release();
		children.clear();
	}
	
	inline T *retain(){ ++retain_count; return (T*)this; }
	inline void release(){ assert(retain_count > 0); if (--retain_count == 0) delete this; }
	inline void release_dont_delete(bool exact = true){ --retain_count; assert(!exact || retain_count == 0); }
	inline void collect(){ if (retain_count == 0) delete this; } // same as retain+release
	
	int retainCount() const{ return retain_count; }
	
	#ifdef DEBUG
	bool check_retained(bool exact) const
	{
		assert(exact ? retain_count == 1 : retain_count >= 1);
		for (const T *c : children) if (!c->check_retained(exact)) return false;
		return (exact ? retain_count == 1 : retain_count >= 1);
	}
	#endif
	
	void add_child(T *c){ children.push_back(c->retain()); }
	int  num_children() const{ return (int)children.size(); }
	T *child(int i){ assert(i >= 0 && (size_t)i < children.size()); return children[i]; }
	const T *child(int i) const{ assert(i >= 0 && (size_t)i < children.size()); return children[i]; }
	
	typedef typename std::vector<T*>::const_iterator const_iterator;
	inline const_iterator begin() const{ return children.begin(); }
	inline const_iterator   end() const{ return children.end();   }
	
protected:
	int retain_count;
	std::vector<T*> children;
	
	void child(int i, T *c)
	{
		assert(i >= 0 && i < num_children());
		assert(c);
		T *c0 = children[i];
		children[i] = c->retain();
		c0->release();
	}
};
