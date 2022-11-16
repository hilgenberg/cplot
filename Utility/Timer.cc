#include "Timer.h"
#include <chrono>
#include <thread>
#include <time.h>
#include <cmath>
#include <unistd.h>
#include <sys/time.h>
#include <sys/param.h>

double now()
{
#if 1
	auto now = std::chrono::high_resolution_clock::now();
	auto duration = now.time_since_epoch();
	using sec = std::chrono::duration<double, std::ratio<1,1>>;
	return std::chrono::duration_cast<sec>(duration).count();
#else
	timeval t;
	gettimeofday(&t, NULL);
	return t.tv_sec + t.tv_usec * 1e-6;
#endif
}

void sleep(double t)
{
	if (t <= 0.0) return;
	double s; t = modf(t, &s);
	timespec ts;
	ts.tv_sec  = (time_t)s;
	ts.tv_nsec = (long)(t * 1e9);
	nanosleep (&ts, NULL);
}
