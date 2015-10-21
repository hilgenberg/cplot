#include "OptimizingTree.h"
#include "WorkingTree.h"
#include "Token.h"
#include "../Namespace/RootNamespace.h"
#include "../Namespace/Constant.h"
#include "../Namespace/Variable.h"
#include "../Namespace/Function.h"
#include "../Namespace/UserFunction.h"
#include "../Namespace/Parameter.h"
#include <assert.h>
#include <string>
#include <ostream>
#include <iostream>
#include <list>

//----------------------------------------------------------------------------------------------------------------------
//  Printing
//----------------------------------------------------------------------------------------------------------------------

static void print(std::ostream &out, const OptimizingTree &tree)
{
	int nc = tree.num_children();
	assert(!tree.root());
	switch(tree.type)
	{
		case OptimizingTree::OT_Constant: out << to_string(*tree.value); break;
		case OptimizingTree::OT_Variable: out << tree.variable->name(); break;
		case OptimizingTree::OT_Function: out << tree.function->name(); break;
		default: out << "???";
	}
	if (nc) out << "( ";
	for (int i = 0; i < nc; ++i)
	{
		if(i>0) out << ", ";
		out << *tree.child(i);
	}
	if(nc) out << " )";
}
static void getUsage(const OptimizingTree *tree, int &total, int &unique, std::set<const OptimizingTree*> &visited)
{
	++total;
	if(visited.find(tree) == visited.end())
	{
		visited.insert(tree);
		++unique;
	}
	for (auto *c : *tree) getUsage(c, total, unique, visited);
}
std::ostream &operator<<(std::ostream &out, const OptimizingTree &tree)
{
	if(!tree.root())
	{
		print(out, tree);
		return out;
	}
	
	int nc = tree.num_children(), total = 0, unique = 0;
	std::set<const OptimizingTree*> visited;
	for (int i = 0; i < nc; ++i)
	{
		getUsage(tree.child(i), total, unique, visited);
	}
	out << std::endl << "OTree[" << unique << "/" << total << "]: ";
	for (int i = 0; i < nc; ++i)
	{
		if(nc > 1) out << std::endl << "    ";
		print(out, *tree.child(i));
	}
	if(nc==0) out << "(empty)";
	return out << std::endl;
}

//----------------------------------------------------------------------------------------------------------------------
//  Constructor, Destructor
//----------------------------------------------------------------------------------------------------------------------

