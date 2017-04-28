#pragma once

#include <vector>
#include <cassert>
#include <string>
#include <functional>
#include <atomic>
#include "ThreadInfo.h"
#include "../../Utility/Mutex.h"

class WorkLayer;
class Task;

/**
 * @defgroup ThreadMaps Thread Maps
 * @{
 */

// Task class handles the thread_data 
typedef std::function<void(void *thread_data)> Work;

/**
 * Part of a WorkLayer. Distinguished from its siblings in the same layer by the data in its work lambda.
 * If the WorkLayer plows through some loop then one WorkUnit is typically some part of the loop where
 * i1 <= i < i2 and the units are more or less independent and can often run in parallel.
 * 
 * The execution lifecycle is like this:
 * -# The unit starts in State::TODO
 * -# The layer calls assign and state switches to State::ASSIGNED
 * -# WorkUnit::start is called which waits until all units that this one depends on are WorkUnit::done()
 * -# WorkLayer::work runs for the unit, passing thread_data (cf. Task class) of the thread that runs it
 * -# WorkUnit::finish sets state to State::DONE
 */

class WorkUnit
{
	friend class WorkLayer;
	friend class Task;

private:
	WorkUnit() = delete;
	WorkUnit(const WorkUnit &) = delete;
	WorkUnit &operator= (const WorkUnit &) = delete;

	/// Units are created by the WorkLayer that contains them
	WorkUnit(WorkLayer *layer, const Work &w) : work(w), layer(layer), state(State::TODO) { }
	
	enum class State : int
	{
		TODO     =  0, ///< ready to go
		ASSIGNED =  1, ///< assigned, possibly running
		DONE     =  2  ///< done, but can be reactivated
	};

	volatile State    state;
	Work              work;  ///< Called to do the actual work
	WorkLayer * const layer; ///< The containing WorkLayer
	
	bool  assign();            ///< Set state to ASSIGNED
	void   start(int myIndex); ///< Wait for dependencies to finish
	void  finish();            ///< Set state to DONE and notify the layer
	bool    done() const{ return state == State::DONE; }
};


/**
 * Typically one major step of an algorithm. Divided into an array of work units that run in parallel.
 * Layers are stacked on top of each other. The entire stack is the Task.
 *
 * Work layers can handle three types of dependencies between work units:
 * -# This layer needs the entire layer below to finish before it can run any work units
 * -# Every work unit needs its pendant on the layer below (and possible the neighbours of that unit) to finish
 *    before it starts running.
 * -# No work unit can run in parallel with its direct neighbours (or an entire range of neighbours)
 *
 * The work that a work layer does is passed as a std::function which should contain all needed data as well.
 *
 * Work units have two natural orderings: On the one hand they are elements of an array - call that index_order.
 * On the other hand they can be sorted by the neighbour dependencies: 0, 1+space, 2+2space, ..., wrapping around.
 * Call that work_order. This distinction is important for the internal bookkeeping and WorkLayer has private
 * methods to convert between the two orderings.
 */

class WorkLayer
{
	friend class WorkUnit;
	friend class Task;
	WorkLayer() = delete;
	WorkLayer(const WorkLayer &) = delete;
	WorkLayer &operator= (const WorkLayer &) = delete;

public:
	
	/**
	 * @param name  Mainly for debugging
	 * @param t     The containing task
	 * @param down  The next lower layer or NULL if this will be the first layer
	 * @param space Unit i is blocked by units[i-space] ... units[i+space] (except for itself, of course)
	 *              It is guaranteed, that units[0], units[1+space], [2+2space], etc run first.
	 * @param range_below  Unit i is blocked by below->units[i+offset+0] ... [i+offset+range_below-1].
	 *                     If range_below < 0, this needs the entire layer below to finish first.
	 * @param offset
	 * @see cyclic
	 */
	WorkLayer(const std::string &name, Task *t, WorkLayer *down, int space = 0, int range_below = 0, int offset = 0);
	~WorkLayer();

	WorkUnit *add_unit(const Work &work)
	{
		WorkUnit *u = new WorkUnit(this, work);
		units.push_back(u);
		++unfinished;
		return u;
	}

	bool done() const{ return !unfinished; }

	void set_cyclic(){ cyclic = true; }

	/**
	 * Tries to get the next work unit assigned and waits for its dependencies to finish before returning.
	 * @param its_index On success will be the index (in units array) of the returned unit.
	 * @return The next work unit, in state State::ASSIGNED or NULL on failure (no unit is TODO).
	 */
	WorkUnit *get(int &its_index);
	
private:
	
