#pragma once

#include <functional>
#include <vector>

namespace util
{
    // Replacement for Qt signal/slot mechanism
    template<class... Args>
    class Signal
    {
    public:
        typedef std::function<void(Args...)> Callback;

        void operator+=(const Callback &callback)
        {
            this->callbacks.push_back(callback);
        }

        void operator()(Args... args)
        {
            for(Callback &c: callbacks)
                c(args...);
        }

    private:
        std::vector<Callback> callbacks;
    };
}
