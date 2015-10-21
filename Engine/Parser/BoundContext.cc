#include "BoundContext.h"
#include <cstring>

BoundContext::BoundContext(const Evaluator &e)
: stack(new cnum[e.ctx->size]), last_change(e.ctx->last_change)
, nin(e.ctx->nin), nout(e.ctx->nout)
, start(new int[e.ctx->nin+1])
{
	memcpy(stack, e.ctx->stack, e.ctx->size*sizeof(cnum));
	memcpy(start, e.start, (nin+1)*sizeof(int));
	
	int nf = 0;
	for (CP_PARSER::ExecToken **F = e.funcs; *F; ++F) ++nf;

	#ifdef DEBUG
	for (int i = 0; i <= nin; ++i) assert(start[i] >= 0 && start[i] <= nf);
	#endif

	funcs = new FCall[nf+1];
	funcs[nf].function = NULL;
	
	assert(sizeof(cnum) == 2*sizeof(double));
	
	for (int i = 0; i < nf; ++i)
	{
		CP_PARSER::ExecToken &F = *e.funcs[i];
		FCall &f = funcs[i];
		
		assert(F.result_index >= nin && F.result_index < e.ctx->size);
		f.result   = stack + F.result_index;
		f.param[0] = F.param_index[0] < 0 ? NULL : stack + F.param_index[0]; assert(F.param_index[0] < e.ctx->size);
		f.param[1] = F.param_index[1] < 0 ? NULL : stack + F.param_index[1]; assert(F.param_index[1] < e.ctx->size);
		f.param[2] = F.param_index[2] < 0 ? NULL : stack + F.param_index[2]; assert(F.param_index[2] < e.ctx->size);
		f.param[3] = F.param_index[3] < 0 ? NULL : stack + F.param_index[3]; assert(F.param_index[3] < e.ctx->size);
		f.function = F.function();
		f.type     = F.type();
		assert(f.function);
		
		#define RR   const_cast<cnum*>(f.result)  ->imag(0.0); assert(f.result  ->imag() == 0.0)
		#define R(i) const_cast<cnum*>(f.param[i])->imag(0.0); assert(f.param[i]->imag() == 0.0)
		using CP_PARSER::ExecToken;
		switch (f.type)
		{
			case ExecToken::Exec_2RC: R(1); // fallthrough
			case ExecToken::Exec_1RC: R(0); break;
			case ExecToken::Exec_3RR: R(2); // fallthrough
			case ExecToken::Exec_2RR: R(1); // fallthrough
			case ExecToken::Exec_1RR: R(0); // fallthrough
			case ExecToken::Exec_0R:
			case ExecToken::Exec_1CR:
			case ExecToken::Exec_2CR: RR; break;
			default: break;
		}
		#undef RR
		#undef R
	}
}
