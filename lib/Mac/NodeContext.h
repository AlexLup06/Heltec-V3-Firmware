#pragma once

#include "LoggerManager.h"
#include "LoRaDisplay.h"

class NodeContext
{
public:
    LoggerManager *loggerManager = nullptr;
    LoRaDisplay *loraDisplay = nullptr;
    uint8_t nodeId;
};