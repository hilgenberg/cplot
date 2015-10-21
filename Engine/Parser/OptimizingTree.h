#pragma once

#include "RetainTree.h"
#include "ParsingResult.h"
#include "../Namespace/RootNamespace.h"

#include <vector>
#include <string>
#include <map>
#include <unordered_map>
#include <set>
#include <cassert>

class WorkingTree;

/**
 * A simplified version of ParsingTree which can be optimized more easily.
 */
class OptimizingTree : public RetainTree<OptimizingTree>
{
public:
	typedef RetainTree<OptimizingTree> Super;
	
	explicit OptimizingTree(const WorkingTree *wt, ParsingResult &result);
	static OptimizingTree *copier(OptimizingTree *tree_to_copy, const RootNamespace &rns);
	~OptimizingTree(); // only ever delete the root!
	
	void optimize()
	{
		/// @todo support associativity and commutativity while merging
		/// @todo support associativity in calculate optimization
		
		update_deterministic();
		calculate();
		merge();
	}
	
	bool root() const{ return type == OT_Function && function == NULL; }
	bool has_real_children() const{ for (auto c : children) if (!c->real) return false; return true; }
	void update_real(); /// Figure out which nodes are real and which are complex.
	
	enum OTType
	{
		OT_Constant,
		OT_Function,
		OT_Variable
	};
	
	OTType type;
	
	bool deterministic;
	bool real;
	
	union
	{
		const cnum         *value;
		const BaseFunction *function;
		const Element      *variable; // Parameter or Variable
	};
	
	void set_child(int i, OptimizingTree *c)
	{
		Super::child(i, c);
	}
	
private:
	void update_deterministic();
	void calculate();
	void merge();
	
	OptimizingTree();
	
	bool equals(OptimizingTree *other);
	
	struct cnum_hash{ size_t operator()(const cnum &z) const
	{
		return std::hash<double>()(z.real()) ^ std::hash<double>()(z.imag()); }
	};
	void merge_const(std::unordered_map<cnum,OptimizingTree*,cnum_hash> &nums,
					 std::map<const Element *, OptimizingTree *> &vars);
	
	friend void merge_funcs(OptimizingTree *tree, std::set<OptimizingTree*> &visited);
};

std::ostream &operator<<(std::ostream &out, const OptimizingTree &tree);
