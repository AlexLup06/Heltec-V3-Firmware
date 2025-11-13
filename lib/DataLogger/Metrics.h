#pragma once
#include <Arduino.h>

enum class Metric : uint8_t
{
    ReceivedCompleteMission_V, // ✅
    SentMissionRTS_V,          // ✅
    SentMissionFragment_V,     // ✅
    TimeToLastTrajectory_V,    // ✅
    ReceivedEffectiveBytes_V,  // ✅
    ReceivedBytes_V,           // ✅
    SentEffectiveBytes_V,      // ✅
    SentBytes_V,               // ✅
    Collisions_S,              // ✅
    TimeOnAir_S,               // ✅
    SingleValues,
    COUNT
};

inline const char *metricToString(Metric m)
{
    switch (m)
    {
    case Metric::ReceivedCompleteMission_V:
        return "received_complete_mission";
    case Metric::SentMissionRTS_V:
        return "sent_mission_rts";
    case Metric::SentMissionFragment_V:
        return "sent_mission_fragment";
    case Metric::TimeToLastTrajectory_V:
        return "time_of_last_trajectory";
    case Metric::ReceivedEffectiveBytes_V:
        return "received_effective_bytes";
    case Metric::ReceivedBytes_V:
        return "received_bytes";
    case Metric::SentEffectiveBytes_V:
        return "sent_effective_bytes";
    case Metric::SentBytes_V:
        return "sent_bytes";
    case Metric::TimeOnAir_S:
        return "time_on_air";
    case Metric::Collisions_S:
        return "collisions";
    case Metric::SingleValues:
        return "single_values";
    case Metric::COUNT:
        return "count";
    default:
        return "unknown";
    }
}

#pragma pack(push, 1)
struct SentEffectiveBytes_data
{
    uint16_t bytes;
};

struct SentBytes_data
{
    uint16_t bytes;
};

struct ReceivedEffectiveBytes_data
{
    uint16_t bytes;
};

struct ReceivedBytes_data
{
    uint16_t bytes;
};

struct ReceivedCompleteMission_data
{
    uint32_t time;
    uint8_t source;
    uint16_t missionId;
};

struct SentMissionRTS_data
{
    uint32_t time;
    uint8_t source;
    uint16_t missionId;
};

struct SentMissionFragment_data
{
    uint32_t time;
    uint8_t source;
    uint16_t missionId;
};

struct TimeToLastTrajecotory
{
    uint16_t time; // hundedth of second: 600 => 6s ; 1 => 0.01
};
#pragma pack(pop)
