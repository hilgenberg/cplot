#include "Evaluator.h"
#include "ExecToken.h"
#include "OptimizingTree.h"
#include "../../Utility/StringFormatting.h"
#include "../Namespace/Function.h"
#include "../Namespace/Parameter.h"

#include <assert.h>
#include <iostream>
#include <list>
#include <set>
#include <algorithm>
#include <functional>
#include <climits>

using std::set;
using std::map;
using std::vector;

using namespace CP_PARSER;

//----------------------------------------------------------------------------------------------------------------------
//  Utilities
//----------------------------------------------------------------------------------------------------------------------

void Evaluator::set_parameters(const std::set<Parameter*> &params)
{
	assert(ctx);
	if (!ctx) return;
	
	for (Parameter *p : params)
	{
		int i = var_index(p);
		if (i >= 0) ctx->set_input(i, p->value());
	}
}

typedef const OptimizingTree *PCOT; // "Pointer to Constant Optimizing Tree"

static void collect(PCOT tree, set<PCOT> &constants, set<PCOT> &variables, set<PCOT> &parameters, set<PCOT> &functions)
{
	switch (tree->type)
	{
		case OptimizingTree::OT_Constant:
			constants.insert(tree);
			break;
			
		case OptimizingTree::OT_Variable:
			(tree->variable->isVariable() ? variables : parameters).insert(tree);
			break;
			
		case OptimizingTree::OT_Function:
			if(!tree->root()) functions.insert(tree);
			for(auto *c : *tree) collect(c, constants, variables, parameters, functions);
			break;
	}
}

// how many nodes are dependent on item
static size_t get_usage(PCOT tree, PCOT item)
{
	if (tree == item) return 1;
	
	size_t n = 0;
	for (auto *c : *tree) n += get_usage(c, item);
	if (n && !tree->root()) ++n; // add 1 for tree as well
	return n;
}

//----------------------------------------------------------------------------------------------------------------------
// PCOT POOL will be filled with subtrees, grouped into those already and those ready to be converted to ExecTokens.
// The subtrees will be ordered during the conversion so that arguments will be evaluated before the trees that
// use them. This still leaves some freedom for optimization regarding EvalContext::get_last_change (see below).
// The POOL helps with this (somewhat convoluted) process.
// Order of operations:
// (1) all trees that are ready to be converted (because all their parameters are already converted) go into the pool
// (2) the next tree to be converted is fished out of the pool, converted, and put into pool.done
// (3) if the pool is not empty, goto 1
//----------------------------------------------------------------------------------------------------------------------

struct POOL
{
	std::list<PCOT> list; // contents of the pool, ordered by PoolCmp
	std::set <PCOT> set;  // the same objects in a set for faster membership testing
	std::set <PCOT> done; // these are already converted to ExecTokens
	std::map <PCOT, std::set<PCOT> > deps; // tree -> vars (as PCOTs) that it depends on

#ifdef DEBUG
	void print()
	{
		using std::cerr;
		using std::endl;
		cerr << endl << "Pool List: " << endl;
		for (auto x : list)
		{
			cerr << *x << endl;
		}
		cerr << endl << "Pool Set: " << endl;
		for (auto x : set)
		{
			cerr << *x << endl;
		}
		
		assert(list.size() == set.size());
	}
#endif
	
	//------------------------------------------------------------------------------------------------------------------
	// Step (1): Collects all functions that do not depend on other functions into the pool.
	//           This does the initial filling of the pool.
	//------------------------------------------------------------------------------------------------------------------

	void initialize(PCOT tree)
	{
		
		if (set.count(tree) || tree->type != OptimizingTree::OT_Function) return;
		
		bool leaf = true;
		for (auto c : *tree)
		{
			if (c->type != OptimizingTree::OT_Function) continue;
			initialize(c);
			leaf = false; // if any child is a function, this is not a func_leaf
		}
		
		if (leaf && !tree->root() && !set.count(tree))
		// set.count(tree) again because another initialize might have put it in the pool
		// the tree->root() test is here because we want to initialize root's children
		{
			set.insert(tree);
			list.push_back(tree);
			std::set<PCOT> &tdeps = deps[tree];
			for (auto c : *tree) if (c->type == OptimizingTree::OT_Variable) tdeps.insert(c);
		}
	}

