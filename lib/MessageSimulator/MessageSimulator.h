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

void MessageSimulator::simulateMessages()
{
    uint32_t currentTime = millis();
    if (currentTime < nextMission)
    {
        uint16_t size = random(50, 247);
        uint8_t *dummyPayload = (uint8_t *)malloc(size);

        for (int i = 0; i < size; i++)
        {
            // TODO: check if correct generation
            dummyPayload[i] = random(0, 256);
        }

        auto floodPacket = (MessageToSend_t *)malloc(sizeof(MessageToSend_t));
        floodPacket->payload = dummyPayload;
        floodPacket->size = size;
        floodPacket->isMission = true;

        messageToSend = floodPacket;
        packetReady = true;

        nextMission = currentTime + timeToNextMission;
        return;
    }

    if (currentTime < nextNeighbour)
    {
        uint16_t size = 144;
        uint8_t *dummyPayload = (uint8_t *)malloc(size);

        for (int i = 0; i < size; i++)
        {
            // TODO: check if correct generation
            dummyPayload[i] = random(0, 256);
        }

        auto floodPacket = (MessageToSend_t *)malloc(sizeof(MessageToSend_t));
        floodPacket->payload = dummyPayload;
        floodPacket->size = size;
        floodPacket->isMission = false;

        messageToSend = floodPacket;
        packetReady = true;

        nextMission = currentTime + timeToNextNeighbour;
        return;
    }
}

MessageSimulator::MessageSimulator() {}

MessageSimulator::~MessageSimulator() {}
