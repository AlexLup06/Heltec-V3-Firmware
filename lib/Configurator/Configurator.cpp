#include "Configurator.h"

void Configurator::setCtx(LoRaDisplay *_loraDisplay, SX1262Public *_radio, uint8_t _nodeId)
{
    loraDisplay = _loraDisplay;
    radio = _radio;
    nodeId = _nodeId;
}

void Configurator::handleDioInterrupt()
{
    uint16_t flags = radio->irqFlag;
    radio->irqFlag = 0;

    if (flags & RADIOLIB_SX126X_IRQ_TX_DONE)
    {
        loraDisplay->incrementSent();
        radio->startReceive();
        return;
    }

    if (flags & RADIOLIB_SX126X_IRQ_RX_DONE)
    {
        radio->standby();

        size_t len = radio->getPacketLength();
        uint8_t *receiveBuffer = (uint8_t *)malloc(len);

        int state = radio->readData(receiveBuffer, len);
        float rssi = radio->getRSSI();

        if (state == RADIOLIB_ERR_NONE)
        {
            loraDisplay->updateNode(receiveBuffer[1], rssi);
            handleConfigPacket(receiveBuffer[0], receiveBuffer, len);
        }
        else
        {
            DEBUG_PRINTF("Receive failed: %d\n", state);
        }
        free(receiveBuffer);
        radio->startReceive();
    }
}

void Configurator::handleConfigPacket(const uint8_t messageType, const uint8_t *rawPacket, const size_t packetSize)
{
    switch (messageType)
    {
    case MESSAGE_TYPE_BROADCAST_CONFIG:
    {
        BroadcastConfig *packet = (BroadcastConfig *)rawPacket;

        startTimeUnix = packet->startTime;

        time_t now = time(NULL);
        if (now < 946684800)
        {
            DEBUG_PRINTF("[CONFIG] Set current Time: currentTime=%lu (UTC)\n", packet->currentTime);
            setClockFromTimestamp(packet->currentTime);
        }

        DEBUG_PRINTF("[CONFIG] Received CONFIG message: startTime=%lu (UTC)\n", packet->startTime);
        break;
    }
    default:
        break;
    }
}

void Configurator::handleConfigMode()
{
    handleDioInterrupt();
    time_t nowUnix = time(NULL);
    if (nowUnix >= startTimeUnix && startTimeUnix != 0)
    {
        turnOnOperationMode();
    }

    unsigned long nowMs = millis();
    time_t now = time(NULL);
    unsigned long cycleDuration = (unsigned long)maxCW * slotTime;
    unsigned long cycleElapsed = nowMs - cycleStartMs;

    if (cycleElapsed >= cycleDuration)
    {
        cycleStartMs = nowMs;
        chosenSlot = random(0, maxCW);
        hasSentConfigMessage = false;
        DEBUG_PRINTF("[CONFIG] New indicator cycle -> slot %d\n", chosenSlot);
        cycleElapsed = 0;
    }

    if (!hasSentConfigMessage && cycleElapsed >= (unsigned long)chosenSlot * slotTime)
    {
        sendBroadcastConfig();
        hasSentConfigMessage = true;
        DEBUG_PRINTF("[CONFIG] Sent MESSAGE_TYPE_BROADCAST_CONFIG at slot %d\n", chosenSlot);
    }
}

void Configurator::turnOnOperationMode()
{
    operationMode = OPERATIONAL;
}
bool Configurator::isInConfigMode()
{
    return operationMode == CONFIG;
}

void Configurator::setStartTime(time_t startTime)
{
    startTimeUnix = startTime;
}

void Configurator::setClockFromTimestamp(uint32_t unixTime)
{
    struct timeval tv;
    tv.tv_sec = unixTime;
    tv.tv_usec = 0;
    settimeofday(&tv, NULL);
}

void Configurator::sendBroadcastConfig()
{
    BroadcastConfig msg{};
    msg.currentTime = static_cast<uint32_t>(time(NULL)) + getToAByPacketSizeInUS(sizeof(BroadcastConfig)) / 1'000'000;
    msg.source = nodeId;
    msg.startTime = startTimeUnix;

    int state = radio->sendRaw((uint8_t *)&msg, sizeof(msg));
    DEBUG_PRINTF("[CONFIG] Master sending TIME_SYNC t=%lu\n", msg.currentTime);

    if (state != RADIOLIB_ERR_NONE)
    {
        DEBUG_PRINTF("[RadioBase] Send failed: %d\n", state);
        radio->startReceive();
    }
}
