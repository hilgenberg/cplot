#include "../WorkingTree.h"
#include "../../Namespace/Function.h"

void WorkingTree::simplify_unary(bool &change)
{
	for (auto &c : children) c.simplify_unary(change);
	if (children.empty()) return;

	const RootNamespace &rns = ns();

	if (is_operator(rns.Pow) && child(0).type == TT_Product && child(1).is_constant(true))
	{
		auto &b = child(0), &e = child(1);
		for (auto &c : b.children)
		{
			c.pack(rns.Pow, true);
			c.children.emplace_back(e);
		}
		pull(0);
		change = true;
		return;
	}
	
	if (!(type == TT_Operator || type == TT_Function && function->base())) return;

	const Function *f = function;
	if (f->arity() != 1) return;
	
	WorkingTree *t = &child(0);
	int redo = 0;
	while (true)
	{
		// thread into sums and products so flatten and cancelling work better
		if (t->type == TT_Sum && f->additive() || t->type == TT_Product && f->multiplicative())
		{
			if (f->involution())
			{
				for (auto &c : t->children) c.flip_involution(f);
			}
			else
			{
				for (auto &c : t->children) c.pack(f);
			}
			pull(0);
			change = true;
			redo = -1;
			break;
		}
		
		if (!(t->type == TT_Operator || t->type == TT_Function && t->function->base())) break;
		
		const Function *g = t->function;
		bool done = false;
		switch(rns.combine(f, g))
		{
			case RootNamespace::Nothing:  done = true; break;
			case RootNamespace::Commutes: assert(t->num_children() == 1); t = &t->child(0); break;
			case RootNamespace::Zero:
				change = true;
				if (g == f){ *this = 0.0; return; }else{ *t = 0.0; done = true; break; }
			case RootNamespace::One:
				change = true;
				if (g == f){ *this = 1.0; return; }else{ *t = 1.0; done = true; break; }
			case RootNamespace::First:
				change = true;
				assert(t->num_children() == 1);
				t->pull(0);
				if (t != &child(0)) redo = 1;
				break;
			case RootNamespace::Second: change = true; pull(0); return;
			case RootNamespace::Collapses:
				assert(t->num_children() == 1);
				change = true;
				t->pull(0); pull(0);
				if (t != &child(0)) redo = -1;
				done = true;
				break;
		}
		if (done) break;
	}
	
	if (redo == -1) return simplify_unary(change);
	if (redo ==  1) for (auto &c : children) c.simplify_unary(change);
}
