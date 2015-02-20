#pragma once

#include <functional>
#include <list>

namespace util
{
    // Replacement for Qt signal/slot mechanism
    template<class... Args>
    class Signal
    {
    public:
        typedef std::function<void(Args...)> Slot;
        typedef typename std::list<Slot>::iterator SlotHandle;

        SlotHandle operator+=(const Slot &slot)
        {
            _slots.push_back(slot);
            return --_slots.end();
        }

        void operator-=(SlotHandle handle) {
            _slots.erase(handle);
        }

        void operator()(Args... args)
        {
            for(Slot &c: _slots)
                c(args...);
        }

        size_t receivers() {
            return _slots.size();
        }

    private:
        std::list<Slot> _slots;
    };
}
