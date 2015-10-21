#include "Namespace.h"
#include "Function.h"
#include "Constant.h"
#include "BaseFunction.h"
#include "RootNamespace.h"
#include <string>
#include <cstring>
#include <cctype>
#include <vector>
#include <map>
#include <cassert>
#include "../Parser/utf8/utf8.h"

//----------------------------------------------------------------------------------------------------------------------
// Namespace
//----------------------------------------------------------------------------------------------------------------------

Namespace::~Namespace()
{
	for (auto &p : named){ p.second->ns = NULL; delete p.second; }
	for (Element *p :  nameless){ p->ns = NULL; delete p; }
	for (Element *p :    linked){ p->ns = NULL; }
}

void Namespace::save(Serializer &s) const
{
	if (s.version() < FILE_VERSION_1_10)
	{
		// check that there are no duplicate (name, a1), (name, a2) entries
		std::set<std::string> n;
		for (auto &p : named)
		{
			Element *x = p.second;
			if (!x || x->builtin()) continue;
			if (n.count(x->name())) NEEDS_VERSION(FILE_VERSION_1_10, "overloaded functions");
			n.insert(x->name());
		}
	}
	
	Element::save(s);
	for (auto &p : named)
	{
		Element *x = p.second;
		if (!x || x->builtin()) continue;
		s._object(x);
	}
	for (Element *x : nameless)
	{
		if (x) s._object(x);
	}
	s._object(NULL);
}
void Namespace::load(Deserializer &s)
{
	clear(); // RootNamespace must override this
	Element::load(s);

	while (true)
	{
		Serializable *x;
		s._object(x);
		if (!x) break;
		add((Element*)x);
	}
}

Element *Namespace::copy() const
{
	assert(false); // should not be called yet
	
	Namespace *n = new Namespace;
	try
	{
		for (const Element *e : *this)
		{
			e->copy(*n);
		}
	}
	catch(...)
	{
		delete n;
		throw;
	}
	return n;
}

static inline bool valid_name_char(int c)
{
	return !isspace(c) && !iscntrl(c) && !strchr("+-*/^%~!&?()[]{},;<>°\"'|¹²³", c) && (c < 0x2070 || c > 0x2079 /*exponents*/);
}
bool Namespace::valid_name(const std::string &name)
{
	if (name.empty() || !utf8::is_valid(name.begin(), name.end())) return false;
	
	auto i = name.begin();

	int c0 = utf8::next(i, name.end());
	if (isdigit(c0) || !valid_name_char(c0)) return false;
	
	while (i != name.end()) if (!valid_name_char(utf8::next(i, name.end()))) return false;
	return true;
}

std::string Namespace::unique_name(const std::string &basename, int arity) const
{
	std::string s;
	utf8::replace_invalid(basename.begin(), basename.end(), back_inserter(s));
	
	if (!valid_name(s))
	{
		std::vector<int> t;
		utf8::utf8to32(s.begin(), s.end(), back_inserter(t));
		
		bool changed = false;
		for (int &c : t)
		{
			if (!valid_name_char(c))
			{
				c = '_';
				changed = true;
			}
		}
		
		if (changed)
		{
			s = "";
			utf8::utf32to8(t.begin(), t.end(), back_inserter(s));
		}
	}
	if (!valid_name(s))
	{
		s = "a";
		size_t len = 1;
		while(find(s, false))
		{
			size_t i = len-1;
			while (s[i] == 'z')
			{
				s[i] = 'a';
				if (i == 0)
				{
					s.insert(s.begin(), 'a'-1);
					++len;
				}
				else
				{
					--i;
				}
			}
			++s[i];
		}
		return s;
	}
	
	if (!find(s, arity, false)) return s;
	
	std::vector<int> t;
	utf8::utf8to32(s.begin(), s.end(), back_inserter(t));

	size_t len = t.size(), numlen = 0;
	bool asc = false; // numeric suffix is ASCII or UTF-subscript?
	int sub0 = 0x2080, sub9 = 0x2089; // subscript 0 and 9
	if (isdigit(t[len-1]))
	{
		asc = true;
		numlen = 1;
		while(numlen < len && isdigit(t[len-1-numlen])) ++numlen;
	}
	else if(t[len-1] >= sub0 && t[len-1] <= sub9)
	{
		asc = false;
		numlen = 1;
		while(numlen < len && t[len-1-numlen] >= sub0 && t[len-1-numlen] <= sub9) ++numlen;
	}
	
	size_t i0 = len-1-numlen; // index of last nonnumeric char
	assert(i0 < len); // otherwise the second "if (!valid_name(s))" would have caught it
	do
	{
		size_t i = len-1;
		while (i > i0 && (asc && t[i] == '9' || !asc && t[i] == sub9))
		{
			t[i--] = asc ? '0' : sub0;
		}
		if (i == i0)
		{
			t.insert(t.begin() + ++i, asc ? (int)'0' : (int)sub0);
			++len; ++numlen;
		}
		++t[i];
		s = "";
		utf8::utf32to8(t.begin(), t.end(), back_inserter(s));
	}
	while(find(s, arity, false));
	return s;
}

