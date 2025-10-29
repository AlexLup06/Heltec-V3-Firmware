#pragma once

#include <cstdint>
#include <Arduino.h>
#include "messages.h"

class MessageSimulator
{
private:
    uint32_t nextMission = -1;
    uint32_t nextNeighbour = -1;

    uint16_t timeToNextMission;
    uint16_t timeToNextNeighbour;

public:
    MessageSimulator();
    ~MessageSimulator();

    void simulateMessages();

    bool packetReady = false;
    MessageToSend_t *messageToSend;
};