#include "MessageSimulator.h"

void MessageSimulator::init()
{
    uint32_t currentTime = millis();
    nextMission = currentTime + timeToNextMission;
    nextNeighbour = currentTime + timeToNextNeighbour;
}

void MessageSimulator::finish()
{
    packetReady = false;
    if (messageToSend != nullptr)
    {
        free(messageToSend->payload);
        free(messageToSend);
        messageToSend = nullptr;
    }
}

void MessageSimulator::simulateMessages()
{
    uint32_t currentTime = millis();

    if (initRun)
    {
        init();
        initRun = false;
    }

    if (currentTime > nextMission)
    {
        uint16_t size = random(50, 245);
        uint8_t *dummyPayload = (uint8_t *)malloc(size);

        for (int i = 0; i < size; i++)
        {
            dummyPayload[i] = random(0, 256);
        }

        auto msg = (MessageToSend_t *)malloc(sizeof(MessageToSend_t));
        msg->payload = dummyPayload;
        msg->size = size;
        msg->isMission = true;

        messageToSend = msg;
        packetReady = true;

        nextMission = currentTime + random(timeToNextMission - timeToNextMission / 10, timeToNextMission - timeToNextMission / 10);
        return;
    }

    if (currentTime > nextNeighbour)
    {
        uint16_t size = 144;
        uint8_t *dummyPayload = (uint8_t *)malloc(size);

        for (int i = 0; i < size; i++)
        {
            dummyPayload[i] = random(0, 256);
        }

        auto msg = (MessageToSend_t *)malloc(sizeof(MessageToSend_t));
        msg->payload = dummyPayload;
        msg->size = size;
        msg->isMission = false;

        messageToSend = msg;
        packetReady = true;

        nextNeighbour = currentTime + random(timeToNextNeighbour - timeToNextNeighbour / 10, timeToNextNeighbour - timeToNextNeighbour / 10);
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
