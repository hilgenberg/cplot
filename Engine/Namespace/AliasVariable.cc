#include "AliasVariable.h"
#include "Variable.h"
#include "Expression.h"
#include "RootNamespace.h"
#include "../Parser/WorkingTree.h"

AliasVariable::AliasVariable(const std::string &n, Variable *re)
: Element(n), m_replacement(re)
{
	assert(re);
}

AliasVariable::AliasVariable(const std::string &n, Variable *re, Variable *im)
: Element(n), m_replacement(re->root_container()->Combine, WorkingTree(re), WorkingTree(im))
{
	assert(re && im);
}

AliasVariable::AliasVariable(const std::string &n, const Expression &ex)
: Element(n), m_replacement(*ex.wt0())
{
	assert(ex.wt0());
}

AliasVariable::AliasVariable(const std::string &n, AliasVariable *x)
: Element(n), m_replacement(x->m_replacement)
{
}
