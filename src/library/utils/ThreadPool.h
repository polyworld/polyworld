#pragma once

#include <condition_variable>
#include <deque>
#include <functional>
#include <memory>
#include <mutex>
#include <thread>
#include <vector>

class ThreadPool
{
public:
    typedef std::function<void()> Task;

    ThreadPool(unsigned max_threads);
    ~ThreadPool();

    void schedule(Task task);
    void join();

private:
    unsigned _max_threads;
    unsigned _waiting_threads;
    bool _destructing;
    bool _joining;

    class Thread
    {
    public:
        Thread(ThreadPool &pool);

        void wait_exit();

    private:
        void run();
        bool wait_for_task(Task &task);

        ThreadPool &_pool;
        std::thread _systhread;
    };

    std::deque<Task> _tasks;
    std::vector<std::unique_ptr<Thread>> _threads;
    std::mutex _mutex;
    std::condition_variable _cv_tasks;
    std::condition_variable _cv_join;
};
