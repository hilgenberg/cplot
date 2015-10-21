#include "Element.h"
#include "Namespace.h"
#include "RootNamespace.h"
#include <cassert>

//----------------------------------------------------------------------------------------------------------------------
// Element
//----------------------------------------------------------------------------------------------------------------------

Element::~Element()
{
	if (ns) ns->remove(this);
}

void Element::save(Serializer &s) const
{
	s._string(nm);
}
void Element::load(Deserializer &s)
{
	s._string(nm);
}

std::string Element::displayName(PrintingStyle ds) const
{
	auto i = dn.find(ds); if (i != dn.end()) return i->second;
	switch (ds)
	{
		case PS_Debug:
		case PS_Function:
		case PS_Console:  return name();
			
		case PS_Input:
		case PS_Head:
		case PS_HTML:
		case PS_LaTeX:
		case PS_Mathematica: ds = PS_Console; break; // try again
	}
	i = dn.find(ds); return i != dn.end() ? i->second : name();
}

RootNamespace *Element::root_container() const
{
	for (Namespace *ns = container(); ns; ns = ns->container())
	{
		if (ns->isRoot()) return (RootNamespace*)ns;
	}
	assert(isNamespace() && ((Namespace*)this)->isRoot());
	return NULL;
}

bool Element::rename(const std::string &new_name)
{
	if (nm == new_name) return true;
	
	if (!ns)
	{
		nm = new_name;
		return true;
	}
	else
	{
		return ns->rename(this, new_name);
	}
}

Element *Element::copy(Namespace &other) const
{
	if (&other == container()) return NULL;
	
	Element *e = copy(); if (!e) return NULL;
	
	assert(e->name() == name());
	
	try
	{
		other.add(e);
	}
	catch(...)
	{
		delete e;
		throw;
	}
	return e;
}