OptimizingTree::OptimizingTree(const WorkingTree *wt, ParsingResult &result)
{
	assert(wt);
	std::vector<WorkingTree *> tmpTrees; // for applied UserFunctions
Restart:
	
	switch(wt->type)
	{
		case WorkingTree::TT_Root:
			type = OT_Function;
			function = NULL;
			for (auto &c : *wt) add_child(new OptimizingTree(&c, result));
			return;
			
		case WorkingTree::TT_Variable:
			type = OT_Variable;
			variable = wt->variable;
			break;
			
		case WorkingTree::TT_Parameter:
			type = OT_Variable;
			variable = wt->parameter;
			break;
			
		case WorkingTree::TT_Constant:
		{
			type = OT_Constant;
			value = new cnum(wt->constant->value());
			break;
		}
			
		case WorkingTree::TT_Number:
			type = OT_Constant;
			value = new cnum((cnum)*wt);
			break;
			
		case WorkingTree::TT_Function:
		case WorkingTree::TT_Operator:
		{
			type = OT_Function;
			const Function *f = wt->function;
			if (f->base())
			{
				function = (BaseFunction*)f;
				for(auto &c : *wt) add_child(new OptimizingTree(&c, result));
				assert(function);
			}
			else
			{
				const UserFunction *uf = (const UserFunction*)f;
				WorkingTree *r = uf->apply(wt->children);
				if (!r)
				{
					for (auto *x : tmpTrees) delete x;
					result.error("Invalid subexpression", 0, 0);
					throw result;
				}
				tmpTrees.push_back(r);
				wt = r;
				if (wt->type == WorkingTree::TT_Root)
				{
					assert(wt->num_children() == 1);
					wt = &r->children[0];
				}
				goto Restart;
			}
			break;
		}
			
		case WorkingTree::TT_Sum:
		case WorkingTree::TT_Product:
		{
			if (wt->num_children() == 1)
			{
				assert(wt->num_children() == 1);
				wt = &wt->children[0];
				goto Restart;
			}
			
			bool sum = (wt->type == WorkingTree::TT_Sum);
			
			if (wt->num_children() == 0)
			{
				type = OT_Constant;
				value = new cnum(sum ? 0.0 : 1.0);
				break;
			}
			
			type = OT_Function;
			const BinaryOperator *add = (sum ? wt->ns().Plus   : wt->ns().Mul);
			const BinaryOperator *sub = (sum ? wt->ns().Minus  : wt->ns().Div);
			const UnaryOperator  *neg = (sum ? wt->ns().UMinus : wt->ns().Invert);
			
			add_child(new OptimizingTree(&wt->child(0), result));
			OptimizingTree *dst = this;
			for (int i = wt->num_children()-1; i >= 1; --i)
			{
				OptimizingTree *c1;
				if (i == 1)
				{
					c1 = new OptimizingTree(&wt->child(0), result);
				}
				else
				{
					c1 = new OptimizingTree;
					assert(c1->type == OT_Function);
				}
				dst->add_child(c1);
				
				auto &c = wt->child(i);
				if (c.is_operator(neg))
				{
					dst->function = sub;
					dst->add_child(new OptimizingTree(&c.child(0), result));
				}
				else
				{
					dst->function = add;
					dst->add_child(new OptimizingTree(&c, result));
				}
				
				dst = c1;
			}
			break;
		}
	}
	
	for (auto *x : tmpTrees) delete x;
}

// Constructor for root node
OptimizingTree::OptimizingTree() : type(OT_Function)
{
	function = NULL;
}

OptimizingTree::~OptimizingTree()
{
	if(type == OT_Constant) delete value;
}

//----------------------------------------------------------------------------------------------------------------------
//  Helper methods
//----------------------------------------------------------------------------------------------------------------------

OptimizingTree *OptimizingTree::copier(OptimizingTree *tree_to_copy, const RootNamespace &rns)
{
	OptimizingTree *ret = new OptimizingTree();
	ret->type = OT_Function;
	ret->function = rns.Identity;
	ret->add_child(tree_to_copy);
	ret->deterministic = tree_to_copy->deterministic;
	ret->real          = tree_to_copy->real;
	return ret;
}

//----------------------------------------------------------------------------------------------------------------------
//  Updating methods for deterministic and real flags
//----------------------------------------------------------------------------------------------------------------------

void OptimizingTree::update_deterministic()
{
	if (type != OT_Function)
	{
		deterministic = true;
		return;
	}
	for (auto x : children)
	{
		x->update_deterministic();
	}
	for (auto x : children)
	{
		if(!x->deterministic){ deterministic = false; return; }
	}
	deterministic = (!function || function->deterministic());
}

void OptimizingTree::update_real()
{
	for (auto c : children) c->update_real();
	switch(type)
	{
		case OT_Variable:
			if (variable->isVariable())
			{
				real =  ((Variable*)variable)->real();
			}
			else
			{
				assert(variable->isParameter());
				real = ((const Parameter *)variable)->is_real();
			}
			break;
		case OT_Constant:
			real = is_real(*value);
			break;
		case OT_Function:
		{
			bool realParams = true;
			for(auto c : children) if(!c->real){ realParams = false; break; }
			real = function ? function->is_real(realParams) : realParams;
		}
	}
}

//----------------------------------------------------------------------------------------------------------------------
//  Calculate as much as possible
//----------------------------------------------------------------------------------------------------------------------

