#pragma once
#include "Timer.h"

class FPSCounter
{
public:
	void frame()
	{
		double t = now();
		if (last > 0.0)
		{
			double dt = t - last; assert(dt >= 0.0);
			if (n >= N) n = N-1;

			// Keep a running average. Because vsync only kicks in after the second draw()
			// call, weigh the first frame less strongly than times after the framerate had
			// a chance to stabilize (normally one would use f = n/(n+1) or some such - our
			// f goes from 0 to N-1 / N, so it ends up at the same value)
			double f = (double)n / (double)((N-n)*N);
			avg = f*avg + (1.0-f)*dt;
			
			++n;
		}
		last = t;
	}
	void pause() { last = -1.0; avg = -1.0; n = 0; }

	double fps() const { return avg > 0.0 ? 1.0 / avg : 0.0; }

private:
	double last = -1.0;
	double avg  = -1.0;
	int    n    = 0;
	static constexpr int N = 10;
};