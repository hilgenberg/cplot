#include "WorkingTree.h"
#include "../Namespace/Function.h"
#include "../Namespace/Variable.h"
#include "../Namespace/RootNamespace.h"
#include <cassert>
#include <functional>
#include <memory>

WorkingTree WorkingTree::add(WorkingTree &&a, WorkingTree &&b)
{
	if (a.is_zero()) return b;
	if (b.is_zero()) return a;
	
	if (b.is_negation()) return WorkingTree(a.ns().Minus, std::move(a), std::move(b.child(0)));
	if (a.is_negation()) return WorkingTree(a.ns().Minus, std::move(b), std::move(a.child(0)));
	
	return WorkingTree(a.ns().Plus, std::move(a), std::move(b));
}
WorkingTree WorkingTree::mul(WorkingTree &&a, WorkingTree &&b)
{
	if (a.is_zero()) return a;
	if (b.is_zero()) return b;
	if (a.is_one ()) return b;
	if (b.is_one ()) return a;
	
	if (a.is_minus_one())
	{
		if (b.is_minus_one()) return WorkingTree(1.0, a.ns());
		if (b.is_negation()) return WorkingTree(std::move(b.child(0)));
		return WorkingTree(b.ns().UMinus, std::move(b));
	}
	if (b.is_minus_one()) return WorkingTree(b.ns().UMinus, std::move(a));
	
	if (b.is_inversion()) return WorkingTree(a.ns().Div, std::move(a), std::move(b.child(0)));
	if (a.is_inversion()) return WorkingTree(a.ns().Div, std::move(b), std::move(a.child(0)));
	
	return WorkingTree(a.ns().Mul, std::move(a), std::move(b));
}

//----------------------------------------------------------------------------------------------------------------------
// derivative
// caller must delete the result
//----------------------------------------------------------------------------------------------------------------------

WorkingTree *WorkingTree::derivative(const Variable &x, std::string &error) const
{
	bool cpx = !x.real();
	switch (type)
	{
		case TT_Root:
		{
			WorkingTree *ret = new WorkingTree(ns());
			for (auto &c : children)
			{
				WorkingTree *dc = c.derivative(x, error);
				if (!dc){ delete ret; return NULL; }
				ret->add_child(std::move(*dc)); delete dc;
			}
			return ret;
		}
			
		case TT_Constant:
		case TT_Parameter:
		case TT_Number:   return new WorkingTree(0.0, ns());
			
		case TT_Variable: return new WorkingTree(variable == &x ? 1.0 : 0.0, ns());
			
		case TT_Function:
		case TT_Operator:
		{
			if (function->arity() == 0)
			{
				if (!function->deterministic())
				{
					// must be one of the random() functions
					error = format("%s is nowhere differentiable", function->displayName(PS_Console).c_str());
					return NULL;
				}
				// user-defined computed constant
				return new WorkingTree(0.0, ns());
			}
			
			std::unique_ptr<WorkingTree> grad(cpx ? function->complex_gradient(children)
											  : function->real_gradient(children));
			if (!grad)
			{
				error = format("%s is not %sdifferentiable (or derivative is not implemented)",
							   function->displayName(PS_Console).c_str(),
							   cpx ? "complex " : "");
				return NULL;
			}
			
			// chain rule: inner derivatives times gradient
			WorkingTree *ret = new WorkingTree(&ns(), TT_Sum);
			
			if (cpx)
			{
				for (int i = 0, n = num_children(); i < n; ++i)
				{
					WorkingTree *dc = child(i).derivative(x, error);
					if (!dc){ delete ret; return NULL; }
					ret->add_child(mul(std::move(*dc), std::move(grad->child(i)))); delete dc;
				}
			}
			else
			{
				assert(grad->num_children() == 2*num_children());
				
				for (int i = 0, n = num_children(); i < n; ++i)
				{
					WorkingTree *dc = child(i).derivative(x, error);
					if (!dc){ delete ret; return NULL; }
					if (dc->is_real())
					{
						ret->add_child(mul(std::move(*dc), std::move(grad->child(2*i)))); delete dc;
					}
					else
					{
						WorkingTree &gx = grad->child(2*i);
						WorkingTree &gy = grad->child(2*i+1);
						
						// add re(dc)*gx + im(dc)*gy
						
						if (gy.is_operator(ns().Mul) && gy.child(0) == cnum(0.0, 1.0) && gy.child(1) == gx)
						{
							// gy = i*gx  =>  re(dc)*gx + im(dc)*gy = dc*gx
							ret->add_child(mul(std::move(*dc), std::move(gx))); delete dc;
						}
						else
						{
							WorkingTree redc(ns().Re, WorkingTree(*dc));
							WorkingTree imdc(ns().Im, std::move(*dc)); delete dc;
							ret->add_child(mul(std::move(redc), std::move(gx)));
							ret->add_child(mul(std::move(imdc), std::move(gy)));
						}
					}
				}
			}
			
			ret->simplify(true);
			assert(ret->verify());
			return ret;
		}
			
		case TT_Sum:
		{
			WorkingTree *ret = new WorkingTree(element, type);
			for (auto &c : children)
			{
				WorkingTree *dc = c.derivative(x, error);
				if (!dc){ delete ret; return NULL; }
				ret->add_child(std::move(*dc)); delete dc;
			}
			ret->simplify(true);
			assert(ret->verify());
			return ret;
		}
			
		case TT_Product:
		{
			WorkingTree *ret = new WorkingTree(element, TT_Sum);
			for (int i = 0, n = num_children(); i < n; ++i)
			{
				ret->add_child(WorkingTree(element, type));
				WorkingTree &t = ret->children.back();
				for (int j = 0; j < n; ++j)
				{
					if (j == i)
					{
						WorkingTree *dc = child(j).derivative(x, error);
						if (!dc){ delete ret; return NULL; }
						t.add_child(std::move(*dc)); delete dc;
					}
					else
					{
						t.add_child(WorkingTree(child(j)));
					}
				}
			}
			ret->simplify(true);
			assert(ret->verify());
			return ret;
		}

		default: assert(false); return NULL;
	}
}

//----------------------------------------------------------------------------------------------------------------------
// split_var - replace f(z) by f(x+iy) (for partial derivatives)
// caller must delete the result
//----------------------------------------------------------------------------------------------------------------------

WorkingTree *WorkingTree::split_var(const Variable &z, const Variable &x, const Variable &y) const
{
	assert(x.real() && y.real() && !z.real());
	switch (type)
	{
		case TT_Constant:
		case TT_Parameter:
		case TT_Number:   return new WorkingTree(*this);
			
		case TT_Variable:
		{
			assert(variable != &x && variable != &y);
			if (variable != &z) return new WorkingTree(*this);
			return new WorkingTree(ns().Combine, WorkingTree(&x), WorkingTree(&y));
		}
			
		case TT_Root:
		case TT_Sum:
		case TT_Product:
		case TT_Function:
		case TT_Operator:
		{
			WorkingTree *ret = new WorkingTree(element, type);
			for (auto &c : children)
			{
				WorkingTree *dc = c.split_var(z, x, y); assert(dc);
				ret->add_child(std::move(*dc)); delete dc;
			}
			assert(ret->verify());
			return ret;
		}

		default: assert(false); return NULL;
	}
}
