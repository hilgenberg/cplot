#pragma once
#include <cstddef>
#include <stdexcept>
#include <cassert>

/**
 * @defgroup MemoryPool Memory Pools
 * @{
 */

// If any MemoryPool is shared between threads, MP_THREADSAFE must be defined
// to add a mutex to every pool. For now we don't use that because it's too slow
// and every thread gets its own MemoryPools anyway.

// #define MP_THREADSAFE


/**
 * POOL_ITEM(<CLASS>) should be added to the declaration of every class that can live in a memory pool.
 */

#define POOL_ITEM_BASE(CLASSNAME) \
void *operator new(size_t s, RawMemoryPool &p){ return p.alloc(s); }\
void *operator new(size_t s, MemoryPool<CLASSNAME> &p){ return p.alloc(s); }\
void *operator new(size_t, void *p){ return p; }\
void  operator delete(void *, size_t){ }\

#define POOL_ITEM(CLASSNAME) \
POOL_ITEM_BASE(CLASSNAME) \
CLASSNAME(const CLASSNAME &) = delete;\
CLASSNAME &operator=(const CLASSNAME &) = delete


#ifdef MP_THREADSAFE
#include <pthread.h>
#endif

template<typename T> class PoolIterator;

/**
 * For allocating large amounts of small objects more quickly.
 * This works by allocating memory in larger chunks and dispensing it in smaller
 * pieces. ChunkSize/sizeof(contained Objects) is the allocation compression factor.
 * Every MemoryPool has an alignment (some power of 2) and all memory it dispenses
 * will be aligned like that (f.i. 64-byte aligned so two objects don't share cache
 * lines).
 * Usage is like this:
 *
 * @code
 * class Foo{ public: POOL_ITEM(Foo); ... };
 * RawMemoryPool pool;
 * Foo *foo = new(pool) Foo(<args>);
 * Bar *bar = new(pool.alloc(sizof(Bar)) Bar(<args>);
 * @endcode
 *
 * The underlying memory is freed on pool.clear(); or when the pool goes out of scope
 * but it will not call any destructors.
 */

class RawMemoryPool
{
public:
	explicit RawMemoryPool(size_t chunkSize = 1024*16, size_t alignment = 8);
	~RawMemoryPool(){ clear(); }

	RawMemoryPool(const RawMemoryPool &) = delete;
	RawMemoryPool &operator=(const RawMemoryPool &) = delete;
	
	char *alloc(size_t size); ///< Dispense align_size(size) bytes
	void  clear();            ///< Empty the entire pool (calls no destructors)

	inline size_t align(size_t x) const{ return ((x+alignment-1) & ~(alignment-1)); }
	inline char *align(const char *x) const
	{
		return reinterpret_cast<char*>(align(reinterpret_cast<size_t>(x)));
	}

protected:
#ifdef MP_THREADSAFE
	pthread_mutex_t mutex;
	struct Lock
	{
		Lock(pthread_mutex_t &mutex) : mutex(mutex)
		{
			int error = pthread_mutex_lock(&lock);
			if(error) throw std::runtime_error("Could not aquire mutex");
		}
		~Lock()
		{
			int error = pthread_mutex_unlock(&lock);
			if(error) throw std::runtime_error("Could not release mutex");
		}
		pthread_mutex_t &mutex;
	};
#endif

	friend class Chunk;
	class Chunk // one Chunk of memory
	{
		friend class RawMemoryPool;
		template <typename T> friend class PoolIterator;
		template <typename T> friend class MemoryPool;
		
		Chunk(RawMemoryPool &pool, size_t size);
		~Chunk(){ delete [] mem0; }

		Chunk(const Chunk &) = delete;
		Chunk &operator=(const Chunk &) = delete;

		char * const mem0;  ///< Allocated memory
		char *       mem;   ///< Aligned memory
		size_t size, avail; ///< Total usable and free size in bytes
		Chunk *next;        ///< Chunks in a pool form a singly linked list
	};
	
	Chunk       *c0, *cn; ///< First and last chunks
	const size_t chunkSize, alignment; // As set in c'tor
	
	template <typename T> friend class PoolIterator;
};


template <typename T> class MemoryPool;

/**
 * For iterating a typed memory pool, one object at a time.
 */

template<typename T> class PoolIterator
{
private:
	friend class MemoryPool<T>;

	PoolIterator(const MemoryPool<T> &pool) : stride(pool.align(sizeof(T))), c(pool.c0), j(0), done(false)
	{
		update(); // this is needed for when the first chunk in the pool is smaller than stride
	}
	PoolIterator() : stride(0), c(NULL), j(0), done(true){ } // an end() iterator

public:
	PoolIterator(const PoolIterator<T> &I) = default;
	
	inline PoolIterator & operator++ (/*prefix*/)
	{
		j += stride;
		update();
		return *this;
	}
	inline T & operator* ()
	{
		if (done){ assert(false); throw std::logic_error("Iterator expired"); }
		return *(T*)(c->mem + j);
	}
	inline operator bool() const{ return !done; }
	inline bool operator != (const PoolIterator<T> &I) const
	{
		return done != I.done || !done && (c != I.c || j != I.j);
	}

private:
	const size_t stride;           ///< Size of one T + alignment
	const RawMemoryPool::Chunk *c; ///< Current chunk
	size_t j;                      ///< Offset in current chunk
	bool   done;
	
	inline void update() /// Ensures that the iterator never points beyond the end of a chunk.
	{
		if (done) return;
		while (c && j+stride > c->size - c->avail)
		{
			c = c->next;
			j = 0;
		}
		if (!c) done = true;
	}
};


/**
 * Subclass of RawMemoryPool that contains only objects of a single class T.
 *
 * Usage is similar to RawMemoryPool:
 *
 * @code
 * MemoryPool<Foo> pool;
 * Foo *foo = new(pool) Foo(<args>);
 * @endcode
 *
 * Unlike RawMemoryPools, MemoryPools can be iterated:
 * @code
 * for (const Foo &foo : pool) something;
 * @endcode
 */

template <typename T> class MemoryPool : private RawMemoryPool
{
public:
	friend class PoolIterator<T>;
	
	explicit MemoryPool(size_t chunkSize = 1024*sizeof(T), size_t alignment = 8)
	: RawMemoryPool(chunkSize, alignment){ }
	
	void  clear(){ RawMemoryPool::clear(); }
	char *alloc(size_t n){ assert(n % sizeof(T) == 0); return RawMemoryPool::alloc(n); }

	size_t n_items() const
	{
		size_t N = 0, stride = align(sizeof(T));
		for (const RawMemoryPool::Chunk *c = c0; c; c = c->next)
		{
			N += (c->size - c->avail) / stride;
		}
		return N;
	}
	
	PoolIterator<T> begin() const{ return PoolIterator<T>(*this); }
	PoolIterator<T>   end() const{ return PoolIterator<T>(); }
};

/** @} */