void Namespace::add(Element *object)
{
	if (!object){ assert(false); return; }
	
	if(object->ns == this) return;
	
	if(object->ns) object->ns->remove(object);
	
	#ifdef DEBUG
	assert(nameless.count(object) == 0);
	for (auto &i : named) assert(i.second != object);
	#endif
	
	if (find(object->nm, object->arity(), false))
	{
		object->nm = unique_name(object->nm, object->arity());
	}

	object->ns = this;

	if (valid_name(object->nm))
	{
		named[std::make_pair(object->nm, object->arity())] = object;
		redefine(object);
	}
	else
	{
		nameless.insert(object);
	}

	object->added_to_namespace();
}

void Namespace::add_builtin(Element *object)
{
	assert(object && !object->ns);
	assert(named.find(std::make_pair(object->nm, object->arity())) == named.end());
	named[std::make_pair(object->nm, object->arity())] = object;
	object->ns = this;
}

void Namespace::link(Namespace *parent)
{
	assert(!ns);
	assert(nm.empty());
	assert(parent != this);
	ns = parent;
	if (parent) parent->linked.insert(this);
}

void Namespace::remove(Element *object)
{
	// called from ~Element - must not call pure virtuals!
	if (!object){ assert(false); return; }
	assert(object->ns == this);
	if (object->ns == this) object->ns = NULL;

	bool found = false;
	if (object->nm.length() && valid_name(object->nm))
	{
		for (auto &i : named)
		{
			if (i.second == object)
			{
				found = true;
				named.erase(i.first);
				break;
			}
		}
	}
	nameless.erase(object);
	linked.erase((Namespace*)object); // do not check isNamespace(), which is virtual!
	if (found) redefinition(std::set<std::string>({object->nm}));
}

void Namespace::clear()
{
	for (auto &p : named){ p.second->ns = NULL; delete p.second; }
	for (Element *p : nameless){ p->ns = NULL; delete p; }
	
	named.clear();
	nameless.clear();
	// leave linked as is
}

bool Namespace::rename(Element *object, int old_arity, const std::string &new_name)
{
	#ifndef NDEBUG
	size_t n0 = named.size() + nameless.size();
	#endif

	// don't call Element::rename from here!
	if (!object){ assert(false); return false; }
	assert(!object->isNamespace()); // otherwise handling linked stuff is needed
	if (object->ns != this){ assert(false); return false; }

	if (object->nm == new_name && object->arity() == old_arity)
	{
		#ifdef DEBUG
		if (valid_name(new_name))
		{
			auto key = std::make_pair(new_name, old_arity);
			assert(named.find(key) != named.end());
			assert(named.find(key)->second == object);
		}
		#endif
		return true;
	}
	
	auto key0 = std::make_pair(object->nm, old_arity);
	auto key1 = std::make_pair(new_name,   object->arity());
	bool fromDict = named.count(key0);
	assert(!fromDict || named.find(key0) != named.end() && named.find(key0)->second == object);
	std::string old_name = object->nm;

	if (!valid_name(new_name))
	{
		object->nm = new_name;
		if (fromDict)
		{
			named.erase(key0);
			nameless.insert(object);
			assert(n0 == named.size() + nameless.size());
			redefinition(std::set<std::string>({old_name}));
		}
		assert(n0 == named.size() + nameless.size());
		return true;
	}
	else if (named.count(key1))
	{
		assert(named.find(key1)->second != object);
		assert(n0 == named.size() + nameless.size());
		return false;
	}

	std::set<std::string> redefs;
	if (!fromDict) // nameless -> elements
	{
		nameless.erase(object);
	}
	else // renaming an element
	{
		redefs.insert(old_name);
		named.erase(key0);
	}
	named[key1] = object;
	object->nm = new_name;

	assert(n0 == named.size() + nameless.size());
	redefs.insert(new_name);
	redefinition(redefs);
	assert(n0 == named.size() + nameless.size());
	return true;
}

