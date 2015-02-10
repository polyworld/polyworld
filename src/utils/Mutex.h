#pragma once

#include <pthread.h>
#include <unistd.h>

#if ((_POSIX_SPIN_LOCKS - 200112L) >= 0L)
	#define SPINLOCK_PTHREAD
#elif __APPLE__
	#define SPINLOCK_APPLE

	#include <libkern/OSAtomic.h>
#else
	#define SPINLOCK SPINLOCK_NONE
#endif

class IMutex
{
 public:
	virtual ~IMutex() {}

	virtual void lock() = 0;
	virtual void unlock() = 0;
};

class IMonitor : public IMutex
{
 public:
	virtual ~IMonitor() {}

	virtual void notify() = 0;
	virtual void notifyAll() = 0;
	virtual void wait() = 0;
};

class WaitMutex : public IMutex
{
 public:
	WaitMutex();
	virtual ~WaitMutex();

	virtual void lock();
	virtual void unlock();

 protected:
	pthread_mutex_t mutex;
};

#if defined(SPINLOCK_PTHREAD)
class SpinMutex : public IMutex
{
 public:
	SpinMutex();
	virtual ~SpinMutex();

	virtual void lock();
	virtual void unlock();

 private:
	pthread_spinlock_t spinlock;
};
#elif defined(SPINLOCK_APPLE)
class SpinMutex : public IMutex
{
 public:
	SpinMutex();
	virtual ~SpinMutex();

	virtual void lock();
	virtual void unlock();

 private:
	OSSpinLock spinlock;
};
#else
typedef WaitMutex SpinMutex;
#endif

class ConditionMonitor : public IMonitor, public WaitMutex
{
 public:
	ConditionMonitor();
	virtual ~ConditionMonitor();

	virtual void notify();
	virtual void notifyAll();
	virtual void wait();

 private:
	pthread_cond_t cond;
};

class MutexGuard
{
 public:
	MutexGuard( IMutex *mutex );
	MutexGuard( IMutex &mutex );
	~MutexGuard();

 private:
	void init( IMutex *mutex );

	IMutex *mutex;
};

#define MUTEX(MTX, STMT) { MutexGuard __mutexGuard(MTX); STMT; }