	//------------------------------------------------------------------------------------------------------------------
	// Step (2): Define a sort order that determines the next tree to be fished out of the pool
	//------------------------------------------------------------------------------------------------------------------
private:
	std::function<bool(PCOT,PCOT)> cmp;
public:
	POOL(map<PCOT, long> &result_index)
	{
		// The next tree will be taken from the front of list, so sorting to the front means earlier evaluation.
		// The order here is purely for optimization (because all trees that are already in the pool can be
		// converted; their dependencies are handled by initialize and update), so errors will be rather hard to
		// track down and not catastrophic.
		
		cmp = [&](PCOT a, PCOT b) -> bool
		{
			// non-deterministic functions should come as late as possible because they must always be calculated
			// whereas stuff near the front of the resulting ExecToken array can possibly be skipped
			if( a->deterministic && !b->deterministic) return true;
			if(!a->deterministic &&  b->deterministic) return false;
			if(!a->deterministic && !b->deterministic) return false; // don't care
			
			// What we can use now is the var_order that was passed to Evaluator::Evaluator() and which ends
			// up in result_index (in the same order, but restricted to the variables that are used).

			// For a and b find their minimum result_order[variable they depend on]
			const std::set<PCOT> &ad = deps[a], &bd = deps[b];
			long amin = LONG_MAX, bmin = LONG_MAX;
			
			for (auto x : ad) if (!bd.count(x)){ long n = result_index[x]; if(n < amin) amin = n; }
			for (auto x : bd) if (!ad.count(x)){ long n = result_index[x]; if(n < bmin) bmin = n; }
			
			// Define a < b if bmin < amin, which means that b depends on a var that changes faster than
			// every var that a depends on (because of how var_order is supposed to work).

			//if(bmin < amin) std::cerr << *a << " < " << *b << std::endl;
			return bmin < amin;
		};
	}
	
	void sort()
	{
		list.sort(cmp);
	}

	//------------------------------------------------------------------------------------------------------------------
	// Step (3) / (1'): Add new nodes into the pool when all their children are converted
	//------------------------------------------------------------------------------------------------------------------

	void update(OptimizingTree *tree)
	{
		assert(tree->type == OptimizingTree::OT_Function);
		if (done.count(tree) || set.count(tree)) return;
		
		bool collect = !tree->root(); // root is never put into the pool
		
		// recursion over all children + find out if this node can be collected
		for (auto c : *tree)
		{
			if (c->type != OptimizingTree::OT_Function) continue;
			update(c);
			if (!done.count(c)) collect = false; // unconverted child
		}
		if (!collect) return;
		
		std::set<PCOT> &tdeps = deps[tree];
		for (auto c : *tree)
		{
			switch (c->type)
			{
				case OptimizingTree::OT_Constant: break;
				case OptimizingTree::OT_Variable: tdeps.insert(c); break;
				case OptimizingTree::OT_Function:
				{
					auto cdeps = deps[c];
					tdeps.insert(cdeps.begin(), cdeps.end());
				}
			}
		}

		// insert and keep list sorted
		auto lb = std::lower_bound(list.begin(), list.end(), tree, cmp);
		list.insert(lb, tree);
		set.insert(tree);
	}
};


//----------------------------------------------------------------------------------------------------------------------
// Evaluator: Destructor, Constructor
//----------------------------------------------------------------------------------------------------------------------

Evaluator::~Evaluator()
{
	if (funcs) for(ExecToken **t = funcs; *t; ++t) delete *t;
	delete [] funcs;
	delete [] start;
	delete ctx;
}

