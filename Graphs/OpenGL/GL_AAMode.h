#pragma once

enum AntialiasMode
{
	AA_Off    = 0,
	AA_Lines  = 1,
	AA_4x     = 2,
	AA_8x     = 3,
	AA_4x_Acc = 4,
	AA_8x_Acc = 5
};

inline AntialiasMode translate(AntialiasMode m, bool accum_ok)
{
	if (!accum_ok)
	{
		if (m == AA_4x_Acc) return AA_4x;
		if (m == AA_8x_Acc) return AA_8x;
	}
	return m;
}

