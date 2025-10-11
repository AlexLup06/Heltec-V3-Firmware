#include "macBase/OperationalBase.h"

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
        // TODO: setCurrentTime(packet->currentTime);
        break;
    }
    default:
        break;
    }
}

void OperationalBase::startTimeIR()
{
    startTime = -1;
    turnOnOperationMode();
}

bool OperationalBase::isStartTimePassed()
{
    int currentTime = millis();
    if (currentTime < startTime)
    {
        return false;
    }
    return true;
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

void OperationalBase::receiveDio1Interrupt()
{
    uint16_t irq = radio->getIrqFlags();
    radio->clearIrqFlags(irq);

    if (irq & RADIOLIB_SX126X_IRQ_PREAMBLE_DETECTED)
        onPreambleDetectedIR();

    if (irq & RADIOLIB_SX126X_IRQ_RX_DONE)
        onReceiveIR();

    if (irq & RADIOLIB_SX126X_IRQ_CRC_ERR)
        onCRCerrorIR();
}