void OptimizingTree::calculate()
{
	if(type != OT_Function) return;
	size_t nc = children.size();
	for(auto x : children) x->calculate();
	
	if(root() || !deterministic) return;
	
	for(auto x : children) if(x->type != OT_Constant) return;
	cnum z;
	switch(nc)
	{
		case 1: z = (*function)(*children[0]->value); break;
		case 2: z = (*function)(*children[0]->value, *children[1]->value); break;
		case 3: z = (*function)(*children[0]->value, *children[1]->value, *children[2]->value); break;
		case 4: z = (*function)(*children[0]->value, *children[1]->value,
								*children[2]->value, *children[3]->value); break;
	}
	type = OT_Constant;
	value = new cnum(z);
	Super::reset();
}

//----------------------------------------------------------------------------------------------------------------------
//  Merge multiple occurances of equal subexpressions (as in sinx * exp sinx)
//----------------------------------------------------------------------------------------------------------------------

void OptimizingTree::merge()
{
	assert(root());
	// first we merge all constants and vars - afterwards we can compare them by pointer
	std::unordered_map<cnum,OptimizingTree*,cnum_hash> nums;
	std::map<const Element *, OptimizingTree *> vars;
	merge_const(nums, vars);
	
	std::set<OptimizingTree*> visited;
	merge_funcs(this, visited);
}

void OptimizingTree::merge_const(std::unordered_map<cnum,OptimizingTree*,cnum_hash> &nums,
								 std::map<const Element *, OptimizingTree *> &vars)
{
	size_t nc = children.size();
	for(size_t i = 0; i < nc; ++i)
	{
		OptimizingTree *c = children[i];
		
		if(c->type == OT_Constant)
		{
			auto it = nums.find(*c->value);
			if(it == nums.end())
			{
				nums[*c->value] = c;
			}
			else
			{
				OptimizingTree *cc = it->second;
				c->release();
				children[i] = cc;
				cc->retain();
			}
		}
		else if(c->type == OT_Variable)
		{
			auto it = vars.find(c->variable);
			if(it == vars.end())
			{
				vars[c->variable] = c;
			}
			else
			{
				OptimizingTree *cc = it->second;
				c->release();
				children[i] = cc;
				cc->retain();
			}
		}
		else
		{
			c->merge_const(nums, vars);
		}
	}
}

static bool can_merge_funcs(const OptimizingTree &a, const OptimizingTree &b)
{
	// never merge non-deterministic nodes (random()+random() != 2random())
	if (!a.deterministic || !b.deterministic) return false;
	
	// merge only functions, the rest is done by MergeConst
	if (a.type != OptimizingTree::OT_Function || b.type != OptimizingTree::OT_Function) return false;
	
	if (a.root() || b.root() || a.function != b.function) return false;
	int nc = a.num_children(); if (nc != b.num_children()) return false;
	
	// return true if all children are already merged
	for (int i = 0; i < nc; ++i) if (a.child(i) != b.child(i)) return false;
	return true;
}

void merge_funcs(OptimizingTree *tree, std::set<OptimizingTree*> &visited)
{
	// visited contains all unique subtrees we have seen so far
	size_t nc = tree->children.size();
	for(size_t i = 0; i < nc; ++i)
	{
		merge_funcs(tree->children[i], visited);
	}
	for(size_t i = 0; i < nc; ++i)
	{
		OptimizingTree *c = tree->children[i];
		if(c->type != OptimizingTree::OT_Function) continue;
		for(std::set<OptimizingTree*>::iterator it = visited.begin(); it != visited.end(); ++it)
		{
			if(can_merge_funcs(*c, **it))
			{
#ifdef PARSER_DEBUG
				std::cerr << "Merging " << *c << std::endl;
#endif
				assert(!visited.count(c));
				tree->children[i] = *it;
				tree->children[i]->retain();
				c->release(); c = NULL;
				break;
			}
		}
		if(c) visited.insert(c);
	}
}
