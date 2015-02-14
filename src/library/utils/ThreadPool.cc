#include "ThreadPool.h"

using namespace std;

ThreadPool::ThreadPool(unsigned max_threads)
    : _max_threads(max_threads)
    , _waiting_threads(0)
    , _destructing(false)
    , _joining(false)
{
}

ThreadPool::~ThreadPool()
{
    join();

    {
        unique_lock<mutex> lock(_mutex);

        _destructing = true;
        _cv_tasks.notify_all(); // wake up threads
    }

    for(unsigned i = 0; i < _threads.size(); i++)
    {
        _threads[i]->wait_exit();
    }
}

void ThreadPool::join()
{
    // Help clear the queue
    while(true)
    {
        Task task;
        bool have_task = false;
        {
            unique_lock<mutex> lock(_mutex);
            if(_tasks.size() > 0)
            {
                task = _tasks.front();
                _tasks.pop_front();
                have_task = true;
            }
            else
            {
                if(_waiting_threads < _threads.size()) {
                    _joining = true;
                }
            }
        }

        if(have_task)
        {
            task();
        }
        else
        {
            break;
        }
    }

    if(_joining)
    {
        unique_lock<mutex> lock(_mutex);

        _cv_join.wait(lock, [=]() {
                return (_waiting_threads == _threads.size()) && (_tasks.size() == 0);
            });

        _joining = false;
    }
}

void ThreadPool::schedule(Task task)
{
    {
        unique_lock<mutex> lock(_mutex);

        _tasks.emplace_back(task);

        if(_waiting_threads == 0)
        {
            if(_threads.size() < _max_threads)
            {
                // Spawn a new thread.
                _threads.emplace_back(unique_ptr<Thread>(new Thread(*this)));
            }
        }
        else
        {
            _cv_tasks.notify_one();
        }
    }
}

ThreadPool::Thread::Thread(ThreadPool &pool)
    : _pool(pool)
    , _systhread([this](){ run(); })
{
}

void ThreadPool::Thread::wait_exit()
{
    _systhread.join();
}

bool ThreadPool::Thread::wait_for_task(Task &task)
{
    {
        unique_lock<mutex> lock(_pool._mutex);

        while(true)
        {
            if(_pool._tasks.size() > 0)
            {
                task = _pool._tasks.front();
                _pool._tasks.pop_front();
                return true;
            }
            else if(_pool._destructing)
            {
                return false;
            }
            else
            {
                ++_pool._waiting_threads;

                if( _pool._joining && (_pool._waiting_threads == _pool._threads.size()) )
                {
                    _pool._cv_join.notify_one();
                }

                _pool._cv_tasks.wait( lock, [=]() {
                        return _pool._destructing || (_pool._tasks.size() > 0);
                    } );

                --_pool._waiting_threads;
            }
        }
    }
}

void ThreadPool::Thread::run()
{
    Task task;
    while( wait_for_task(task) )
    {
        task();
    }
}
