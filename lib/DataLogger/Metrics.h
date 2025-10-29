#pragma once
#include <Arduino.h>

enum class Metric : uint8_t
{
    ToA,
    Collisions,
    Delay,
    Energy,
    SingleValues,
    COUNT
};

// Convert enum to readable string (for filenames, printing, etc.)
inline const char *metricToString(Metric m)
{
    switch (m)
    {
    case Metric::ToA:
        return "toa";
    case Metric::Collisions:
        return "collisions";
    case Metric::Delay:
        return "delay";
    case Metric::Energy:
        return "energy";
    case Metric::SingleValues:
        return "singlevalues";
    default:
        return "unknown";
    }
}

struct ToAData
{
    uint32_t timestamp;
    float timeOnAir;
};
struct CollisionData
{
    uint32_t timestamp;
    uint8_t collided;
};
struct DelayData
{
    uint32_t timestamp;
    float delay;
};
struct EnergyData
{
    uint32_t timestamp;
    float mA;
};
