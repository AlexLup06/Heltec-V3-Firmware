#pragma once
#include <Arduino.h>

enum class Metric : uint8_t
{
    // needed for node reachibility
    SentMissionRTS_V,
    ReceivedCompleteMission_V,
    // needed for reception success ratio
    ReceivedMissionIdFragment_V,
    ReceivedNeighbourIdFragment_V,
    // byte data
    ReceivedEffectiveBytes_S,
    ReceivedBytes_S,
    SentEffectiveBytes_S,
    SentBytes_S,
    // other scalars
    Collisions_S,
    TimeOnAir_S,
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
    case Metric::ReceivedEffectiveBytes_S:
        return "received_effective_bytes";
    case Metric::ReceivedBytes_S:
        return "received_bytes";
    case Metric::SentEffectiveBytes_S:
        return "sent_effective_bytes";
    case Metric::SentBytes_S:
        return "sent_bytes";
    case Metric::ReceivedMissionIdFragment_V:
        return "received_mission_id_fragment";
    case Metric::ReceivedNeighbourIdFragment_V:
        return "received_neighbour_id_fragment";
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
struct SentMissionRTS_data
{
    uint32_t time;
    uint8_t source;
    uint16_t missionId;
};

struct ReceivedCompleteMission_data
{
    uint32_t time;
    uint8_t source;
    uint16_t missionId;
};

struct ReceivedMissionIdFragment_data
{
    uint16_t missionId;
    uint8_t hopId;
    uint8_t sourceId;
};

struct ReceivedNeighbourIdFragment_data
{
    uint16_t missionId;
    uint8_t sourceId;
};

#pragma pack(pop)
