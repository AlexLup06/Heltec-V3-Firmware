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

void MessageSimulator::setTimeToNextMission(uint16_t _timeToNextMission)
{
    timeToNextMission = _timeToNextMission;
}

void MessageSimulator::simulateMessages()
{
    uint32_t currentTime = millis();

    // Do not allocate a new message while one is still waiting to be consumed
    if (packetReady)
    {
        return;
    }

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

        MessageToSend *msg = (MessageToSend *)malloc(sizeof(MessageToSend));
        msg->payload = dummyPayload;
        msg->size = size;
        msg->isMission = true;

        messageToSend = msg;
        packetReady = true;

        nextMission = currentTime + random(timeToNextMission - timeToNextMission / 20, timeToNextMission - timeToNextMission / 20);
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

        MessageToSend * msg = (MessageToSend *)malloc(sizeof(MessageToSend));
        msg->payload = dummyPayload;
        msg->size = size;
        msg->isMission = false;

        messageToSend = msg;
        packetReady = true;

        nextNeighbour = currentTime + random(timeToNextNeighbour - timeToNextNeighbour / 20, timeToNextNeighbour - timeToNextNeighbour / 20);
        return;
    }
}
void MessageSimulator::cleanUp()
{
    packetReady = false;

    if (messageToSend != nullptr)
    {
        free(messageToSend->payload);
        free(messageToSend);
        messageToSend = nullptr;
    }
}

MessageSimulator::MessageSimulator() {}

MessageSimulator::~MessageSimulator() {}
