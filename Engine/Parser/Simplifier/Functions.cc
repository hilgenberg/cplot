#include "../WorkingTree.h"

//----------------------------------------------------------------------------------------------------------------------
// WorkingTree::simplify_functions
//----------------------------------------------------------------------------------------------------------------------

void WorkingTree::simplify_functions(bool &change)
{
	for (auto &c : children) c.simplify_functions(change);

	switch (type)
	{
		case TT_Root:
		case TT_Constant:
		case TT_Number:
		case TT_Variable:
		case TT_Parameter: return;
			
		case TT_Sum:
		case TT_Product:
		case TT_Function:
		case TT_Operator:
			if (is_constant(true))
			{
				cnum z = evaluate();
				if (defined(z) && ::uglyness(z) <= uglyness())
				{
					*this = z;
					change = true;
					return;
				}
			}
			break;
	}

	const RootNamespace &rns = ns();

	if (type == TT_Function)
	{
		const Function *f = function;
		if ((f == rns.Re || f == rns.Conj) && child(0).is_real()
			|| f == ns().Combine && child(1).is_zero())
		{
			pull(0);
			change = true;
			return;
		}
		if (f == rns.Im || f == rns.Re)
		{
			if (child(0).is_real())
			{
				if (f == rns.Re) pull(0); else *this = 0.0;
				change = true;
			}
			else if (child(0).type == TT_Product)
			{
				int k = 0; bool rf = false;
				for (auto &c : child(0))
				{
					if (c.is_real())
					{
						rf = true;
					}
					else if (c.type == TT_Number)
					{
						cnum z = (cnum)c;
						if (is_imag(z)){ double x = z.imag(); k += (x >= 0.0 ? 1 : 3); c = fabs(x); change = rf = true;}
					}
					else if (c.type == TT_Constant && c.constant == rns.I)
					{
						++k; c = 1.0; change = true; // leave rf as is, the 1 will be removed inside too
					}
				}
				
				bool s = (k & 2);
				if ((k & 1))
				{
					if (f == rns.Re) s = !s; // re(iz) = -imz, im(iz) = re(z)
					function = (f == rns.Im ? rns.Re : rns.Im);
				}
				
				if (rf) // move any real factors outside
				{
					int n = child(0).num_children();
					pack(TT_Product, rns); // prod(re/im(prod(...))) now
					for (int i = 0; i < n; ++i)
					{
						auto &c = child(0).child(0).child(i);
						if (c.is_real())
						{
							add_child(std::move(c));
							child(0).child(0).remove_child(i);
							--i; --n;
						}
					}
				}
				
				if (s) flip_involution(rns.UMinus);
			}
			// re(complex(x,y)) = x, im(complex(x,y)) = y
			else if (child(0).is_function(rns.Combine) && child(0).has_real_children())
			{
				pull(0, f == rns.Re ? 0 : 1);
				change = true;
			}
			return;
		}
		// abs(a+ib) = hypot(a,b)
		if (f == rns.Abs && child(0).is_function(rns.Combine) && child(0).has_real_children())
		{
			pull(0);
			function = rns.Hypot;
			change = true;
			return;
		}
	}
	else if (type == TT_Operator)
	{
		// ~x = x
		const Operator *op = operator_;
		if (op == rns.ConjOp && child(0).is_real())
		{
			pull(0);
			change = true;
			return;
		}
		if (op == rns.Pow && child(0) == M_E && child(0).is_function(rns.Log))
		{
			pull(0, 0);
			change = true;
			return;
		}
	}
}

//----------------------------------------------------------------------------------------------------------------------
// WorkingTree::combine_function_product
// sets F to F*G, F/G, F/~G, ... and returns true or returns false if it can not be simplified
// the result will be inverted if a_inv is true and conugated if a_conj is true
//----------------------------------------------------------------------------------------------------------------------

bool WorkingTree::combine_function_product(WorkingTree &F, WorkingTree &G, bool a_inv, bool b_inv, bool a_conj, bool b_conj)
{
	assert(F.is_function() && G.is_function());

	(void)a_inv;
	(void)b_inv;
	(void)a_conj;
	(void)b_conj;
#if 0
	TODO...
	bool same_fn = (F.function == G.function);
	
	bool same_args = (F.num_children() == G.num_children());
	for (int i = 0, n = F.num_children(); same_args && i < n; ++i)
	{
		if (F.child(i) != G.child(i)) same_args = false;
	}
#endif
	return false;
}

