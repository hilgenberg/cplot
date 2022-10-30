#include "ThreadMap.h"

#include <cassert>
#include <iostream>
#include <atomic>

#include <thread>
#include <mutex>
#include <condition_variable>
#include <sched.h>

#ifdef DEBUG
//#define TASK_DEBUG
#endif

//----------------------------------------------------------------------------------------------------------------------
// WorkUnit
//----------------------------------------------------------------------------------------------------------------------

bool WorkUnit::assign()
{
	if (state != State::TODO) return false;
	state = State::ASSIGNED;
	return true;
}

void WorkUnit::finish()
{
	assert(state == State::ASSIGNED);
	state = State::DONE;
	layer->finish(this);
}

void WorkUnit::start(int i)
{
	//std::condition_variable foo;
	
	// check the items in range_below
	if (layer->range_below != 0)
	{
		WorkLayer *last = layer->below;
		int lastn = (int)last->units.size();
		for (;;)
		{
			if (layer->range_below < 0)
			{
				if (last->done()) break;
			}
			else
			{
				bool wait = false;
				for (int j = 0; j < layer->range_below; ++j)
				{
					int k = layer->offset + i + j;
					if (k >= 0 && k < lastn && !last->units[k]->done())
					{
						#ifdef TASK_DEBUG
						layer->task->start_logging();
						std::cerr << "Unit " << i << " / " << layer->work_order(i) << " of layer " << layer->name <<
						" waiting on unit " << k << " / " << last->work_order(k) << " below (" << last->name << ")" << std::endl;
						layer->task->finish_logging();
						#endif
						
						wait = true;
						break;
					}
				}
				if (!wait) break;
			}
			

			//usleep(500 + arc4random_uniform(200)); // waiting for the lower level could take some time
			#ifdef USE_PTHREADS
			sched_yield();
			#else
			std::this_thread::yield();
			#endif
		}
	}
	
	// check that all items before (in the layer's work order) this one that intersect its space range are done
	if (layer->space > 0)
	{
		int iw = layer->work_order(i);
		for (;;)
		{
			bool wait = false;
			
			for (int j = -layer->space; j <= layer->space; ++j)
			{
				if (j == 0) continue;
				int u = i+j; if (layer->cyclic && u > 0) u %= layer->units.size();
				int k = layer->work_order(u);
				assert(k != iw);
				if (k >= 0 && k < iw && !layer->units[u]->done())
				{
					#ifdef TASK_DEBUG
					layer->task->start_logging();
					std::cerr << "Unit " << i << " / " << layer->work_order(i) << " of layer " << layer->name <<
					" waiting on neighbour " << u << " / " << k << std::endl;
					layer->task->finish_logging();
					#endif
					
					wait = true;
					break;
				}
			}
			if (!wait) break;
			

			#ifdef USE_PTHREADS
			sched_yield();
			#else
			std::this_thread::yield();
			std::this_thread::sleep_for(std::chrono::nanoseconds(100));
			//usleep(100 + arc4random_uniform(100)); // waiting for neighbours should almost never happen
			#endif
		}
	}
}

//----------------------------------------------------------------------------------------------------------------------
// WorkLayer
//----------------------------------------------------------------------------------------------------------------------

WorkLayer::WorkLayer(const std::string &name, Task *t, WorkLayer *down, int space_, int range_below_, int offset_)
: name(name), task(t), below(down), above(NULL), space(space_), range_below(range_below_), offset(offset_)
, next_todo(0), unfinished(0), cyclic(false)
{
	assert(range_below == 0 || below != NULL);
	if (space < 0) space = 0;
	
	if (below) below->above = this;
		
	if (!task->layer0)
	{
		task->layer0 = this;
		task->active = this;
	}
}

WorkLayer::~WorkLayer()
{
	for (WorkUnit *u : units) delete u;
}

//----------------------------------------------------------------------------------------------------------------------

void WorkLayer::finish(WorkUnit *u)
{
	if (!acquire(true)){ assert(false); return; }
	assert(u->state == WorkUnit::State::DONE);
	--unfinished; // atomic
	//OSAtomicDecrement32Barrier(&unfinished);
	release();
}

WorkUnit *WorkLayer::get(int &i)
{
	int n = (int)units.size();
	if (next_todo >= n) return NULL;
	if (!acquire(true)) return NULL;
	
	if (next_todo >= n)
	{
		release();
		return NULL;
	}

	i = index_order(next_todo);
	WorkUnit *u = units[i];
	if (!u->assign())
	{
		assert(false);
		release();
		return NULL;
	}
	++next_todo;
	release();
	return u;
}

//----------------------------------------------------------------------------------------------------------------------
// Task
//----------------------------------------------------------------------------------------------------------------------

WorkUnit *Task::get(int &i)
{
	WorkLayer *a = active;
	while (a)
	{
		WorkUnit *u = a->get(i);
		if (u) return u;
		
		// active layer is done, move up
		// threadsafe variant of if (a == active) active = a->above
		
		//OSAtomicCompareAndSwapPtr((void*)a, (void*)a->above, (void**)&active);
		if (std::atomic_compare_exchange_weak(&active, &a, a->above)) a = a->above;
		// otherwise acxw loads a = active
	}
	return NULL;
}

void *Task::run_thread(void *task_)
{
	Task *task = (Task*)task_;
	void *data;
	task->setup(task->info, data);
	try
	{
		WorkUnit *u;
		int       i;
		while (u = task->get(i))
		{
			u->start(i);
			u->work(data);
			u->finish();
			#ifdef USE_PTHREADS
			sched_yield();
			#else
			std::this_thread::yield();
			#endif
		}
	}
	catch(...)
	{
		// if threads start throwing exceptions, we should set some failure bits and cancel everybody
		assert(false);
	}
	task->finish(data);
	return NULL;
}

void Task::run(int n_threads)
{
	if (n_threads <= 1)
	{
		run_thread(this);
		return;
	}

#ifndef USE_PTHREADS
	
	std::vector<std::thread> threads;
	for (int i = 0; i < n_threads; ++i)
	{
		threads.emplace_back(Task::run_thread, this);
	}

	if (threads.empty())
	{
		assert(false);
		run_thread(this);
		return;
	}

	for (auto &t : threads)
	{
		t.join();
	}

#else

	std::vector<pthread_t> threads;
	for (int i = 0; i < n_threads; ++i)
	{
		pthread_t ID;
		if (0 == pthread_create(&ID, NULL, Task::run_thread, this))
		{
			threads.push_back(ID);
		}
	}
	
	if (threads.empty())
	{
		assert(false);
		run_thread(this);
		return;
	}
	
	for (auto ID : threads)
	{
		pthread_join(ID, NULL);
	}
	
#endif
}

