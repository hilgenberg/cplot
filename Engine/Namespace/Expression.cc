#include "Expression.h"
#include "../Parser/PreToken.h"
#include "../Parser/Token.h"
#include "../Parser/ParsingResult.h"
#include "../Parser/ParsingTree.h"
#include "../Parser/WorkingTree.h"
#include "../Parser/OptimizingTree.h"
#include "../Parser/Evaluator.h"
#include "Namespace.h"
#include "../Parser/utf8/utf8.h"

#include <iostream>
#include <stack>
#include <memory>

void Expression::save(Serializer &ser) const
{
	assert(false); // should probably never be saved
	Element::save(ser);
	ser._uint64(m_strings.size());
	for (const std::string &s : m_strings)
	{
		ser._string(s);
	}
}
void Expression::load(Deserializer &ser)
{
	assert(false);
	Element::load(ser);
	uint64_t n;
	ser._uint64(n);
	for (size_t i = 0; i < n; ++i)
	{
		std::string s;
		ser._string(s);
		m_strings.push_back(s);
	}
}

cnum Expression::parse(const std::string &s, const Namespace *ns_, ParsingResult &result)
{
	Expression x;
	
	Namespace *ns = const_cast<Namespace*>(ns_);
	assert(ns && ns->root_container());

	if (ns) ns->add(&x);
	auto deleter = [ns](Expression *x){ if (ns) ns->remove(x); };
	std::unique_ptr<Expression, decltype(deleter)> cleanup(&x, deleter);
	
	try
	{
		x.strings(s);
		result = x.result();
	}
	catch(...)
	{
		result.error("Unexpected exception", 0, s.length());
		return UNDEFINED;
	}
	
	if (!x.result().ok)
	{
		return UNDEFINED;
	}
	if (!x.constant(false))
	{
		result.error("Expression is not constant", 0, s.length());
		return UNDEFINED;
	}
	std::map<const Variable*, cnum> dummy;
	cnum res = x.evaluate(dummy);
	if (!defined(res))
	{
		result.error("Expression is undefined", 0, s.length());
	}
	return res;
}

Expression::~Expression()
{
	delete m_ev;
	delete m_ot;
	delete m_wt;
}

void Expression::redefinition(const std::set<std::string> &affected_names)
{
	if (dirty) return;
	
	bool match = false;
	for (auto &s : affected_names)
	{
		for (auto &f : m_strings)
		{
			if (f.find(s) != std::string::npos){ match = true; break; }
		}
	}
	if (match)
	{
		dirty = true;
	}
}

WorkingTree *Expression::wt0() const
{
	WorkingTree *t = wt();
	assert(t);
	assert(t->type == WorkingTree::TT_Root && t->num_children() == 1);
	return (t && t->num_children() > 0) ? &t->child(0) : NULL;
}

void Expression::strings(const std::vector<std::string> &strs)
{
	m_strings = strs;
	dirty = true;
}
void Expression::parse() const
{
	if (!dirty) return;
	dirty = false;

	// reset state
	delete m_ev; m_ev = NULL;
	delete m_ot; m_ot = NULL;
	delete m_wt; m_wt = NULL;
	m_result.reset(true);

	if (!container()){ m_result.error("No namespace", 0, 0); return; }
	RootNamespace *rns = root_container();
	if (!rns){ m_result.error("No root namespace", 0, 0); return; }
	if (container()->empty()){ m_result.error("Empty namespace", 0, 0); return; }
	if (m_strings.size() == 0){ m_result.error("Empty list of expressions", 0, 0); return; }
	
	m_wt = new WorkingTree(*rns);
	
	for (auto &s : m_strings)
	{
		ParsingTree *c = ParsingTree::parse(s, *container(), m_result);
		if (!c)
		{
			assert(!m_result.ok);
			delete m_wt; m_wt = NULL;
			return;
		}

		m_wt->add_subtree(WorkingTree(*c, *rns));
		delete c;
		++m_result.index;
		
		#ifdef PARSER_DEBUG
		std::cerr << "After wtree conversion: " << m_wt->child(m_wt->num_children()-1) << std::endl << std::endl;
		#endif
	}

	if (!m_result.ok)
	{
		assert(false); // should already have returned
		delete m_wt;
		m_wt = NULL;
	}
}

bool Expression::constant(bool strong) const
{
	if (!valid()) return true;
	if (m_wt->num_children() != 1) return false;
	return m_wt->child(0).is_constant(strong);
}
bool Expression::deterministic() const
{
	if (!valid()) return true;
	return m_wt->is_deterministic();
}

cnum Expression::evaluate(const std::map<const Variable*, cnum> &values) const
{
	if (!valid()) return UNDEFINED;
	if (m_wt->num_children() != 1) return UNDEFINED;
	return m_wt->child(0).evaluate(values);
}

Evaluator *Expression::evaluator(const std::vector<const Variable *> &var_order)
{
	if (!valid()) return NULL;

	if (m_ev) return m_ev;
	assert(!m_ot); delete m_ot; m_ot = NULL;

	// (1) WorkingTree --> OptimizingTree
	try
	{
		m_ot = new OptimizingTree(m_wt, m_result);
	}
	catch(ParsingResult &)
	{
		return NULL;
	}
	
	#ifdef PARSER_DEBUG
	std::cerr << "After otree building: " << *m_ot << endl;
	#endif
	
	// (2) optimize
	try
	{
		m_ot->optimize();
	}
	catch(...)
	{
		delete m_ot; m_ot = NULL;
		return NULL;
	}
	
	#ifdef PARSER_DEBUG
	std::cerr << "After optimizing: " << *m_ot << endl;
	#endif

	// (3) create the Evaluator
	try
	{
		RootNamespace *rns = root_container();
		assert(rns); // otherwise !valid()
		
		m_ev = new Evaluator(m_ot, var_order, *rns);
		
		#ifdef PARSER_DEBUG
		std::cerr << "After flattening: " << endl;
		m_ev->print(std::cerr, container());
		std::cerr << endl;
		#endif
	}
	catch(...)
	{
		m_ev = NULL;
		return NULL;
	}
	
	return m_ev;
}

std::set<Parameter*> Expression::usedParameters() const
{
	std::set<Parameter*> ps;
	if (valid()) m_wt->collect_parameters(ps);
	return ps;
}

std::set<Variable*> Expression::usedVariables() const
{
	std::set<Variable*> vs;
	if (valid()) m_wt->collect_variables(vs);
	return vs;
}

bool Expression::uses_object(const std::string &name) const
{
	for (const std::string &s : m_strings) if (s.find(name) != std::string::npos) return true;
	return false;
}

WorkingTree *Expression::derivative(const Variable &x, std::string &error) const
{
	if (dirty) parse();
	if (!m_wt){ error = m_result.info; return NULL; }
	return m_wt->derivative(x, error);
}

