#pragma once

#include <stdint.h>

class MovieRecorder
{
public:
    virtual ~MovieRecorder() {}

    virtual void recordFrame(uint32_t timestep) = 0;
};
