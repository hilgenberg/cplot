#include "ExecToken.h"
#include "OptimizingTree.h"
#include "../../Utility/StringFormatting.h"

#include <ostream>

namespace CP_PARSER
{
	
	ExecToken *ExecToken::convert(const OptimizingTree *node, const std::map<const OptimizingTree *, long> &result_index)
	{
		const BaseFunction *f = node->function;
		assert(node->type == OptimizingTree::OT_Function && f != NULL);

		// get node's result index
		auto i = result_index.find(node);
		assert(i != result_index.end()); if(i == result_index.end()) return NULL;
		long ret = i->second;
		
		// parameter indexes are result indexes of node's children
		long P[4];
		int nc = node->num_children();
		assert(nc <= 4); if (nc > 4) return NULL;
		for (int k = 0; k < nc; ++k)
		{
			i = result_index.find(node->child(k));
			assert(i != result_index.end()); if(i == result_index.end()) return NULL;
			P[k] = i->second;
		}

		switch(nc)
		{
			case 0:
				if (f->vfr) return new ExecToken_0R(f->vfr, ret);
				if (f->vfc) return new ExecToken_0C(f->vfc, ret);
				break;

			case 1:
				if (node->has_real_children())
				{
					if (f->ufrr) return new ExecToken_1RR(f->ufrr, ret, P[0]);
					if (f->ufrc) return new ExecToken_1RC(f->ufrc, ret, P[0]);
				}
				if (f->ufcr) return new ExecToken_1CR(f->ufcr, ret, P[0]);
				if (f->ufcc) return new ExecToken_1CC(f->ufcc, ret, P[0]);
				break;
				
			case 2:
				if (node->has_real_children())
				{
					if (f->bfrr) return new ExecToken_2RR(f->bfrr, ret, P[0], P[1]);
					if (f->bfrc) return new ExecToken_2RC(f->bfrc, ret, P[0], P[1]);
				}
				if (f->bfcr) return new ExecToken_2CR(f->bfcr, ret, P[0], P[1]);
				if (f->bfcc) return new ExecToken_2CC(f->bfcc, ret, P[0], P[1]);
				break;
				
			case 3:
				if (node->has_real_children())
				{
					if (f->tfrr) return new ExecToken_3RR(f->tfrr, ret, P[0], P[1], P[2]);
				}
				if (f->tfcc) return new ExecToken_3CC(f->tfcc, ret, P[0], P[1], P[2]);
				break;
				
			case 4: if (f->qfcc) return new ExecToken_4CC(f->qfcc, ret, P[0], P[1], P[2], P[3]); break;
		}
		return NULL;
	}
	
}
