#pragma once

#include "EvalContext.h"
#include "../Namespace/BaseFunction.h"
#include "FPTR.h"

#include <map>

class Evaluator;
class BoundContext;
class OptimizingTree;

namespace CP_PARSER
{
	/**
	 * ExecToken stores a function pointer and indexes into EvalContext's stack where to read the function's
	 * parameters and where to store the calculated return value.
	 * Has subclasses for our various function types (arity, real/complex).<BR>
	 * The subclasses are named ExecToken_[n params][from: R|C|nothing][to: R|C]
	 */

	class ExecToken
	{
	public:
		static ExecToken *convert(const OptimizingTree *ot, const std::map<const OptimizingTree *, long> &result_index);
		
		explicit ExecToken(long res, long p0=-1, long p1=-1, long p2=-1, long p3=-1)
		: result_index(res)
		{
			param_index[0] = p0;
			param_index[1] = p1;
			param_index[2] = p2;
			param_index[3] = p3;
		}
		virtual ~ExecToken(){ }
		
		virtual void eval(EvalContext &ctx) const = 0;
		
		long maxIndex() const
		{
			long ret = result_index;
			if(param_index[0] > ret) ret = param_index[0];
			if(param_index[1] > ret) ret = param_index[1];
			if(param_index[2] > ret) ret = param_index[2];
			if(param_index[3] > ret) ret = param_index[3];
			return ret;
		}
		
		enum Type
		{
			Exec_1RR, Exec_1CC, Exec_1CR, Exec_1RC,
			Exec_2RR, Exec_2RC,
			Exec_0R,  Exec_0C,
			Exec_2CC,
			Exec_2CR,
			Exec_3RR, Exec_3CC,
			Exec_4CC
		};
		
		virtual Type type() const = 0;

	protected:
		long param_index[4];
		long result_index;

		virtual FPTR function() const = 0; // returns the function pointer

		friend class ::Evaluator;    // its print(...) method
		friend class ::BoundContext; // its constructor
	};
	
	#define P0 ctx.stack[param_index[0]]
	#define P1 ctx.stack[param_index[1]]
	#define P2 ctx.stack[param_index[2]]
	#define P3 ctx.stack[param_index[3]]
	#define R  ctx.stack[result_index]

	//------------------------------------------------------------------------------------------------------------------
	// No arguments
	//------------------------------------------------------------------------------------------------------------------
	
	class ExecToken_0R : public ExecToken
	{
	public:
		ExecToken_0R(vfuncR *F, long res) : ExecToken(res), f(F){ }
		void eval(EvalContext &ctx)const{ R = f(); }
	protected:
		vfuncR *f;
		virtual FPTR function() const{ return (FPTR)f; }
		virtual Type type() const{ return Exec_0R; }
	};
		
	class ExecToken_0C : public ExecToken
	{
	public:
		ExecToken_0C(vfunc *F, long res) : ExecToken(res), f(F){ }
		void eval(EvalContext &ctx)const{ f(R); }
	protected:
		vfunc *f;
		virtual FPTR function() const{ return (FPTR)f; }
		virtual Type type() const{ return Exec_0C; }
	};
	
	//------------------------------------------------------------------------------------------------------------------
	// One argument
	//------------------------------------------------------------------------------------------------------------------

	class ExecToken_1CC : public ExecToken
	{
	public:
		ExecToken_1CC (ufunc *F, long res, long p0) : ExecToken(res,p0), f(F){ }
		void eval(EvalContext &ctx)const{ f(P0, R); }
	protected:
		ufunc *f;
		virtual FPTR function() const{ return (FPTR)f; }
		virtual Type type() const{ return Exec_1CC; }
	};
	
	//------------------------------------------------------------------------------------------------------------------
	
	class ExecToken_1RC : public ExecToken
	{
	public:
		ExecToken_1RC(ufuncRC *F, long res, long p0) : ExecToken(res,p0), f(F){ }
		void eval(EvalContext &ctx)const{ f(P0.real(), R); }
	protected:
		ufuncRC *f;
		virtual FPTR function() const{ return (FPTR)f; }
		virtual Type type() const{ return Exec_1RC; }
	};
	
	//------------------------------------------------------------------------------------------------------------------
	
	class ExecToken_1RR : public ExecToken
	{
	public:
		ExecToken_1RR(ufuncRR *F, long res, long p0) : ExecToken(res,p0), f(F){ }
		void eval(EvalContext &ctx)const{ R = f(P0.real()); }
	protected:
		ufuncRR *f;
		virtual FPTR function() const{ return (FPTR)f; }
		virtual Type type() const{ return Exec_1RR; }
	};
	
