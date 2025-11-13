#pragma once

#include <cstdint>
#include <Arduino.h>
#include "messages.h"
#include "functions.h"
#include "config.h"

class MessageSimulator
{
private:
    uint32_t nextMission = -1;
    uint32_t nextNeighbour = -1;

    uint16_t timeToNextMission = 5000;
    uint16_t timeToNextNeighbour = 8000;

    bool initRun = true;

public:
    MessageSimulator();
    ~MessageSimulator();

    void init();
    void finish();

    void simulateMessages();
    void setTimeToNextMission(int _timeToNextMission);
    void cleanUp();

    bool packetReady = false;
    MessageToSend *messageToSend;
};