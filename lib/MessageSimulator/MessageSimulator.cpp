#include "MessageSimulator.h"

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
