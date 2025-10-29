#pragma once
#include <Arduino.h>
#include "Metrics.h"

struct FileHeader
{
    uint32_t magic = 0xDEADBEEF;
    uint16_t version = 1;
    uint16_t entrySize;
    Metric metric;
    char topology[16];
    uint16_t runNumber;
    uint32_t timestamp;
};
