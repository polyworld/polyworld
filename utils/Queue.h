#pragma once

#include <assert.h>

#include "Mutex.h"

template <class T>
class IQueue
{
 public:
	virtual ~IQueue () {}

	// Place element at end of queue.
	virtual void post( T val ) = 0;
	// Get element from front of queue, if available. Blocks until
	// gets element or both empty and endOfPosts. Returns false if
	// nothing fetched.
	virtual bool fetch( T *val ) = 0;

	// Mark that no more posts will happen, allowing fetch to 
	// return immediately if empty.
	virtual void endOfPosts() = 0;
	// Clear out the elements and endOfPosts state.
	virtual void reset() = 0;
};

//
// Posts may be performed by any thread context.
//
template <class T>
class SerialQueue : public IQueue<T>
{
 public:
	SerialQueue( size_t capacity = 1024 * 16,
				 IMutex *mutex = new SpinMutex(),
				 bool deleteMutex = true )
	{
		this->capacity = capacity;
		this->mutex = mutex;
		this->deleteMutex = deleteMutex;

		q = new T[capacity];

		head = tail = 0;
		isPostingActive = true;
	}

	virtual ~SerialQueue()
	{
		delete [] q;
		if( deleteMutex )
			delete mutex;
	}

	virtual void post( T val )
	{
		assert( isPostingActive );
		assert( tail <= capacity );


		MUTEX( mutex, q[tail++] = val );
	}

	virtual bool fetch( T *val )
	{
		bool success = false;

		MUTEX(mutex,

			  if( head < tail )
			  {
				  *val = q[head];
				  success = true;
				  head++;
			  }

			  );

		return success;
	}

	virtual void endOfPosts()
	{
		isPostingActive = false;
	}

	virtual void reset()
	{
		assert( head == tail );

		head = tail = 0;

		isPostingActive = true;
	}

 private:
	size_t capacity;
	T *q;
	bool isPostingActive;

	IMutex *mutex;
	bool deleteMutex;

	size_t head;
	size_t tail;
};

// With this implementation the poster will never have to enter a mutex region
// and will therefore never block. That is its key advantage.
//
// Note that posts must always be performed by the master thread.
//
// Fetchers run full tilt until fetch() can return.
template <class T>
class BusyFetchQueue : public IQueue<T>
{
 public:
	BusyFetchQueue( size_t capacity = 1024 * 16,
					IMutex *mutex = new SpinMutex(),
					bool deleteMutex = true )
	{
		this->capacity = capacity;
		this->mutex = mutex;
		this->deleteMutex = deleteMutex;

		q = new T[capacity];

		head = tail = 0;
		isPostingActive = true;
	}

	virtual ~BusyFetchQueue()
	{
		delete [] q;
		if( deleteMutex )
		{
			delete mutex;
		}
	}

	virtual void post( T val )
	{
		assert( isPostingActive );
		assert( tail <= capacity );

		q[tail] = val;

#pragma omp atomic
		tail++;
	}

	virtual bool fetch( T *val )
	{
		bool success = false;

		do
		{
			MUTEX(mutex,
			  
				  if( head < tail )
				  {
					  *val = q[head];
					  success = true;
					  head++;
				  }

				  );
		} while( !success && isPostingActive );

		return success;
	}

	virtual void endOfPosts()
	{
		isPostingActive = false;
	}

	virtual void reset()
	{
		assert( head == tail );

		head = tail = 0;

		isPostingActive = true;
	}

 private:
	size_t capacity;
	T *q;
	bool isPostingActive;

	IMutex *mutex;
	bool deleteMutex;

	size_t head;
	size_t tail;
};

// With this implementation the poster must enter a mutex region and
// will notify the fetchers when data is available. The fetchers will
// sleep when they encounter an empty queue until the poster notifies
// them.
template <class T>
class WaitFetchQueue : public IQueue<T>
{
	WaitFetchQueue( int capacity = 1024 );
	virtual ~WaitFetchQueue();

	virtual void post( T val );
	virtual bool fetch( T *val );

	virtual void endOfPosts();
	virtual void reset();
};
