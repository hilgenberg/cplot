#pragma once

#include <string>
#include <vector>
#include <map>
#include <set>

#include "../../Persistence/Serializer.h"
#include "ObjectDB.h"
#include "Element.h"
#include "../Parser/FPTR.h"

class Function;
class Parameter;
class Constant;
class UserFunction;
class Namespace;
class RootNamespace;

/**
 * A Namespace contains elements and models a mathemetical scope for definitions. So any element's definition can
 * depend on others in the same namespace and on elements in containing namespaces.
 */

class Namespace : public Element
{
public:
	Namespace() : Element(""), redefining(false){ }
	virtual ~Namespace();
	
	virtual void save(Serializer   &s) const;
	virtual void load(Deserializer &s);
	virtual TypeCode type_code() const{ return TypeCode::NAMESPACE; }
	
	virtual bool isNamespace() const{ return true;  }
	virtual bool      isRoot() const{ return false; }
	virtual bool     builtin() const{ return false; }
	virtual int        arity() const{ return -1;    }

	/// Valid names cannot start with numbers or contain spaces or control characters.
	static bool  valid_name(const std::string &name);
	std::string unique_name(const std::string &basename, int arity) const; ///< Add some suffix if needed.

	/**
	 * Add object to this namespace. The object will be owned and later deleted by this namespace.
	 * Can modify object's name to unique_name(old_name).
	 */
	void add(Element *object);
	
	/**
	 * Make parent's symbols accessible to this namespace.
	 * Unlike parent->add(this), linking does not give owning semantics to parent.
	 * Sets this->ns to parent.
	 */
	void link(Namespace *parent);
	
	/// Remove and/or unlink object, but do not delete it.
	void remove(Element *object);
	
	/**
	 * Rename object if possible.
	 * @param object The element to rename
	 * @param new_name Its new name
	 * @return false if new_name already exists
	 */
	bool rename(Element *object, const std::string &new_name){ return rename(object, object->arity(), new_name); }
	bool rename(Element *object, int old_arity, const std::string &new_name);
	
	/// Removes and deletes everything in the namespace, but does not affect linked namespaces.
	virtual void  clear();
	
	Element  *find(const std::string &name, int arity, bool recursive = true) const; ///< Find by name.
	Function *find(FPTR function,           int arity) const; ///< Find BaseFunction or Operator by function pointer.
	
	/**
	 * Find constant by value.
	 * @return NULL if not found
	 */
	const Constant *constant(const cnum &z) const;

	/**
	 * Find all elements whose name is a prefix of s.substr(pos, end)
	 */
	std::vector<Element*> candidates(const std::string &s, size_t pos) const
	{
		std::vector<Element*> ret;
		candidates(s, pos, ret);
		return ret;
	}
	virtual void candidates(const std::string &s, size_t pos, std::vector<Element*> &ret) const;

	/**
	 * Like candidates but match suffix s[pos+k]...s[pos-2]s[pos+n-2]s[pos+n-1], k >= 0
	 */
	std::vector<Element*> rcandidates(const std::string &s, size_t pos, size_t n) const
	{
		std::vector<Element*> ret;
		rcandidates(s, pos, n, ret);
		return ret;
	}
	virtual void rcandidates(const std::string &s, size_t pos, size_t n, std::vector<Element*> &ret) const;

	bool empty() const{ return named.empty() && nameless.empty() && (!ns || ns->empty()); }
	
	std::set<Parameter*>    all_parameters(bool recursive = false) const; ///< Only named ones, recursive also collects in container(), etc
	std::set<UserFunction*> all_functions (bool recursive = false) const; ///< Unnamed ones too
	
	void redefine(const Element *x)
	{
		if (!x || !named.count(std::make_pair(x->name(), x->arity()))) return;
		
		redefinition(std::set<std::string>({x->name()}));
		assert(named[std::make_pair(x->name(), x->arity())] == x);
	}
	
	typedef std::map<std::pair<std::string, int>, Element*> NamedMap; // (name, arity) -> element

	class const_iterator : public std::iterator<std::forward_iterator_tag, Element*>
	{
	public:
		bool operator!= (const const_iterator& other) const
		{
			return i1 != other.i1 || i2 != other.i2;
		}
		bool operator== (const const_iterator& other) const
		{
			return i1 == other.i1 && i2 == other.i2;
		}
		Element* operator* () const
		{
			return first ? i1->second : *i2;
		}
		const_iterator &operator++ ()
		{
			if (first)
			{
				if (++i1 == i1end) first = false;
			}
			else
			{
				++i2;
			}
			return *this;
		}
		
	private:
		friend class Namespace;
		const_iterator(NamedMap::const_iterator i1,
					   NamedMap::const_iterator i1end,
					   std::set<Element*>::const_iterator i2)
		: i1(i1), i1end(i1end), i2(i2), first(i1 != i1end)
		{ }
		NamedMap::const_iterator i1, i1end;
		std::set<Element*>::const_iterator i2; // elements without a valid, nonempty name
		bool first; // still iterating the first iterator?
	};
	
	const_iterator begin() const{ return const_iterator(named.begin(), named.end(), nameless.begin()); }
	const_iterator   end() const{ return const_iterator(named.end(),   named.end(), nameless.end());   }
	
#ifdef DEBUG
	virtual void dump(std::ostream &o) const{ o << "Namespace " << named.size() << " named, " << nameless.size() << " nameless"; }
#endif

protected:
	virtual Element *copy() const;
	void add_builtin(Element *object); // does not mangle the name

	// a Namespace owns its elements in named and nameless
	NamedMap             named;    ///< elements with a valid name
	std::set<Element*>   nameless; ///< elements without a valid, nonempty name
	std::set<Namespace*> linked;   ///< not iterated, owned or saved, but notified of redefinitions
	
	std::set<std::string> redefinition_queue; // recursion breaker during redefinition runs
	bool                  redefining;
	
	virtual void redefinition(const std::set<std::string> &affected_names)
	{
		if (affected_names.empty()) return;

		if (redefining)
		{
			redefinition_queue.insert(affected_names.begin(), affected_names.end());
			return;
		}

		while (true)
		{
			try
			{
				std::set<std::string> q;
				if (!redefining){ q = affected_names; redefining = true; }else{ std::swap(q, redefinition_queue); }
				
				std::set<Element*> es(begin(), end());
				for (Element *x : es) x->redefinition(q);
				for (Element *x : linked) x->redefinition(q);
				
				if (redefinition_queue.empty()) break;
			}
			catch(...){ }
		}
		redefining = false;
	}
};

void Element::redefine() const{ if (ns) ns->redefine(this); }

