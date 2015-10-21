#include "WorkingTree.h"
#include "ParsingTree.h"
#include "../Namespace/BaseFunction.h"
#include "../Namespace/UserFunction.h"
#include "../Namespace/Operator.h"
#include "../Namespace/Function.h"
#include "../Namespace/Constant.h"
#include "../Namespace/Variable.h"
#include "../Namespace/Parameter.h"
#include "../Namespace/Expression.h"
#include "../Namespace/RootNamespace.h"
#include "../Namespace/AliasVariable.h"
#include <ostream>
#include <iostream>
#include <cassert>

//----------------------------------------------------------------------------------------------------------------------
// c'tor, d'tor, comparison
//----------------------------------------------------------------------------------------------------------------------

WorkingTree::WorkingTree(const ParsingTree &pt, const RootNamespace &ns)
{
	Token *head = pt.head;
	if (!head)
	{
		type = TT_Root;
		rns  = &ns;
	}
	else switch(head->type)
	{
		case Token::TT_Variable:  type = TT_Variable;  variable  = head->data.variable;  break;
		case Token::TT_Parameter: type = TT_Parameter; parameter = head->data.parameter; break;
		case Token::TT_Constant:  type = TT_Constant;  constant  = head->data.constant;  break;
		case Token::TT_Number:    type = TT_Number;    num       = new CNP({&ns, head->num}); break;
		case Token::TT_Function:  type = TT_Function;  function  = head->data.function;  break;
		case Token::TT_Operator:  type = TT_Operator;  operator_ = head->data.operator_; break;
		case Token::TT_Alias:
		{
			const WorkingTree &t = head->data.alias->replacement();
			type = t.type;
			element = t.element;
			if (type == TT_Number) num = new CNP(*num);
			children.assign(t.children.begin(), t.children.end());
			break;
		}
		default:
			assert(false); // it went through ParsingTree::verify
			throw std::logic_error("Unexpected Token type");
	}
	
	for (auto *c : pt) add_child(WorkingTree(*c, ns));
}

bool WorkingTree::verify() const
{
#ifdef DEBUG
	// std::vector can copy stuff after moving
	if ((int)type < 0){ assert(children.empty()); return true; }
	assert(!is_operator(ns().UMinus) || !child(0).is_operator(ns().UMinus));
#endif
	
	int nc = num_children();
	for (int i = 0; i < nc; ++i)
	{
		if (!child(i).verify()){ assert(false); return false; }
	}
	switch (type)
	{
		case TT_Root:
		case TT_Sum:
		case TT_Product:   return rns;
			
		case TT_Number:    return num && num->rns && nc == 0;
		case TT_Constant:  return constant  && constant->isConstant()   && nc == 0;
		case TT_Variable:  return variable  && variable->isVariable()   && nc == 0;
		case TT_Parameter: return parameter && parameter->isParameter() && nc == 0;
		case TT_Function:  return function  && function->isFunction()   && nc == function->arity();
		case TT_Operator:  return operator_ && operator_->isOperator()  && nc == operator_->arity();
	}
	assert(false); return false;
}

bool WorkingTree::operator== (const WorkingTree &t) const
{
	// TODO: this should be much more clever
	if (!type != !t.type || children.size() != t.children.size()) return false;
	if (type == TT_Number) return eq(num->z, t.num->z);
	if (element != t.element) return false;
	for (size_t i = 0, n = num_children(); i < n; ++i) if (children[i] != t.children[i]) return false;
	return true;
}

//----------------------------------------------------------------------------------------------------------------------
// find root namespace
//----------------------------------------------------------------------------------------------------------------------

const RootNamespace &WorkingTree::ns() const
{
	switch (type)
	{
		case TT_Root:
		case TT_Sum:
		case TT_Product: return *rns;
			
		case TT_Number:  return *num->rns;
			
		case TT_Variable:
		case TT_Parameter:
		case TT_Constant:
		case TT_Function:
		case TT_Operator:  return *element->root_container();
	}
	assert(false); throw std::logic_error("can't happen");
}

//----------------------------------------------------------------------------------------------------------------------
// is_constant: true if this can already be computed, independent of variable and parameter values
//----------------------------------------------------------------------------------------------------------------------

bool WorkingTree::is_constant(bool strong) const
{
	switch (type)
	{
		case TT_Variable:  return false;
		case TT_Parameter: return !strong;
		case TT_Number:
		case TT_Constant: return true;
			
		case TT_Function:
		case TT_Operator:
			if (strong && !is_deterministic()) return false;
			// fallthrough
		case TT_Root:
		case TT_Sum:
		case TT_Product:
			for (auto &c : children) if (!c.is_constant(strong)) return false;
			return true;
	}
	assert(false); throw std::logic_error("can't happen");
}

