#include "../WorkingTree.h"
#include <cassert>

//----------------------------------------------------------------------------------------------------------------------
// WorkingTree flattening
// sum(.., sum(...)...) --> sum(...), same for product
//----------------------------------------------------------------------------------------------------------------------

void WorkingTree::flatten(bool &change)
{
	for (auto &c : children) c.flatten(change);

	if (type == TT_Sum || type == TT_Product)
	{
		int nc = num_children();
		for (int i = 0; i < nc; ++i)
		{
			if (child(i).type != type) continue;
			int ngc = child(i).num_children();
			unpack_child(i);
			i  += ngc-1;
			nc += ngc-1;
			change = true;
		}
		if (nc == 0)
		{
			*this = (type == TT_Sum ? 0.0 : 1.0);
			change = true;
		}
		else if (nc == 1)
		{
			pull(0);
			change = true;
		}
	}
}

//----------------------------------------------------------------------------------------------------------------------
// WorkingTree normal form
// removes + - * / exp powOps √ id (^1) (U+) sums and products with fewer than 2 args
//----------------------------------------------------------------------------------------------------------------------

void WorkingTree::normalize()
{
	for (auto &c : children) c.normalize();

	const RootNamespace &rns = ns();

	if (is_operator(rns.Minus))
	{
		child(1).flip_involution(rns.UMinus);
		type = TT_Sum; this->rns = &rns;
	}
	else if (is_operator(rns.Plus))
	{
		type = TT_Sum; this->rns = &rns;
	}
	else if (is_operator(rns.Div))
	{
		child(1).flip_involution(rns.Invert);
		type = TT_Product; this->rns = &rns;
	}
	else if (is_operator(rns.Mul))
	{
		type = TT_Product; this->rns = &rns;
	}
	else if ((type == TT_Function || type == TT_Operator) && function->is_power())
	{
		double p; function->is_power(p);
		type = TT_Operator; operator_ = rns.Pow;
		children.emplace_back(p, rns);
	}
	else if (is_function(rns.Exp))
	{
		type = TT_Operator; operator_ = rns.Pow;
		children.emplace(children.begin(), M_E, rns);
	}
	else if (is_operator(rns.UPlus) || is_function(rns.Identity) || is_operator(rns.PowOps[1]))
	{
		pull(0);
	}
	else if (is_operator(ns().PowStar))
	{
		operator_ = ns().Pow;
	}
	else if (is_function(ns().Log_))
	{
		function = ns().Log;
	}
}

//----------------------------------------------------------------------------------------------------------------------
// WorkingTree normal form back to + - * / exp PowOps
//----------------------------------------------------------------------------------------------------------------------

