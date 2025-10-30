#include "MessageSimulator.h"

void MessageSimulator::simulateMessages()
{
    uint32_t currentTime = millis();

    if (isFirstTimeRunning)
    {
        nextMission = currentTime + timeToNextMission;
        nextNeighbour = currentTime + timeToNextNeighbour;
        isFirstTimeRunning = false;
    }

    if (currentTime > nextMission)
    {
        uint16_t size = random(50, 247);
        uint8_t *dummyPayload = (uint8_t *)malloc(size);

        for (int i = 0; i < size; i++)
        {
            // TODO: check if correct generation
            dummyPayload[i] = random(0, 256);
        }

        auto msg = (MessageToSend_t *)malloc(sizeof(MessageToSend_t));
        msg->payload = dummyPayload;
        msg->size = size;
        msg->isMission = true;

        messageToSend = msg;
        packetReady = true;

        nextMission = currentTime + timeToNextMission;
        return;
    }

    if (currentTime > nextNeighbour)
    {
        uint16_t size = 144;
        uint8_t *dummyPayload = (uint8_t *)malloc(size);

        for (int i = 0; i < size; i++)
        {
            // TODO: check if correct generation
            dummyPayload[i] = random(0, 256);
        }

        auto msg = (MessageToSend_t *)malloc(sizeof(MessageToSend_t));
        msg->payload = dummyPayload;
        msg->size = size;
        msg->isMission = false;

        messageToSend = msg;
        packetReady = true;

        nextNeighbour = currentTime + timeToNextNeighbour;
        return;
    }
}
void MessageSimulator::cleanUp()
{
    packetReady = false;

    if (messageToSend)
    {
        free(messageToSend->payload);
        free(messageToSend);
        messageToSend = nullptr;
    }
}

MessageSimulator::MessageSimulator() {}

MessageSimulator::~MessageSimulator() {}
