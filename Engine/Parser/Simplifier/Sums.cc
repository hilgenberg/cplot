#include "../WorkingTree.h"
#include <cassert>

//----------------------------------------------------------------------------------------------------------------------
// WorkingTree helper methods - flatten and unflatten
//----------------------------------------------------------------------------------------------------------------------

void WorkingTree::simplify_sums(bool &change)
{
	for (auto &c : children) c.simplify_sums(change);
	if (type != TT_Sum) return;

	
	int nc = num_children();
	
	//------------------------------------------------------------------------------------------------------------------
	// sort constants to the front
	//------------------------------------------------------------------------------------------------------------------
	int n_const = 0;
	std::vector<cnum> vals;
	
	for (int i = 0; i < nc; ++i)
	{
		auto &c = child(i);
		if (c.is_constant(true))
		{
			cnum z = c.evaluate();
			if (isz(z))
			{
				remove_child(i);
				--nc; --i;
				change = true;
				continue;
			}
			move_child(i, n_const++); // no real change here
			vals.push_back(z);
		}
	}
	
	//------------------------------------------------------------------------------------------------------------------
	// combine constants
	//------------------------------------------------------------------------------------------------------------------
	
	for (int i = 0; i+1 < n_const; ++i)
	{
		for (int j = i+1; j < n_const; ++j)
		{
			if (::uglyness(vals[i]+vals[j]) <= child(i).uglyness() + child(j).uglyness())
			{
				change = true;
				vals[i] += vals[j];
				remove_child(j); vals.erase(vals.begin()+j);
				
				if (isz(vals[i]))
				{
					remove_child(i); vals.erase(vals.begin()+i);
					n_const -= 2; nc -= 2; --i;
					break;
				}
				else
				{
					children[i] = vals[i];
					--n_const; --nc; --j;
				}
			}
		}
	}
	
	//------------------------------------------------------------------------------------------------------------------
	// (4) combine a*X+b*X into (a+b)*X
	//------------------------------------------------------------------------------------------------------------------

	for (int i = n_const; i < nc; ++i)
	{
		assert(nc == num_children());
		for (int j = i+1; j < nc; ++j)
		{
			int d = combine_sum(i, j);
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
	// check 1-arg and empty sums
	//------------------------------------------------------------------------------------------------------------------
	
	if (nc == 0)
	{
		*this = 0.0;
		change = true;
	}
	else if (nc == 1)
	{
		pull(0);
		change = true;
	}
}

//----------------------------------------------------------------------------------------------------------------------
// WorkingTree helper methods - combining children in a sum
//----------------------------------------------------------------------------------------------------------------------

bool WorkingTree::sum_compatible(const WorkingTree &A, const WorkingTree &B,
							 cnum &a_coeff, cnum &b_coeff,
							 int &a_coeff_i, int &b_coeff_i)
{
	assert(&A != &B);
	a_coeff_i = b_coeff_i = -1;
	
	std::vector<const WorkingTree*> a, b;
	if (A.type == TT_Product) for (auto &c : A) a.push_back(&c); else a.push_back(&A);
	if (B.type == TT_Product) for (auto &c : B) b.push_back(&c); else b.push_back(&B);
	std::vector<bool> a_done(a.size(), false), b_done(b.size(), false);
	
	int na = (int)a.size(), nb = (int)b.size();
	if (abs(na - nb) > 1) return false;
	
	for (int i = 0; i < na; ++i)
	{
		if (a_done[i]) continue;
		bool found = false;
		for (int j = 0; j < nb; ++j)
		{
			if (b_done[j]) continue;
			
			if (*a[i] == *b[j])
			{
				a_done[i] = b_done[j] = true;
				found = true;
				break;
			}
		}
		if (!found && a[i]->type != TT_Number) return false;
	}
	
	for (int i = 0; i < na; ++i)
	{
		if (a_done[i]) continue;
		if (a[i]->type != TT_Number) return false;
		if (a_coeff_i >= 0) return false;
		a_coeff_i = i;
		a_coeff = (cnum)*a[i];
	}
	for (int i = 0; i < nb; ++i)
	{
		if (b_done[i]) continue;
		if (b[i]->type != TT_Number) return false;
		if (b_coeff_i >= 0) return false;
		b_coeff_i = i;
		b_coeff = (cnum)*b[i];
	}
	return true;
}

int WorkingTree::combine_sum(int i, int j)
{
	assert(i < j);
	WorkingTree *ci = &child(i), *cj = &child(j);
	bool si = false, sj = false;
	if (ci->is_negation()){ si = true; ci = &ci->child(0); }
	if (cj->is_negation()){ sj = true; cj = &cj->child(0); }
	
	cnum xi, xj; int xii, xjj;
	if (!sum_compatible(*ci, *cj, xi, xj, xii, xjj)) return 0;

	if (xii < 0 && xjj < 0)
	{
		remove_child(j);
		if (si != sj){ remove_child(i); return 2; }
		*ci *= 2.0;
		return 1;
	}
	if (xii < 0)
	{
		std::swap(xii, xjj);
		std::swap(xi,  xj);
		std::swap(children[i], children[j]);
		std::swap(ci, cj);
		// keep the order of i and j
	}
	assert(xii >= 0);
	if (xjj < 0) xj = 1.0;
	if (si == sj) xi += xj; else xi -= xj;
	if (isz(xi))
	{
		remove_child(j);
		remove_child(i);
		return 2;
	}
	ci->child(xii) = xi;
	remove_child(j);
	return 1;
}