Evaluator::Evaluator(OptimizingTree *root, const std::vector<const Variable *> &var_order, const RootNamespace &rns)
: funcs(NULL), start(NULL), ctx(NULL)
{
	assert(root);
	
	// Collect all nodes (a.k.a. subtrees or root)
	set<PCOT> variables, parameters, constants, functions;
	collect(root, constants, variables, parameters, functions);
	size_t no = root->num_children(), nv = variables.size(), np = parameters.size();
	
	// Assign every node a place in the EvalContext to write their result/value
	// The EvalContext will be ordered like this:
	//    [ vars | parameters | output | constants | intermediates ]
	
	map<PCOT, long> result_index; // for constants, vars, params, this will be the place for their value

	long idx = (long)nv; // skip vars for now
	for (auto x : variables)  result_index[x] = -1;
	
	// Parameters need no particular order because they will be set only once in every drawing cycle,
	// which means they're almost constant (assuming that we draw a lot of points in every cycle).
	for (auto x : parameters) result_index[x] = idx++;
	
	// Assign output, taking care that every one will actually get written, even if it just a
	// copy of a constant or variable, or merged (as in OptimizingTree) with another output
	std::vector<std::pair<long, long>> copiers;
	for (size_t i = 0; i < no; ++i)
	{
		OptimizingTree *c = root->child((int)i);
		if (result_index.count(c) > 0)
		{
			// it's merged with something else, so create a copier node
			copiers.emplace_back(idx, result_index[c]);
			OptimizingTree *cc = OptimizingTree::copier(c, rns);
			root->set_child((int)i, cc);
			c = cc;
			functions.insert(cc);
		}
		result_index[c] = idx++;
	}
	assert(idx == (long)(np + nv + no));
	
	// Assign constants, again in no particular order
	for (auto x : constants) if (!result_index.count(x)) result_index[x] = idx++;
	size_t nc = idx - (np + nv + no);
	
	// Intermediates (the outputs are already in result_index)
	for (auto x : functions) if (!result_index.count(x)) result_index[x] = idx++;
	
	// We know all sizes to initialize the template context
	ctx = new EvalContext(int(nv + np), (int)no, (int)nc, int(idx - nv - np - no - nc));
	for (auto x : constants) ctx->stack[result_index[x]] = *x->value;

	// initialize the copier output in case they only copy constants, which might be skipped later
	for (auto &c : copiers) ctx->stack[c.first] = ctx->stack[c.second];
	
	// Build var order for all used vars.
	// The closer to the front var is in the order, the more we will try to move the dependant functions to the back,
	// which means that start[var] can be larger, which is what we want for the frequent changers
	
	idx = 0; // start with the best place and fill in the var_order we were passed
	for (auto v : var_order)
	{
		for (auto x : variables)
		{
			if ((const Variable*)x->variable != v) continue;
			if (result_index[x] < 0) result_index[x] = idx++;
			break;
		}
	}
	
	// If we have vars that were not in the var_order, sort them by decreasing amount of dependents.
	// This is some kind of optimism-sort since skipping an unchanged var with lots of dependents should
	// be better than skipping one with few dependents.
	if (idx < (long)nv)
	{
		std::vector<PCOT> remains;
		map<PCOT, size_t> usage;
		for (auto x : variables)
		{
			if (result_index[x] >= 0) continue;
			usage[x] = get_usage(root, x);
			remains.push_back(x);
		}
		
		std::sort(remains.begin(), remains.end(), [&](PCOT a, PCOT b){ return usage[a] > usage[b]; });
		for (auto x : remains) result_index[x] = idx++;
	}
	assert(idx == (long)nv);
	
	// Save var_indexes so users of the Evaluator know what goes where
	for (auto x : variables)  var_indexes[x->variable] = (int)result_index[x];
	for (auto x : parameters) var_indexes[x->variable] = (int)result_index[x];
	
	// have the tree figure out what is real and complex so we can convert to the optimal variant of functions
	root->update_real();
	
	// Now we can finally flatten the tree.
	// We need to take care of these things:
	// 1 - functions have to be evaluated before any dependant functions
	// 2 - stick to var order as much as possible
	// 3 - keep track of the first function that needs to be evaluated when some var changes

	size_t nf = functions.size();
	funcs = new ExecToken* [nf+1];
	funcs[nf] = NULL; // terminator

	// start[i+1] is the index of the first ExecToken that must be recalculated when var_i and possibly var_i-1,
	// var_i-2, ... var_0 have changed.
	// start[0] is where calculation must restart when nothing has changed (this can be less than nf if there are
	// nondeterministic functions in the mix).
	start = new int[nv + np + 1];
	for(size_t i = 0; i <= nv + np; ++i) start[i] = (int)nf; // set all to "do nothing no matter which var changed"

	// Fill a pool with all Nodes we can convert. New nodes get added to the pool
	// when all their children have been converted.
	POOL pool(result_index);
	pool.initialize(root);
	pool.sort();
	
	size_t i; // current funcs index
	
	for (i = 0; !pool.set.empty(); ++i)
	{
		//pool.print();
		
		// convert the next node
		assert(i < nf);
		PCOT node = pool.list.front();
		funcs[i] = ExecToken::convert(node, result_index);
		
		// and remove it from the pool
		pool.list.pop_front();
		pool.set.erase(node);
		pool.done.insert(node);
		pool.update(root);
		
		// update the start array:
		// When any var that funcs[i] depends on is changed, funcs[i] must be
		// recalculated, so we must have start[var+1] <= i.
		// But then we must also set start[var'+1] <= i for all var' > var because their
		// start value also contains var possibly having changed.
		// So we can just do this for the minimum var index that funcs[i] depends on
		long firstdep = (long)(nv + np); // the minimum var index
		if (!node->deterministic)
		{
			firstdep = -1; // update from start[0]
		}
		else for(PCOT d : pool.deps[node])
		{
			long var = result_index[d];
			assert(var >= 0 && var < (long)(nv + np));
			if(var < 0 || var >= (long)(nv + np)) continue;
			if(var < firstdep) firstdep = var;
		}
		
		for (long v = firstdep; v < (long)(nv + np); ++v)
		{
			if(start[v+1] > (int)i)
			{
				start[v+1] = (int)i;
			}
		}
	}
	
	assert(i == nf);
}

