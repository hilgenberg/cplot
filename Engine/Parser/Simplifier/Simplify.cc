#include "../WorkingTree.h"
#include "Pattern.h"
#include <cassert>
#include <iostream>

//----------------------------------------------------------------------------------------------------------------------
// WorkingTree::simplify
//----------------------------------------------------------------------------------------------------------------------

void WorkingTree::simplify(bool full)
{
	#ifdef DEBUG
	std::ostringstream os;
	os << *this;
	std::string last = os.str();
	std::cerr << "Simplify in  [" << size() << "/" << depth() << "]: " << last << std::endl;
	#define UPD(step) do{\
		assert(verify());\
		os.str(""); os << *this;\
		if (os.str() != last){\
			last = os.str();\
			std::cerr << step << " --> " << last << std::endl;\
		}\
	}while(0)
	#else
	#define UPD(step)
	#endif
	
	bool change = true;
	assert(verify());
	normalize();
	flatten(change);
	assert(verify());

	for (int i = 0; change && i < (full ? 100 : 10); ++i)
	{
		change = false;
		simplify_functions(change); UPD("functions");
		flatten(change);            UPD("  flatten");
		simplify_powers(change);    UPD("   powers");
		simplify_products(change);  UPD(" products");
		simplify_sums(change);      UPD("     sums");
		simplify_unary(change);     UPD("    unary");
		apply_rules(change);        UPD("    rules");
	}
	
	denormalize();
	
	#ifdef DEBUG
	std::cerr << "Simplify out [" << size() << "/" << depth() << "]: " << *this << std::endl;
	#endif
	
	assert(verify());
	

}

//----------------------------------------------------------------------------------------------------------------------
// WorkingTree::apply_rules
//----------------------------------------------------------------------------------------------------------------------

const Function *WorkingTree::head() const
{
	switch (type)
	{
		case TT_Function:
		case TT_Operator: return function;
			
		case TT_Sum:     return rns->Plus;
		case TT_Product: return rns->Mul;

		case TT_Root:
		case TT_Constant:
		case TT_Number:
		case TT_Variable:
		case TT_Parameter: return NULL;
	}
	assert(false); throw std::logic_error("Can't happen");
}

bool WorkingTree::can_match(std::set<Element*> &es) const
{
	for (auto &c : children) c.can_match(es);
	
	switch (type)
	{
		case TT_Function:
		case TT_Operator: es.erase((Element*)function);  break;
		case TT_Sum:      es.erase(rns->Plus); break;
		case TT_Product:  es.erase(rns->Mul);  break;
		case TT_Constant: es.erase((Element*)constant);  break;
			
		case TT_Root:
		case TT_Number:
		case TT_Variable:
		case TT_Parameter: break;
	}
	return es.empty();
}

void WorkingTree::apply_rules(bool &change)
{
	for (auto &c : children) c.apply_rules(change);
	
	const std::vector<Rule> &R = ns().rules(head());
	if (R.empty()) return;
	
	for (const Rule &r : R)
	{
		auto es(r.elements());
		if (can_match(es) && r.transform(*this))
		{
			change = true;
			break;
		}
	}
}

