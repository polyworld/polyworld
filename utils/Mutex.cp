#include "Mutex.h"

#include <assert.h>

SpinMutex::SpinMutex()
{
	int rc = pthread_spin_init( &spinlock, PTHREAD_PROCESS_PRIVATE );
	assert( rc == 0 );
}

SpinMutex::~SpinMutex()
{
	pthread_spin_destroy( &spinlock );
}

void SpinMutex::lock()
{
	int rc = pthread_spin_lock( &spinlock );
	assert( rc == 0 );
}

void SpinMutex::unlock()
{
	int rc = pthread_spin_unlock( &spinlock );
	assert( rc == 0 );
}


WaitMutex::WaitMutex()
{
	int rc = pthread_mutex_init( &mutex, NULL );
	assert( rc == 0 );
}

WaitMutex::~WaitMutex()
{
	pthread_mutex_destroy( &mutex );
}

void WaitMutex::lock()
{
	int rc = pthread_mutex_lock( &mutex );
	assert( rc == 0 );
}

void WaitMutex::unlock()
{
	int rc = pthread_mutex_unlock( &mutex );
	assert( rc == 0 );
}

ConditionMonitor::ConditionMonitor()
: WaitMutex()
{
	int rc = pthread_cond_init( &cond, NULL );
	assert( rc == 0 );
}

ConditionMonitor::~ConditionMonitor()
{
	pthread_cond_destroy( &cond );
}

void ConditionMonitor::notify()
{
	int rc = pthread_cond_signal( &cond );
	assert( rc == 0 );
}

void ConditionMonitor::notifyAll()
{
	int rc = pthread_cond_broadcast( &cond );
	assert( rc == 0 );
}

void ConditionMonitor::wait()
{
	int rc = pthread_cond_wait( &cond, &mutex );
	assert( rc == 0 );
}

MutexGuard::MutexGuard( IMutex *mutex )
{
	init( mutex );
}

MutexGuard::MutexGuard( IMutex &mutex )
{
	init( &mutex );
}

MutexGuard::~MutexGuard()
{
	mutex->unlock();
}

void MutexGuard::init( IMutex *mutex )
{
	this->mutex = mutex;

	mutex->lock();
}