	/**
	 * Locks the layer, so its state can be updated.
	 * @param block If true, the call will block until the lock is acquired, otherwise it returns immediately.
	 * @return True if the lock was acquired.
	 */
	bool acquire(bool block)
	{
		return (block ? (lock.lock(), true) : lock.try_lock());
	}
	
	/**
	 * Release the lock. Must only be called after a successful call to acquire.
	 */
	void release(){ lock.unlock(); }
	
	/**
	 * Call u->finish() while locked and decrement the 'unfinished' counter.
	 */
	void finish(WorkUnit *u);
	
	WorkLayer *above, *below;     ///< Doubly linked list.
	Task      *task;              ///< Task that this belongs to.
	Mutex      lock;              ///< Lock for all state updates.

	bool cyclic; ///< First and last units are considered neighbours if cyclic is true.
	
	volatile int32_t     next_todo;  ///< in work_order
	std::atomic<int32_t> unfinished; ///< Upper bound for the number of units with !done()
	std::vector<WorkUnit*> units;
	int space;                    ///< Every unit blocks the next and previous space units
	int range_below, offset;      ///< For getting blocked by the lower Layer
	std::string name;             ///< For printing/debugging

	
	/* Some work order examples:
	 
	 n = 4, space = 1
	 0 . 1 . --> wrap around
	 0 2 1 3

	 
	 n = 5, space = 2
	 0 . . 1 . --> wrap around
	 0 2 . 1 3 --> wrap around
	 0 2 4 1 4
	 
	 */

	int index_order(int i) const ///  work oder to index order
	{
		int n = (int)units.size();
		if (i < 0 || i >= n) return -1;
		
		for (int k = 0; k <= space; ++k)
		{
			if (k + i*(space+1) < n) return k + i*(space+1);
			i -= (n-k+space)/(space+1);
		}
		assert(false);
		return -1;
	}
	int work_order(int j) const /// index order to work order
	{
		int n = (int)units.size();
		if (j < 0 || j >= n) return -1;
		
		int k = j % (space+1);
		int i = j / (space+1);
		for (int l = 0; l < k; ++l)
		{
			i += (n-l+space)/(space+1);
		}
		assert(index_order(i) == j);
		return i;
	}
};


/**
 * Tasks consist of a lattice of work units, grouped into layers, that are run by a thread pool.
 */

class Task
{
	friend class WorkLayer;
	
public:
	/**
	 * Defines the thread setup procedure. Typically setup will create some per-thread-structure (memory pools for 
	 * instance) and store a pointer to it in data. The finish call will delete the structure. info will typically
	 * be a struct with settings for the calculations in the task and every thread stores a copy if it.<BR>
	 * Data (by now per-thread-data + copy of info) will then get passed to every layer's work function as the
	 * 'thread_data' parameter.
	 * 
	 * @param setup Will be called by every thread with the info and a data pointer of its own (initially NULL).
	 * @param finish Before the threads terminate, they call finish with the same data pointer.
	 * @param info Something that gets passed to every thread's setup call.
	 */
	Task(void *info,
		 void (*setup )(const void *info, void *&data) = ThreadInfo::thread_setup,
		 void (*finish)(void *data) = ThreadInfo::thread_finish)
	: active(NULL), layer0(NULL), setup(setup), finish(finish), info(info)
	{
	}
	
	~Task()
	{
		WorkLayer *w = layer0;
		while (w)
		{
			WorkLayer *tmp = w;
			w = w->above;
			delete tmp;
		}
	}
	
	void run(int n_threads); ///< Creates worker threads and runs the entire task.
	
private:
	WorkUnit *get(int &its_index); ///< @return The next work unit in State::TODO or NULL if the task is done.
	
	std::atomic<WorkLayer*> active; ///< Lowest layer with work units to be run.
	WorkLayer *layer0;           ///< Lowest layer.
	
	void (*setup)(const void *info, void *&data); ///< Called for every thread, before it starts
	void (*finish)(void *data); ///< Called for every thread when it's done
	const void *info; ///< As passed to constructor.

	bool acquire(WorkLayer *layer) /// Tries to lock the task while layer is still the active layer. Does not block.
	{
		if (active != layer) return false;
		if (!lock.try_lock()) return false;
		if (active != layer)
		{
			lock.unlock();
			return false;
		}
		return true;
	}
	void release() /// Release the lock.
	{
		lock.unlock();
	}

	static void *run_thread(void *task); ///< Called by every thread.

	Mutex lock; ///< Lock for modifying the task's state
	#ifdef DEBUG
	Mutex logger_lock; ///< Lock for writing to stdout or stderr or logfiles
public:
	void  start_logging(){ logger_lock.lock(); }
	void finish_logging(){ logger_lock.unlock(); }
	#endif
};

/** @} */

