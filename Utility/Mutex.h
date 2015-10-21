#pragma once

#include <pthread.h>

class Mutex
{
public:
	Mutex (){ pthread_mutex_init(&mutex, NULL); }
	~Mutex(){ pthread_mutex_destroy(&mutex); }

	Mutex(const Mutex &)            = delete;
	Mutex &operator=(const Mutex &) = delete;
	
private:
	friend class Lock;
	pthread_mutex_t mutex;
};


class Lock
{
public:
	Lock(Mutex &m) : mutex(m)
	{
		pthread_mutex_lock(&mutex.mutex);
	}
	
	~Lock()
	{
		pthread_mutex_unlock(&mutex.mutex);
	}
	
	Lock(const Lock &)            = delete;
	Lock &operator=(const Lock &) = delete;
	
private:
	Mutex &mutex;
};