void WorkingTree::denormalize()
{
	assert(verify());
	for (auto &c : children) c.denormalize();

	assert(verify());

	if (is_operator(ns().Invert) || is_operator(ns().UMinus))
	{
		if (child(0).is_operator(operator_))
		{
			pull(0,0);
		}
	}
	if (is_operator(ns().Pow))
	{
		double e;
		if (child(1).is_real(e))
		{
			if (eq(fabs(e), 0.5))
			{
				type = TT_Function; function = ns().Sqrt;
				remove_child(1);
				if (e < 0.0) pack(ns().Invert);
			}
			else if (is_int(e) && fabs(e) < 9.5)
			{
				int k = (int)std::round(e);
				if (k == -1) // z^-1 = 1/z
				{
					type = TT_Operator; operator_ = ns().Invert;
					remove_child(1);
				}
				else if (k == 0)
				{
					*this = 1.0;
				}
				else if (k == 1) // z^1 = z
				{
					pull(0);
				}
				else if (abs(k) > 1 && abs(k) < 10) // z^k = PowOp(z)
				{
					operator_ = ns().PowOps[abs(k)];
					remove_child(1);
					if (k < 0) pack(ns().Invert);
				}
			}
			else if (e < 0.0)
			{
				child(1) = -e;
				pack(ns().Invert);
			}
		}
		if (child(0) == M_E && is_operator(ns().Pow))
		{
			type = TT_Function; function = ns().Exp;
			remove_child(0);
		}
	}
	else if (type == TT_Sum)
	{
		bool first = true;
		for (auto &c : children)
		{
			if (c.type == TT_Number)
			{
				cnum z = (cnum)c; assert(!isz(z));
				if (!first && prints_sign(z))
				{
					c = -z;
					c.pack(ns().UMinus);
				}
			}
			else if (c.is_negation() && c.child(0).type == TT_Number)
			{
				cnum z = (cnum)c.child(0); assert(!isz(z));
				if (first || prints_sign(z))
				{
					c.child(0) = -z;
					c.pull(0);
				}
			}
			first = false;
		}

		switch (num_children())
		{
			case 0: *this = 0.0; break;
				
			case 1: pull(0); break;
				
			case 2:
				if (!child(0).is_negation() && child(1).is_negation())
				{
					type = TT_Operator; operator_ = rns->Minus;
					child(1).pull(0);
					break;
				}
				else if (child(0).is_negation() && !child(1).is_negation())
				{
					std::swap(children[0], children[1]);
					type = TT_Operator; operator_ = rns->Minus;
					child(1).pull(0);
					break;
				}
				type = TT_Operator; operator_ = rns->Plus;
				// fallthrough
				
			default:
			{
				bool all_neg = true;
				for (auto &c : children) if (!c.is_negation()){ all_neg = false; break; }
				if (all_neg)
				{
					for (auto &c : children) c.pull(0);
					pack(ns().UMinus);
				}
			}
		}
	}
	else if (type == TT_Product)
	{
		for (auto & c : children)
		{
			if (c.type == TT_Number)
			{
				cnum z = (cnum)c;
				if (::uglyness(z) > ::uglyness(inverse(z)))
				{
					c = inverse(z);
					c.pack(ns().Invert);
				}
			}
			else if (c.is_inversion() && c.child(0).type == TT_Number)
			{
				cnum z = (cnum)c.child(0);
				if (::uglyness(z) > ::uglyness(inverse(z)))
				{
					c.pull(0);
					c = inverse(z);
				}
			}
		}
		
		int nc = num_children();
		switch (nc)
		{
			case 0: *this = 1.0; break;
				
			case 1: pull(0); break;
				
			case 2:
				if (!child(0).is_inversion() && child(1).is_inversion())
				{
					type = TT_Operator; operator_ = rns->Div;
					child(1).pull(0);
					break;
				}
				else if (child(0).is_inversion() && !child(1).is_inversion())
				{
					std::swap(children[0], children[1]);
					type = TT_Operator; operator_ = rns->Div;
					child(1).pull(0);
					break;
				}
				
				type = TT_Operator; operator_ = ns().Mul;
				// fallthrough
				
			default:
			{
				int n_inv = 0;
				for (auto &c : children) if (c.is_inversion()) ++n_inv;
				
				if (n_inv == nc)
				{
					for (auto &c : children) c.pull(0);
					pack(ns().Invert);
				}
				else if (n_inv == 1 && !children.back().is_inversion())
				{
					for (int i = 0; i < nc; ++i)
					{
						if (!child(i).is_inversion()) continue;
						move_child(i, nc-1);
						break;
					}
				}
				else if (n_inv > 1)
				{
					pack(ns().Div, true);
					children.emplace(children.end(), &ns(), TT_Product, private_key());
					auto &c0 = child(0), &c1 = child(1);
					for (int i = 0; i < nc; ++i)
					{
						if (!c0.child(i).is_inversion()) continue;
						c0.child(i).pull(0);
						c1.children.push_back(std::move(c0.child(i)));
						c0.remove_child(i);
						--i; --nc;
					}
					for (auto &c : children)
					{
						if (c.num_children() != 2) continue;
						c.type = TT_Operator;
						c.operator_ = ns().Mul;
					}
				}
			}
		}
	}
	assert(verify());
}
