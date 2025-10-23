#include "OperationalBase.h"

void OperationalBase::handleConfigPacket(const uint8_t messageType, const uint8_t *rawPacket, const uint8_t packetSize)
{
    switch (messageType)
    {
    case MESSAGE_TYPE_OPERATION_MODE_CONFIG:
    {
        OperationConfig_t *packet = (OperationConfig_t *)rawPacket;
        startTime = packet->startTime;
        break;
    }
    case MESSAGE_TYPE_TIME_SYNC:
    {
        TimeSync_t *packet = (TimeSync_t *)rawPacket;
        setClockFromTimestamp(packet->currentTime);

        if (!hasPropogatedTimeSync)
        {
            hasPropogatedTimeSync = true;
            int cw = random(0, maxCW);
            backoffTime = time(NULL) + cw * slotTime;
        }

        break;
    }
    default:
        break;
    }
}

void OperationalBase::handlePropagateConfigMessage()
{
    int currentTime = time(NULL);

    if (currentTime < backoffTime)
        return;

    // TODO: create and send the message - adjust the current time with time on air

    // TODO: clean timers and messages
}

void OperationalBase::startTimeIR()
{
    startTime = -1;
    turnOnOperationMode();
}

bool OperationalBase::isStartTimePassed()
{
    int currentTime = time(NULL);
    if (currentTime < startTime)
    {
        return false;
    }
    return true;
}

void OperationalBase::handleStartTime()
{
    if (isStartTimePassed())
    {
        startTimeIR();
    }
}

void OperationalBase::turnOnConfigMode()
{
    operationMode = CONFIG;
}
void OperationalBase::turnOnOperationMode()
{
    operationMode = OPERATIONAL;
}
bool OperationalBase::isInConfigMode()
{
    return operationMode == CONFIG;
}

void OperationalBase::setClockFromTimestamp(uint32_t unixTime)
{
    struct timeval tv;
    tv.tv_sec = unixTime;
    tv.tv_usec = 0;
    settimeofday(&tv, NULL);
}