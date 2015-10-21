#pragma once

#include "../cnum.h"

#include <cassert>
#include <vector>
#include <map>
#include <string>

namespace CP_PARSER
{
	class ExecToken_0R;
	class ExecToken_0C;
	class ExecToken_1CC;
	class ExecToken_1RC;
	class ExecToken_1RR;
	class ExecToken_1CR;
	class ExecToken_2CC;
	class ExecToken_2RC;
	class ExecToken_2RR;
	class ExecToken_2CR;
	class ExecToken_3CC;
	class ExecToken_3RR;
	class ExecToken_4CC;
}

class BoundContext;

/**
 * EvalContext stores input, temporary and output values for evaluations of an expression via Evaluator.
 * It also keeps track of which inputs have changed since the last evaluation, so the Evaluator can
 * possibly skip some of the calculations.
 * There should be one EvalContext per thread, but the Evaluator can be shared.
 */

struct EvalContext
{
	/// Empty context
	EvalContext() : nin(0), nout(0), nc(0), size(0), last_change(-1), stack(NULL)
	{
	}

	/// Zeroed context with defined sizes
	EvalContext(int n_inputs, int n_outputs, int n_constants, int n_internals)
	: nin(n_inputs), nout(n_outputs), nc(n_constants),
	  size(n_inputs+n_outputs+n_constants+n_internals), last_change(n_inputs-1 /*mark all as changed*/) 
	{
		stack = new cnum[size];
		for(int i = 0; i < size; ++i) stack[i] = 0.0; // needed for real functions
	}
	
	/// Copy constructor
	EvalContext(const EvalContext &other)
	: nin(other.nin), nout(other.nout), nc(other.nc), size(other.size),
	  last_change(other.last_change)
	{
		stack = new cnum[size];
		for(int n = 0; n < size; ++n) stack[n] = other.stack[n];
	}
	
	/// Move constructor, avoids allocating and copying other.stack
	EvalContext(EvalContext &&other)
	: nin(other.nin), nout(other.nout), nc(other.nc), size(other.size),
	  last_change(other.last_change), stack(other.stack)
	{
		other.stack = NULL;
	}
	
	EvalContext & operator= (EvalContext&& other)
	{
		nin  = other.nin;
		nout = other.nout;
		nc   = other.nc;
		size = other.size;
		last_change = other.last_change;
		std::swap(stack, other.stack);
		return *this;
	}
	
	EvalContext & operator= (EvalContext& other) = delete;
	
	~EvalContext(){ delete[] stack; }
	
	inline void set_input(int i, const cnum &value)
	{
		assert(i >= 0 && i < nin);
		stack[i] = value;
		if(i > last_change) last_change = i;
	}
	inline void set_input(int i, double value)
	{
		assert(i >= 0 && i < nin);
		stack[i] = value;
		if(i > last_change) last_change = i;
	}
	
	inline const cnum & input(int i) const{ assert(i < nin);  return stack[i]; }
	inline const cnum &output(int i) const{ assert(i < nout); return stack[nin+i]; }
	
	inline int n_total    () const{ return size; }
	inline int n_inputs   () const{ return nin; }
	inline int n_constants() const{ return nc; }
	inline int n_outputs  () const{ return nout; }
	inline int n_internals() const{ return size - nout - nin - nc; }
	
	inline int  get_last_change() const{ return last_change; }
	inline void start_eval(){ last_change = -1; }
	
private:
	/// Laid out like this: [ vars | parameters | output | constants | intermediates ]<BR>
	/// Always use set_input to write to the stack - otherwise the change-flag goes inconsistent
	cnum *stack;
	
	int nin, nout, nc, size;
	int last_change; ///< Maximum index of some input that was changed since start_eval() or -1 if nothing has changed

	char padding[64-8-5*sizeof(int)];
	
	// the ExecTokens need write-access to the intermediates and outputs on the stack
	friend class CP_PARSER::ExecToken_0R;
	friend class CP_PARSER::ExecToken_0C;
	friend class CP_PARSER::ExecToken_1CC;
	friend class CP_PARSER::ExecToken_1RC;
	friend class CP_PARSER::ExecToken_1RR;
	friend class CP_PARSER::ExecToken_1CR;
	friend class CP_PARSER::ExecToken_2CC;
	friend class CP_PARSER::ExecToken_2RC;
	friend class CP_PARSER::ExecToken_2RR;
	friend class CP_PARSER::ExecToken_2CR;
	friend class CP_PARSER::ExecToken_3CC;
	friend class CP_PARSER::ExecToken_3RR;
	friend class CP_PARSER::ExecToken_4CC;
	
	// and so does Evaluator::Evaluator
	friend class Evaluator;
	friend class BoundContext;
};
