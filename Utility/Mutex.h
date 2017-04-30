#pragma once

#ifdef USE_PTHREADS
#include <pthread.h>

class Mutex
{
public:
	Mutex (){ pthread_mutex_init(&mutex, NULL); }
	~Mutex(){ pthread_mutex_destroy(&mutex); }

	Mutex(const Mutex &)            = delete;
	Mutex &operator=(const Mutex &) = delete;

	void lock() { pthread_mutex_lock(&mutex); }
	void unlock() { pthread_mutex_unlock(&mutex); }
	bool try_lock() { return 0 == pthread_mutex_trylock(&mutex); }
	
private:
	friend class Lock;
	pthread_mutex_t mutex;
};

#else

#include <mutex>

class Mutex
{
public:
	Mutex() { }
	~Mutex() { }

	Mutex(const Mutex &) = delete;
	Mutex &operator=(const Mutex &) = delete;

	void lock() { mutex.lock(); }
	void unlock() { mutex.unlock(); }
	bool try_lock() { return mutex.try_lock(); }

private:
	std::mutex mutex;
};

#endif

class Lock
{
public:
	Lock(Mutex &m) : mutex(m)
	{
		mutex.lock();
	}

	~Lock()
	{
		mutex.unlock();
	}

	Lock(const Lock &)            = delete;
	Lock &operator=(const Lock &) = delete;

private:
	Mutex &mutex;
};