//----------------------------------------------------------------------------------------------------------------------
// Printing
//----------------------------------------------------------------------------------------------------------------------

void Evaluator::print(std::ostream &out, const Namespace *ns) const
{
	using std::vector;
	using std::string;
	
	if (!ctx)
	{
		out << "Uninitialized Evaluator";
		return;
	}
	
	const EvalContext &ec = *ctx;
	vector<string> names;
	string empty = "<NOT YET CALCULATED!>";
	for(int i = 0; i < ec.n_inputs (); ++i) names.push_back(format("X%d", i+1));
	for(int i = 0; i < ec.n_outputs(); ++i) names.push_back(format("Y%d", i+1));
	size_t n = ec.n_inputs()+ec.n_outputs();
	for(int i = 0; i < ec.n_constants(); ++i) names.push_back(to_string(ec.stack[n+i]));
	for(int i = 0; i < ec.n_internals(); ++i) names.push_back(empty);
	for(int i = 0; i < 32; ++i) names.push_back("OVERFLOW!");
	
	out << "Order of vars: ";
	{
		std::vector<const Element *> print_order(ec.n_inputs(), NULL);
		for (auto i : var_indexes){ assert(!print_order[i.second]); print_order[i.second] = i.first; }
		
		bool first = true; size_t i = 0;
		for (const Element *x : print_order)
		{
			if (!first) out << ", "; first = false;
			assert(x);
			out << names[i++] << " = " << (x ? x->name() : std::string("<NULL>")); // X1 = radius, X2 = x, ...
		}
		out << std::endl;
	}
	
	out << "Start indices: ";
	bool first = true;
	for (long i = -1; i < (long)ec.n_inputs(); ++i)
	{
		if (!first) out << ", "; first = false;
		out << '#' << start[i+1];
	}
	out << std::endl;
	
	out << "Calculation: " << std::endl;
	
	int nlen = 0;
	for (n = 0; funcs[n]; ++n) ; // count number of ExecTokens
	do { ++nlen; n /= 10; } while (n); // digits needed
	size_t intermediate = 0;
	for(size_t i = 0; funcs[i]; ++i)
	{
		const ExecToken &t = *funcs[i];
		long res = t.result_index;
		if (names[res]==empty) names[res] = format("t%d", ++intermediate);
		out << format("#%0*d: %*s = ", nlen, i, nlen+1, names[res].c_str());
		
		int narg = 0; while(narg < 4 && t.param_index[narg] >= 0) ++narg;
		Function *func = ns->find(t.function(), narg);

		if (func && func->isOperator() && narg == 2)
		{
			out << names[t.param_index[0]];
			out << ' ' << func->name() << ' ';
			out << names[t.param_index[1]];
		}
		else
		{
			out << (func ? func->name() : "<no symbol>") << (narg != 1 ? '(' : ' ');
			for (int j = 0; j < narg; ++j)
			{
				if (j > 0) out << ", ";
				long pos = t.param_index[j];
				out << names[pos];
			}
			if(narg != 1) out << ')';
		}
		out << std::endl;
	}
	out << std::endl;
}
