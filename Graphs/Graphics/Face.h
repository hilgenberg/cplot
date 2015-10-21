#pragma once
#include "Edge.h"

//----------------------------------------------------------------------------------------------------------------------
// Face
//----------------------------------------------------------------------------------------------------------------------

struct Face
{
	POOL_ITEM(Face);

	Edge *e;
	bool  forward;  // direction on the initial edge

	Face(Edge *e, bool forward) : e(e), forward(forward)
	{
		assert(e);
		assert(forward ? e->lb : e->la);
	}
};