Element *Namespace::find(const std::string &name, int arity, bool recursive) const
{
	auto p = named.find(std::make_pair(name, arity));
	if (p != named.end()) return p->second;
	return (recursive && ns) ? ns->find(name, arity, true) : NULL;
	
}

static inline bool hasPrefix(const std::string &s, size_t pos, const std::string &prefix)
{
	return s.size() >= pos+prefix.length() && std::equal(prefix.begin(), prefix.end(), s.begin()+pos);
}
static inline bool hasSuffix(const std::string &s, size_t pos, size_t n, const std::string &suffix)
{
	return n >= suffix.length() && std::equal(suffix.begin(), suffix.end(), s.begin()+pos+n-suffix.length());
}


void Namespace::candidates(const std::string &s, size_t pos, std::vector<Element*> &ret) const
{
	if (pos >= s.length()) return;

	for (auto &x : named)
	{
		const std::string &xn = x.second->name();
		if (!xn.empty() && hasPrefix(s, pos, xn)) ret.push_back(x.second);
	}
	
	if (container()) container()->candidates(s, pos, ret);
}
void Namespace::rcandidates(const std::string &s, size_t pos, size_t n, std::vector<Element*> &ret) const
{
	if (pos >= s.length() || n == 0) return;
	
	for (auto &x : named)
	{
		const std::string &xn = x.second->name();
		if (!xn.empty() && hasSuffix(s, pos, n, xn)) ret.push_back(x.second);
	}
	
	if (container()) container()->rcandidates(s, pos, n, ret);
}

Function *Namespace::find(FPTR f, int arity) const
{
	if(!f || arity > 4) return NULL;

	if (arity == 1 && root_container())
	{
		BaseFunction *id = root_container()->Identity;
		if(f == (FPTR)id->ufcc || f == (FPTR)id->ufrr) return id;
	}
	
	Function *ret = NULL;
	for (Element *x : *this)
	{
		if (!x->isFunction() && !x->isOperator()) continue;
		Function *xf = (Function*)x;
		if (!xf->base()) continue;
		BaseFunction &c = *(BaseFunction*)xf;
		
		if (arity == 0)
		{
			if (f == (FPTR)c.vfc || f == (FPTR)c.vfr) ret = xf;
		}
		else if (arity == 1)
		{
			if (f == (FPTR)c.ufcc || f == (FPTR)c.ufrr || f == (FPTR)c.ufcr || f == (FPTR)c.ufrc) ret = xf;
		}
		else if (arity == 2)
		{
			if (f == (FPTR)c.bfcc || f == (FPTR)c.bfrr || f == (FPTR)c.bfcr || f == (FPTR)c.bfrc) ret = xf;
		}
		else if (arity == 3)
		{
			if (f == (FPTR)c.tfcc || f == (FPTR)c.tfrr) ret = xf;
		}
		else if (arity == 4)
		{
			if (f == (FPTR)c.qfcc) ret = xf;
		}
		if (ret && !ret->name().empty() && ret->name()[0] != '(') return ret;
		// if the name is not good, try to find a better match
	}
	if (ret) return ret;
	return container() ? container()->find(f, arity) : NULL;
}

const Constant *Namespace::constant(const cnum &z) const
{
	if (!isRoot()) return container() ? container()->constant(z) : NULL;
	for (auto &p : named)
	{
		if (!p.second->isConstant()) continue;
		const Constant *c = (const Constant *)p.second;
		if (eq(z, c->value())) return c;
	}
	return NULL;
}

std::set<Parameter*> Namespace::all_parameters(bool rec) const
{
	std::set<Parameter*> ret;
	for (const Namespace *ns = this; ns; ns = (rec ? ns->container() : NULL))
	{
		for (auto &p : ns->named)
		{
			if (!p.second->isParameter()) continue;
			ret.insert((Parameter*)p.second);
		}
	}
	return ret;
}
std::set<UserFunction*> Namespace::all_functions(bool rec) const
{
	std::set<UserFunction*> ret;
	for (const Namespace *ns = this; ns; ns = (rec ? ns->container() : NULL))
	{
		for (Element *e : *ns)
		{
			if (!e->isFunction()) continue;
			Function *f = (Function*)e;
			if (f->base()) continue;
			ret.insert((UserFunction*)f);
		}
	}
	return ret;
}
