#pragma once
#include "../../Persistence/Serializer.h"
#include "ObjectDB.h"
#include <string>
#include <set>
class Namespace;
class RootNamespace;

/**
 * Elements live in a namespace and can be functions, parameters, other namespaces, ...
 * There are two types of elements:
 * -# those with names, which model defined mathematical symbols
 * -# namespaces, which model mathematical scopes
 */
class Element : public Serializable
{
	friend class Namespace;
	friend class RootNamespace;
	friend class Plot;
	
protected:
	Element(const std::string &name) : ns(NULL), nm(name){ }
	Element(const Element &) = delete;
	Element & operator= (const Element &) = delete;
	
	virtual ~Element(); ///< Also removes it from its namespace
	
	Element *copy(Namespace &other) const; ///< Copies the element into another Namespace.
	virtual Element *copy() const = 0; // builtins must return NULL
	
	virtual void save(Serializer   &s) const;
	virtual void load(Deserializer &s);
	
public:
	Namespace     *container() const{ return ns; }
	virtual RootNamespace *root_container() const; ///< Walks up the container chain until it finds the root namespace.
	
	const std::string &name() const{ return nm; }
	bool rename(const std::string &new_name); ///< @return false if new_name already exists in the namespace
	inline size_t namelen() const
	{
		size_t l = nm.length();
		return (l > 2 && nm[0] == '(') ? 1 : l;
	}
	
	virtual bool builtin    () const = 0; ///< Builtins are never saved
	virtual bool isFunction () const{ return false; }
	virtual bool isVariable () const{ return false; }
	virtual bool isAlias    () const{ return false; }
	virtual bool isConstant () const{ return false; }
	virtual bool isOperator () const{ return false; }
	virtual bool isParameter() const{ return false; }
	virtual bool isNamespace() const{ return false; }
	
	virtual int arity() const = 0; // -1 for parameters and such
	
	void        displayName(PrintingStyle ds, const std::string &n){ assert(builtin()); dn[ds] = n; }
	std::string displayName(PrintingStyle ds) const;
	
private:
	std::string nm; ///< Only ns should modify this because its consistency depends on it.
	Namespace  *ns; ///< Only ns should modify this because its consistency depends on it.
	std::map<PrintingStyle, std::string> dn; ///< Display name
	
	/**
	 * Elements get updates from all other elements in the same or any containing namespace, whose definition changes.
	 * Definition means anything that should trigger reparsing of any expressions (things like name, type, function
	 * definitions). This can trigger reparsing of expressions that will not change (which should be ok, parsing is
	 * not that slow), but it *must* trigger reparsing of all expressions that did in fact change. Which means that
	 * subclasses overriding this method must rather err on the side of reparsing too much.
	 *
	 * Infinite recursion is prevented by delaying the reparsing by just setting a dirty bit and ignoring redefinitions
	 * while the dirty bit is set.
	 *
	 * @param affected_names Names of all elements that have changed. Renaming an element puts the old and new names
	 *        in the list.
	 */
	virtual void redefinition(const std::set<std::string> &affected_names)
	{
		(void)affected_names; // not used here
	}
	
protected:
	/// When an element's definition changes, it should call redefine() to notify its namespace
	inline void redefine() const;//{ if (ns) ns->redefine(this); } implementation is below Namespace
	
	virtual void added_to_namespace(){ } // called by namespace after adding an element
};