	//------------------------------------------------------------------------------------------------------------------
	
	class ExecToken_1CR : public ExecToken
	{
	public:
		ExecToken_1CR(ufuncCR *F, long res, long p0) : ExecToken(res,p0), f(F){ }
		void eval(EvalContext &ctx)const{ R = f(P0); }
	protected:
		ufuncCR *f;
		virtual FPTR function() const{ return (FPTR)f; }
		virtual Type type() const{ return Exec_1CR; }
	};
	
	//------------------------------------------------------------------------------------------------------------------
	// Two arguments
	//------------------------------------------------------------------------------------------------------------------
	
	class ExecToken_2CC : public ExecToken
	{
	public:
		ExecToken_2CC(bfunc *F, long res, long p0, long p1) : ExecToken(res,p0,p1), f(F){ }
		void eval(EvalContext &ctx)const{ f(P0, P1, R); }
	protected:
		bfunc *f;
		virtual FPTR function() const{ return (FPTR)f; }
		virtual Type type() const{ return Exec_2CC; }
	};
	
	//------------------------------------------------------------------------------------------------------------------
	
	class ExecToken_2RC : public ExecToken{
	public:
		ExecToken_2RC(bfuncRC *F, long res, long p0, long p1) : ExecToken(res,p0,p1), f(F){ }
		void eval(EvalContext &ctx)const{ f(P0.real(), P1.real(), R); }
	protected:
		bfuncRC *f;
		virtual FPTR function() const{ return (FPTR)f; }
		virtual Type type() const{ return Exec_2RC; }
	};
	
	//------------------------------------------------------------------------------------------------------------------
	
	class ExecToken_2RR : public ExecToken{
	public:
		ExecToken_2RR(bfuncRR *F, long res, long p0, long p1) : ExecToken(res,p0,p1), f(F){ }
		void eval(EvalContext &ctx)const{ R = f(P0.real(), P1.real()); }
	protected:
		bfuncRR *f;
		virtual FPTR function() const{ return (FPTR)f; }
		virtual Type type() const{ return Exec_2RR; }
	};
	
	//------------------------------------------------------------------------------------------------------------------
	
	class ExecToken_2CR : public ExecToken
	{
	public:
		ExecToken_2CR(bfuncCR *F, long res, long p0, long p1) : ExecToken(res,p0,p1), f(F){ }
		void eval(EvalContext &ctx)const{ R = f(P0, P1); }
	protected:
		bfuncCR *f;
		virtual FPTR function() const{ return (FPTR)f; }
		virtual Type type() const{ return Exec_2CR; }
	};
	
	//------------------------------------------------------------------------------------------------------------------
	// Three and four arguments
	//------------------------------------------------------------------------------------------------------------------
	
	class ExecToken_3CC : public ExecToken
	{
	public:
		ExecToken_3CC(tfunc *F, long res, long p0, long p1, long p2) : ExecToken(res,p0,p1,p2), f(F){ }
		void eval(EvalContext &ctx)const{ f(P0, P1, P2, R); }
	protected:
		tfunc *f;
		virtual FPTR function() const{ return (FPTR)f; }
		virtual Type type() const{ return Exec_3CC; }
	};
	
	//------------------------------------------------------------------------------------------------------------------
	
	class ExecToken_3RR : public ExecToken{
	public:
		ExecToken_3RR(tfuncRR *F, long res, long p0, long p1, long p2) : ExecToken(res,p0,p1,p2), f(F){ }
		void eval(EvalContext &ctx)const{ R = f(P0.real(), P1.real(), P2.real()); }
	protected:
		tfuncRR *f;
		virtual FPTR function() const{ return (FPTR)f; }
		virtual Type type() const{ return Exec_3RR; }
	};
	
	//------------------------------------------------------------------------------------------------------------------
	
	class ExecToken_4CC : public ExecToken
	{
	public:
		ExecToken_4CC(qfunc *F, long res, long p0, long p1, long p2, long p3) : ExecToken(res,p0,p1,p2,p3), f(F){ }
		void eval(EvalContext &ctx)const{ f(P0, P1, P2, P3, R); }
	protected:
		qfunc *f;
		virtual FPTR function() const{ return (FPTR)f; }
		virtual Type type() const{ return Exec_4CC; }
	};

	#undef P0
	#undef P1
	#undef P2
	#undef P3
	#undef R
}