//----------------------------------------------------------------------------------------------------------------------
// uglyness: to find out if replacing this tree with some number would be an improvement in readability
//----------------------------------------------------------------------------------------------------------------------

int WorkingTree::uglyness() const
{
	switch (type)
	{
		case TT_Variable:
		case TT_Parameter:
		case TT_Constant:  return 0;
			
		case TT_Number: return ::uglyness(num->z);
			
		case TT_Root:
		case TT_Function:
		case TT_Operator:
		case TT_Sum:
		case TT_Product:
		{
			int u = 0;
			for (auto &c : children) u += c.uglyness();
			return u;
		}
	}
	assert(false); throw std::logic_error("can't happen");
}

//----------------------------------------------------------------------------------------------------------------------
// is_deterministic: true if multiple evaluations with equal variable and parameter values yield equal results
//----------------------------------------------------------------------------------------------------------------------

bool WorkingTree::is_deterministic() const
{
	for (auto &c : children) if (!c.is_deterministic()) return false;
	if (type == TT_Function || type == TT_Operator)
	{
		return function->deterministic();
	}
	return true;
}

//----------------------------------------------------------------------------------------------------------------------
// collect_parameters: find all Parameters that this depends upon
//----------------------------------------------------------------------------------------------------------------------

void WorkingTree::collect_parameters(std::set<Parameter*> &dst) const
{
	if (type == TT_Parameter)
	{
		dst.insert(const_cast<Parameter*>(parameter));
	}
	else if (type == TT_Function && !function->base())
	{
		const UserFunction *uf = (const UserFunction *)function;
		const Expression *fx = uf->expression();
		if (fx && fx->valid() && fx->wt())
		{
			fx->wt()->collect_parameters(dst);
		}
	}
	for(auto &c : children) c.collect_parameters(dst);
}

//----------------------------------------------------------------------------------------------------------------------
// evaluate: numeric variant
// These are the slow evaluations (but with less overhead than setting up an Evaluator object)
//----------------------------------------------------------------------------------------------------------------------

cnum WorkingTree::evaluate(const std::map<const Variable*, cnum> &values) const
{
	switch (type)
	{
		case TT_Root:      assert(false); return UNDEFINED;
		case TT_Constant:  return constant->value();
		case TT_Parameter: return parameter->value();
		case TT_Number:    return num->z;
		case TT_Variable:
		{
			auto it = values.find(variable);
			if (it == values.end()) return UNDEFINED;
			return it->second;
		}
		case TT_Function:
		case TT_Operator:
		{
			int nc = num_children();
			if (nc != function->arity()) return UNDEFINED;
			if (function->base())
			{
				BaseFunction &f = *(BaseFunction*)function;
				switch (nc)
				{
					case 0: return f();
					case 1: return f(children[0].evaluate(values));
					case 2: return f(children[0].evaluate(values), children[1].evaluate(values));
					case 3: return f(children[0].evaluate(values), children[1].evaluate(values),
									 children[2].evaluate(values));
					case 4: return f(children[0].evaluate(values), children[1].evaluate(values),
									 children[2].evaluate(values), children[3].evaluate(values));
					default: assert(false); return UNDEFINED;
				}
			}
			else
			{
				UserFunction &f = *(UserFunction*)function;
				std::map<const Variable*, cnum> args;
				if (nc != f.arity())
				{
					assert(false);
					return UNDEFINED;
				}
				Expression *fx = f.expression();
				if (!fx){ assert(false); return UNDEFINED; }
				for (int i = 0; i < nc; ++i)
				{
					Variable *xi = f.arguments()[i];
					if (!xi || !xi->isVariable()){ assert(false); return UNDEFINED; }
					args[xi] = children[i].evaluate(values);
				}
				return fx->evaluate(args);
			}
		}
			
		case TT_Sum:
		{
			cnum ret = 0.0;
			for (auto &c : children) ret += c.evaluate(values);
			return ret;
		}
		case TT_Product:
		{
			cnum ret = 1.0;
			for (auto &c : children) ret *= c.evaluate(values);
			return ret;
		}
	}
	assert(false); throw std::logic_error("can't happen");
}

//----------------------------------------------------------------------------------------------------------------------
// evaluate: symbolic variant
//----------------------------------------------------------------------------------------------------------------------

