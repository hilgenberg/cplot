#include "../WorkingTree.h"
#include <cassert>

void WorkingTree::simplify_products(bool &change)
{
	for (auto &c : children) c.simplify_products(change);
	
	if (is_operator(ns().UMinus) && child(0).is_operator(ns().UMinus))
	{
		pull(0,0);
		change = true;
		return;
	}
	
	if (type != TT_Product) return;

	int nc = num_children();
	
	//------------------------------------------------------------------------------------------------------------------
	// sort constants to the front, collect negations
	//------------------------------------------------------------------------------------------------------------------
	int n_const = 0;
	bool s = false;
	
	for (int i = 0; i < nc; ++i)
	{
		auto &c = child(i);
		
		if (c.is_negation()){ s = !s; c.pull(0); change = true; }
		
		if (c.is_constant(true)) move_child(i, n_const++); // no real change here
	}

	//------------------------------------------------------------------------------------------------------------------
	// remove +-1, handle 0, prettify constants (0.5 --> 1/2, ...)
	//------------------------------------------------------------------------------------------------------------------

	std::vector<cnum> vals;
	for (int i = 0; i < n_const; ++i)
	{
		auto &c = child(i);
		cnum z = c.evaluate();
		
		if (isz(z))
		{
			*this = 0.0;
			change = true;
			return;
		}
		if (::is_one(z) || ::is_minusone(z))
		{
			remove_child(i);
			--nc; --n_const; --i;
			if (z.real() < 0.0) s = !s;
			change = true;
			continue;
		}
		
		if (c.type == TT_Number)
		{
			if (z.real() <= 0.0 && z.imag() <= 0.0)
			{
				c = z = -z;
				s = !s;
				change = true;
			}
		}
		else if (c.is_inversion() && c.child(0).type == TT_Number)
		{
			if (z.real() <= 0.0 && z.imag() <= 0.0)
			{
				c.child(0) = z = -z;
				s = !s;
				change = true;
			}
		}
		
		vals.push_back(z);
	}
	
	//------------------------------------------------------------------------------------------------------------------
	// combine constants
	//------------------------------------------------------------------------------------------------------------------

	for (int i = 0; i+1 < n_const; ++i)
	{
		for (int j = i+1; j < n_const; ++j)
		{
			if (::uglyness(vals[i]*vals[j]) <= child(i).uglyness() + child(j).uglyness())
			{
				change = true;
				vals[i] *= vals[j];
				remove_child(j);
				vals.erase(vals.begin()+j);

				if (isz(vals[i]))
				{
					*this = 0.0;
					return;
				}
				if (::is_one(vals[i]) || ::is_minusone(vals[i]))
				{
					remove_child(i);
					vals.erase(vals.begin()+i);
					n_const -= 2; nc -= 2; --i;
					break;
				}
				else
				{
					children[i] = vals[i];
					--n_const; --nc; --j;
				}
			}
#if 0 // TODO
			else if (child(i).type == TT_Number && is_inversion(child(j)) && child(j).child(0).type == TT_Number)
			{
				change |= cancel(child(i).num->second, child(j).child(0).num->second);
			}
			else if (child(j).type == TT_Number && is_inversion(child(i)) && child(i).child(0).type == TT_Number)
			{
				change |= cancel(child(j).num->second, child(i).child(0).num->second);
			}
			// test for +-1 again
#endif
		}
	}
	
	//------------------------------------------------------------------------------------------------------------------
	// combine X^a * X^b into X^(a+b)
	//------------------------------------------------------------------------------------------------------------------
	
	for (int i = n_const; i < nc; ++i)
	{
		assert(nc == num_children());
		for (int j = i+1; j < nc; ++j)
		{
			int d = combine_product(i, j);
			nc -= d;
			if (d)
			{
				change = true;
				// restart at child(i) even if d == 1 because it could be child(j) now
				--i;
				break;
			}
		}
	}

	//------------------------------------------------------------------------------------------------------------------
	// check 1-arg and empty product and add sign
	//------------------------------------------------------------------------------------------------------------------

	if (nc == 0)
	{
		*this = (s ? -1.0 : 1.0);
		change = true;
		return;
	}
	if (nc == 1)
	{
		pull(0);
		change = true;
	}
	
	if (s){ assert(change); flip_involution(ns().UMinus); }
}

