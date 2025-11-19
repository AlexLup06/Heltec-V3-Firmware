#pragma once
#include <Arduino.h>
#include "Metrics.h"

#pragma pack(push, 1)
struct FileHeader
{
    uint32_t magic = 0xDEADBEEF;
    uint16_t version = 1;
    uint16_t entrySize;
    uint8_t metric;
    char networkName[32];
    char currentMac[32];
    uint16_t missionMessagesPerMin;
    uint8_t numberOfNodes;
    uint16_t runNumber;
    uint32_t timestamp;
    uint8_t nodeId;
};
#pragma pack(pop)
