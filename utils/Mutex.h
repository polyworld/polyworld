#pragma once

#include <pthread.h>

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