//----------------------------------------------------------------------------------------------------------------------
// WorkingTree helper methods - combining children in a product
//----------------------------------------------------------------------------------------------------------------------

void WorkingTree::add_power(double p)
{
	if (is_operator(ns().Pow)){ child(1) += p; return; }
	pack(ns().Pow, true);
	children.emplace_back(p + 1.0, ns());
	assert(verify());
}
void WorkingTree::add_power(WorkingTree &&p)
{
	if (is_operator(ns().Pow)){ child(1) += std::move(p); return; }
	pack(ns().Pow, true);
	children.emplace_back(std::move(p));
	child(1) += 1.0;
	assert(verify());
}

int WorkingTree::combine_product(int i, int j)
{
	assert(i < j);
	WorkingTree *ci = &child(i), *cj = &child(j);
	bool si = false, sj = false;
	if (ci->is_inversion()){ si = true; ci = &ci->child(0); } bool pi = ci->is_operator(ns().Pow);
	if (cj->is_inversion()){ sj = true; cj = &cj->child(0); } bool pj = cj->is_operator(ns().Pow);

	if (!pi && !pj)
	{
		if (*ci != *cj) return 0;
		
		if (si != sj)
		{
			remove_child(j);
			remove_child(i);
			return 2;
		}

		remove_child(j);
		ci->pack(ns().Pow, true);
		ci->children.emplace_back(2.0, ns());
		return 1;
	}

	double bi; WorkingTree &bxi = pi ? ci->child(0) : *ci; if (!bxi.is_real(bi)) bi = UNDEFINED;
	double bj; WorkingTree &bxj = pj ? cj->child(0) : *cj; if (!bxj.is_real(bj)) bj = UNDEFINED;

	// check base vs. base if both are powers
	if (pi && pj && defined(bi) == defined(bj) && (defined(bi) && eq(bi, bj) || !defined(bi) && bxi == bxj))
	{
		double ei; WorkingTree &exi = ci->child(1); if (!exi.is_real(ei)) ei = UNDEFINED;
		double ej; WorkingTree &exj = cj->child(1); if (!exj.is_real(ej)) ej = UNDEFINED;

		if (defined(ei) && defined(ej))
		{
			if (si != sj) ej = -ej;
			if (isz(ei+ej))
			{
				remove_child(j);
				remove_child(i);
				return 2;
			}
			ci->add_power(ej);
			remove_child(j);
			return 1;
		}
		
		if (defined(ej)){ if (si != sj) ej = -ej; ci->add_power(ej); }
		else            { if (si != sj) exj.flip_involution(ns().UMinus); ci->add_power(std::move(exj)); }
		remove_child(j);
		return 1;
	}

	// check base vs. complete if at least one is a power
	double tmp;
	if (pi && (defined(bi) && cj->is_real(tmp) && eq(tmp, bi) || !defined(bi) && bxi == *cj))
	{
		double ei; WorkingTree &exi = ci->child(1); if (!exi.is_real(ei)) ei = UNDEFINED;
		double ej = (si == sj ? 1.0 : -1.0);
		
		if (defined(ei) && isz(ei+ej))
		{
			remove_child(j);
			remove_child(i);
			return 2;
		}
		
		ci->add_power(ej);
		remove_child(j);
		return 1;
	}
	if (pj && (defined(bj) && ci->is_real(tmp) && eq(tmp, bj) || !defined(bj) && bxj == *ci))
	{
		double ej; WorkingTree &exj = cj->child(1); if (!exj.is_real(ej)) ej = UNDEFINED;

		if (si != sj){ if (defined(ej)) ej = -ej; else exj.flip_involution(ns().UMinus); }
		
		if (defined(ej) && eq(ej, -1.0))
		{
			remove_child(j);
			remove_child(i);
			return 2;
		}
		
		ci->pack(ns().Pow, true);
		ci->add_child(WorkingTree(1.0, ns()));
		if (defined(ej)) ci->add_power(ej); else ci->add_power(std::move(exj));
		remove_child(j);
		return 1;
	}
	
	// no need to check complete vs. complete because they would have the same base
	return 0;
}

