#pragma once

#include "EvalContext.h"
#include "ExecToken.h"
#include "../Namespace/RootNamespace.h"

#include <vector>
#include <set>

class Variable;
class Parameter;
class BoundContext;

/**
 * For evaluating an expression a large number of times, possibly in parallel in several threads.
 * When evaluating only once, there is no need to go beyond ParsingTree. But this is much more optimized.
 */

class Evaluator
{
public:
	/**
	 * Create an Evaluator for an OptimizingTree.
	 * This is the second-to-last step in the parsing ladder.
	 * @param ot Contains the expression to evaluate.
	 * @param var_order Should be sorted from fastest to slowest changer, so we can optimize for the case that
	 * only those vars that change most frequently have changed while the rest remained constant.
	 * For something like this:
	 * @code
	 * for (y = ...){
	 *     context.set_input(yindex, y);
	 *     for (x = ...){
	 *         context.set_input(xindex, x);
	 *         evaluator.eval(context);
	 *         ...
	 *     }
	 * }
	 * @endcode
	 * var_order must be [x, y].
	 *
	 * @param rns The root namespace ot was created with.
	 */
	 
	Evaluator(OptimizingTree *ot, const std::vector<const Variable *> &var_order, const RootNamespace &rns);
	~Evaluator();
	
	Evaluator(const Evaluator &) = delete;
	
	/**
	 * The main method.
	 * @param ec Contains the values of all referenced variables and parameters, and will receive the results.
	 */
	inline void eval(EvalContext &ec) const
	{
		CP_PARSER::ExecToken **F = funcs + start[ec.get_last_change()+1];
		ec.start_eval();
		while(*F) (*(F++))->eval(ec);
	}
	
	/// Number of output slots / complex image space dimension.
	int image_dimension() const{ return ctx ? ctx->n_outputs() : 0; }
	
	/// @return Copy of the template context. Every thread should have its own copy.
	EvalContext context() const{ return EvalContext(*ctx); }
	
	/// @return -1 if variable/parameter is not used, otherwise its value should go to context.stack[varIndex(var)]
	inline int var_index(const Element *var) const
	{
		auto i = var_indexes.find(var);
		return i == var_indexes.end() ? -1 : i->second;
	}

	/// Pass the current parameter values into the template context
	void set_parameters(const std::set<Parameter*> &params);
	
	void print(std::ostream &o, const Namespace *ns) const;

private:
	CP_PARSER::ExecToken         **funcs; /// NULL-terminated
	int                           *start; /// Maps EvalContext::lastChanged to the first func that needs evaluation
	EvalContext                   *ctx;   /// template context, has all constants and space for all intermediate results
	std::map<const Element *, int> var_indexes; /// @see var_index
	
	friend class BoundContext;
};

