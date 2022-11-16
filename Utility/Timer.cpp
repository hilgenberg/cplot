#include "Timer.h"
#include <chrono>
#include <thread>
#include <time.h>
#include <cmath>

double now()
{
	auto now = std::chrono::high_resolution_clock::now();
	auto duration = now.time_since_epoch();
	using sec = std::chrono::duration<double, std::ratio<1, 1>>;
	return std::chrono::duration_cast<sec>(duration).count();
}

void sleep(double t)
{
	if (t <= 0.0) return;
	Sleep((DWORD)(t*1000.0));
}

static void CALLBACK TimerCallback(void *timer_, BOOLEAN TimerOrWaitFired)
{
	assert(timer_);
	Timer &t = *((Timer*)timer_);
	if (t.callback) t.callback();
}

Timer::Timer(double dt) : dt_(dt), timer(0)
{
}

Timer::~Timer()
{
	stop();
}

void Timer::start()
{
	if (timer) return;
	if (!CreateTimerQueueTimer(&timer, NULL, ::TimerCallback, this,
		(DWORD)(dt_*1000.0),
		(DWORD)(dt_*1000.0),
		WT_EXECUTEINTIMERTHREAD))
	{
		assert(false);
		timer = 0;
	}
}

void Timer::stop()
{
	if (!timer) return;

	if (!DeleteTimerQueueTimer(NULL, timer, NULL))
	{
		if (GetLastError() != ERROR_IO_PENDING)
		{
			assert(false);
			return;
		}
	}
	timer = 0;
}

bool Timer::running() const
{
	return timer != 0;
}
