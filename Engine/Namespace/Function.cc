#include "Function.h"
#include "../Parser/WorkingTree.h"
#include "Expression.h"
#include "../Parser/Token.h"
#include "Variable.h"
#include "AliasVariable.h"
#include "RootNamespace.h"

Function::~Function()
{
	delete grd;
	for (Variable *x : grdx) delete x;
	delete grdns;
}

//----------------------------------------------------------------------------------------------------------------------
// gradient
//
// if cpx: returns (df/dz1; df/dz2; ...; df/dzn)(x1, ..., xn)
// else (grds is (df(x1+iy2, ...)/dx1; df/dy1; df/dx2; ...; df/dyn):
//    if size(x) ==   arity: returns (df/dx1; df/dx2; ...; df/dxn)(x1, ..., xn)
//    if size(x) == 2*arity: returns (df(x1+iy2, ...)/dx1; df/dy1; df/dx2; ...; df/dyn)(x1+iy2, ..., x(n-1)+ixn)
// if cdiff, we use df/dxi = df/dzi, df/dyi = idf/dzi (by Cauchy-Riemann)
//----------------------------------------------------------------------------------------------------------------------

bool Function::parse_gradient() const
{
	if (grd) return true;
	if (grds.empty()) return false;

	RootNamespace *rns = root_container(); if (!rns){ assert(false); return false; }
	grdns = new Namespace;
	grdns->link(rns);
	
	Expression xx; grdns->add(&xx);
	
	int n = arity();
	//if (cdiff)
	{
		for (int i = 0; i < n; ++i)
		{
			Variable *z = new Variable(format("z%d",i+1), true);
			grdx.push_back(z); grdns->add(z);
			xx.strings(format("re(z%d)",i+1)); AliasVariable *x = new AliasVariable(format("x%d",i+1), xx);
			xx.strings(format("im(z%d)",i+1)); AliasVariable *y = new AliasVariable(format("y%d",i+1), xx);
			
			grdns->add(x);
			grdns->add(y);
			
			if (i == 0)
			{
				grdns->add(new AliasVariable("x", x));
				grdns->add(new AliasVariable("y", y));
				grdns->add(new AliasVariable("z", z));
			}
		}
	}
	/*else
	{
		for (int i = 0; i < n; ++i)
		{
			Variable *x = new Variable(format("x%d",i+1), false);
			Variable *y = new Variable(format("y%d",i+1), false);
			grdx.push_back(x); grdns->add(x);
			grdx.push_back(y); grdns->add(y);
			AliasVariable *z = new AliasVariable(format("z%d",i+1), x, y);
			grdns->add(z);
			
			if (i == 0)
			{
				grdns->add(new AliasVariable("x", x));
				grdns->add(new AliasVariable("y", y));
				grdns->add(new AliasVariable("z", z));
			}
		}
	}*/
	
	xx.strings(grds);
	grd = xx.wt();
	if (!grd){ assert(false); return false; }
	grd = new WorkingTree(*grd);
	return true;
}

// (df/dz1, df/dz2, ..., df/dzn)(x1, ..., xn)
WorkingTree *Function::complex_gradient(const std::vector<WorkingTree> &x) const
{
	int n = arity();
	if ((int)x.size() != n){ assert(false); return NULL; }
	if (!cdiff || !parse_gradient()) return NULL;
	assert((int)grdx.size() == n && grd->num_children() == n);

	std::map<const Variable*, const WorkingTree *> vars;
	for (int i = 0; i < n; ++i) vars[grdx[i]] = &x[i];
	
	return new WorkingTree(grd->evaluate(vars));
}

// (df/dx1, df/dy1, df/dx2, ..., df/dyn)(x1, ..., xn), where f = f(x1+iy1, x2+iy2, ..., xn+iyn)
// if cdiff, we use df/dxi = df/dzi, df/dyi = i*df/dzi (Cauchy-Riemann)
WorkingTree *Function::real_gradient(const std::vector<WorkingTree> &x) const
{
	int n = arity();
	if ((int)x.size() != n){ assert(false); return NULL; }
	if (!parse_gradient()) return NULL;
	assert((int)grdx.size() == n);

	std::map<const Variable*, const WorkingTree *> vars;
	for (int i = 0; i < n; ++i) vars[grdx[i]] = &x[i];
	
	if (!cdiff)
	{
		assert(grd->num_children() == 2*n);
		return new WorkingTree(grd->evaluate(vars));
	}
	else
	{
		assert(grd->num_children() == n);
		WorkingTree t(grd->evaluate(vars));
		assert(t.num_children() == n);
		
		RootNamespace *rns = root_container(); if (!rns){ assert(false); return NULL; }
		WorkingTree *g = new WorkingTree(*rns);

		for (int i = 0; i < n; ++i)
		{
			g->add_subtree(WorkingTree(t.child(i)));
			g->add_subtree(WorkingTree(rns->Mul, WorkingTree(cnum(0.0, 1.0), *rns), std::move(t.child(i))));
		}
		return g;
	}
}
