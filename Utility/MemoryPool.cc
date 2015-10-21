#include "MemoryPool.h"
#include <cassert>
#include <stdexcept>

RawMemoryPool::RawMemoryPool(size_t chunkSize_, size_t align)
: chunkSize(chunkSize_), c0(NULL), cn(NULL), alignment(align)
{
	assert(!(align & (align-1))); // must be 2^k
	assert(chunkSize > 64);

#ifdef MP_THREADSAFE
	int err = pthread_mutex_init(&mutex, NULL);
	if(err != 0) throw std::runtime_error("Creating mutex failed");
#endif
}

void RawMemoryPool::clear()
{
	while (c0)
	{
		Chunk *c1 = c0->next;
		assert(c1 || c0 == cn);
		delete c0;
		c0 = c1;
	}
	cn = NULL;
}


char *RawMemoryPool::alloc(size_t size)
{
#ifdef MP_THREADSAFE
	Lock lock(mutex);
#endif
	size = align(size);
	
	if (!cn || cn->avail < size)
	{
		Chunk *c = new Chunk(*this, size <= chunkSize ? chunkSize : size);
		if (cn)
		{
			cn->next = c;
			cn = c;
		}
		else
		{
			c0 = cn = c;
		}
	}
	char *ret = cn->mem + cn->size - cn->avail;
	cn->avail -= size;
	assert(ret == align(ret));
	return ret;
}

//----------------------------------------------------------------------------------------------------------------------
//  RawMemoryPool::Chunk
//----------------------------------------------------------------------------------------------------------------------

RawMemoryPool::Chunk::Chunk(RawMemoryPool &pool, size_t n)
: size(n + pool.alignment),
  mem0(new char[n + pool.alignment]), next(NULL)
{
	mem = pool.align(mem0);
	size_t ds = mem - mem0; assert(ds < pool.alignment);
	size -= ds;
	avail = size;
}