WorkingTree WorkingTree::evaluate(const std::map<const Variable*, const WorkingTree *> &values) const
{
	switch (type)
	{
		case TT_Constant:
		case TT_Parameter:
		case TT_Number:    return *this;
			
		case TT_Variable:
		{
			auto it = values.find(variable);
			return *(it == values.end() ? this : it->second);
		}
			
		case TT_Root:
		case TT_Function:
		case TT_Operator:
		case TT_Sum:
		case TT_Product:
		{
			WorkingTree ret(element, type);
			for (auto &c : children) ret.add_child(std::move(c.evaluate(values)));
			return ret;
		}
	}
	assert(false); throw std::logic_error("can't happen");
}

//----------------------------------------------------------------------------------------------------------------------
// range
//----------------------------------------------------------------------------------------------------------------------

bool WorkingTree::is_real() const
{
	switch (type)
	{
		case TT_Root: assert(false); return false;
		case TT_Constant:  return constant->real();
		case TT_Parameter: return parameter->is_real();
		case TT_Number:    return ::is_real(num->z);
		case TT_Variable:  return variable->real();
			
		case TT_Function:
		case TT_Operator:
			if (function->is_real(false)) return true;  // always real
			if (!function->is_real(true)) return false; // always complex
			// fallthrough
			
		case TT_Sum:
		case TT_Product: return has_real_children();
	}
	assert(false); throw std::logic_error("can't happen");
}

bool WorkingTree::is_real(const std::set<Variable*> &real_vars) const
{
	switch (type)
	{
		case TT_Root:      return false;
		case TT_Constant:  return constant->real();
		case TT_Parameter: return parameter->is_real();
		case TT_Number:    return ::is_real(num->z);
		case TT_Variable:  return variable->real() || real_vars.count((Variable*)variable);
			
		case TT_Function:
		case TT_Operator:
			if (function->is_real(false)) return true;  // always real
			if (!function->is_real(true)) return false; // always complex
			// fallthrough
			
		case TT_Sum:
		case TT_Product:
			for (auto &c : children) if (!c.is_real(real_vars)) return false;
			return true;
	}
	assert(false); throw std::logic_error("can't happen");
}

Range WorkingTree::range() const
{
	switch (type)
	{
		case TT_Root: assert(false); return false;
		case TT_Constant:  return constant->range();
		case TT_Parameter: return parameter->range();
		case TT_Number:    return ::range(num->z);
		case TT_Variable:  return variable->range();
			
		case TT_Function:
		case TT_Operator:
		{
			if (children.empty()) return function->range(R_Complex);
			Range r = child(0).range();
			for (int i = 1, n = num_children(); i < n; ++i) r &= child(i).range();
			return function->range(r);
		}
			
		case TT_Sum:
		case TT_Product:
		{
			int n = num_children(), nr = 0, ni = 0;
			if (n == 0) return type == TT_Sum ? R_Zero : R_One;
			Range r = child(0).range(); if (n == 1) return r;
			if ((r & R_Real)) ++nr; else if ((r & R_Imag)) ++ni;
			for (int i = 1; i < n; ++i)
			{
				Range rc = child(i).range();
				r &= rc;
			}
			if (type == TT_Product && nr+ni == n)
			{
				r &= ~(R_Imag | R_Real);
				r |= (ni&1) ? R_Imag : R_Real;
			}
			else
			{
				r &= ~R_Unit;
			}
			return r;
		}
	}
	assert(false); throw std::logic_error("can't happen");
}
Range WorkingTree::range(const std::map<Variable*, Range> &input) const
{
	switch (type)
	{
		case TT_Root: assert(false); return false;
		case TT_Constant:  return constant->range();
		case TT_Parameter: return parameter->range();
		case TT_Number:    return ::range(num->z);
		case TT_Variable:
		{
			auto i = input.find((Variable*)variable);
			return i == input.end() ? variable->range() : i->second;
		}
		
		case TT_Function:
		case TT_Operator:
		{
			if (children.empty()) return function->range(R_Complex);
			Range r = child(0).range();
			for (int i = 1, n = num_children(); i < n; ++i) r &= child(i).range();
			return function->range(r);
		}
			
		case TT_Sum:
		case TT_Product:
		{
			int n = num_children(), nr = 0, ni = 0;
			if (n == 0) return type == TT_Sum ? R_Zero : R_One;
			Range r = child(0).range(); if (n == 1) return r;
			if ((r & R_Real)) ++nr; else if ((r & R_Imag)) ++ni;
			for (int i = 1; i < n; ++i)
			{
				Range rc = child(i).range();
				r &= rc;
			}
			if (type == TT_Product && nr+ni == n)
			{
				r &= ~(R_Imag | R_Real);
				r |= (ni&1) ? R_Imag : R_Real;
			}
			else
			{
				r &= ~R_Unit;
			}
			return r;
		}
	}
	assert(false); throw std::logic_error("can't happen");
}

