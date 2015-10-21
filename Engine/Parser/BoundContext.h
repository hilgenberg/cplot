#pragma once

#include "Evaluator.h"
#include "EvalContext.h"
#include "FPTR.h"

/**
 * This combines an Evaluator and an EvalContext into a single faster entity.
 */

class BoundContext
{
public:
	BoundContext(const Evaluator &e);

	~BoundContext()
	{
		delete [] funcs;
		delete [] start;
		delete stack;
	}
	
	BoundContext(const BoundContext &) = delete;
	BoundContext &operator= (const BoundContext &) = delete;
	
	inline void eval() const
	{
		using CP_PARSER::ExecToken;
		for (FCall *F = funcs + start[last_change+1]; F->function; ++F)
		{
			#define RZ (*F->result)
			#define RR assert(RZ.imag() == 0.0); (*(double*)F->result)
			#define R(i) (*(double*)F->param[i])
			#define Z(i) (*F->param[i])
			#define f(T) ((T*)F->function)
			#define CHK1 assert(Z(0).imag() == 0.0)
			#define CHK2 assert(Z(0).imag() == 0.0 && Z(1).imag() == 0.0)
			#define CHK3 CHK2; assert(Z(2).imag() == 0.0)
			
			switch (F->type)
			{
				case ExecToken::Exec_1RR: CHK1; RR = f(ufuncRR)(R(0));    break;
				case ExecToken::Exec_1CC:            f(ufunc)  (Z(0), RZ); break;
				case ExecToken::Exec_1CR:       RR = f(ufuncCR)(Z(0));    break;
				case ExecToken::Exec_1RC: CHK1;      f(ufuncRC)(R(0), RZ); break;
				case ExecToken::Exec_2RR: CHK2; RR = f(bfuncRR)(R(0), R(1));    break;
				case ExecToken::Exec_2RC: CHK2;      f(bfuncRC)(R(0), R(1), RZ); break;
				case ExecToken::Exec_0R:        RR = f(vfuncR) ();  break;
				case ExecToken::Exec_0C:             f(vfunc)  (RZ); break;
				case ExecToken::Exec_2CC:            f(bfunc)  (Z(0), Z(1), RZ); break;
				case ExecToken::Exec_2CR:       RR = f(bfuncCR)(Z(0), Z(1));    break;
				case ExecToken::Exec_3RR: CHK3; RR = f(tfuncRR)(R(0), R(1), R(2));    break;
				case ExecToken::Exec_3CC:            f(tfunc)  (Z(0), Z(1), Z(2), RZ); break;
				case ExecToken::Exec_4CC:            f(qfunc)  (Z(0), Z(1), Z(2), Z(3), RZ); break;
			}
			
			#undef RZ
			#undef RR
			#undef R
			#undef Z
			#undef f
			#undef CHK1
			#undef CHK2
		}
		last_change = -1;
	}

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
	
	inline int n_inputs   () const{ return nin; }
	inline int n_outputs  () const{ return nout; }
	
	void print(std::ostream &o, const Namespace *ns) const;
	
private:
	//--- Evaluator equivalents ----------------------------------------------------------------------------------------

	struct FCall
	{
		cnum       *result;
		const cnum *param[4];
		FPTR        function;
		CP_PARSER::ExecToken::Type type;
	};
	FCall        *funcs; // terminated by an FCall with function == NULL
	int          *start; // from Evaluator
	
	//--- EvalContext equivalents --------------------------------------------------------------------------------------
	
	mutable cnum *stack;
	mutable int   last_change;
	int           nin, nout;
	
	char padding[64-3*sizeof(void*)-3*sizeof(int)];

};
