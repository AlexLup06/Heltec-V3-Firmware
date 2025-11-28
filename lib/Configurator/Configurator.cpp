#include "Configurator.h"

void Configurator::setCtx(LoRaDisplay *_loraDisplay, SX1262Public *_radio, LoggerManager *_loggerManager, uint8_t _nodeId)
{
    loraDisplay = _loraDisplay;
    radio = _radio;
    nodeId = _nodeId;
    loggerManager = _loggerManager;
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
            if (receiveBuffer[0] == MESSAGE_TYPE_BROADCAST_CONFIG)
            {
                loraDisplay->updateNode(receiveBuffer[1], rssi);
            }

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

        if (!nodeIdArrayContains(packet->source))
        {
            return;
        }

        time_t now = time(NULL);
        if (now < 946684800)
        {
            setClockFromTimestamp(packet->currentTime);
        }

        if (packet->numberOfNodes > 0)
        {
            startTimeUnix = packet->startTime;
            networkId = packet->networkId;
            numberOfNodes = packet->numberOfNodes;
            DEBUG_PRINTF("[CONFIG] Received CONFIG message: startTime=%lu (UTC)\n", packet->startTime);
            loggerManager->setNetworkTopology(networkIdToString(networkId), numberOfNodes);
        }
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
    msg.messageType = MESSAGE_TYPE_BROADCAST_CONFIG;
    msg.currentTime = static_cast<uint32_t>(time(NULL)) + getToAByPacketSizeInUS(sizeof(BroadcastConfig)) / 1'000'000;
    msg.source = nodeId;
    msg.startTime = static_cast<uint32_t>(config.startTimeUnix);
    msg.networkId = config.networkId;
    msg.numberOfNodes = config.numberOfNodes;

    int state = radio->sendRaw((uint8_t *)&msg, sizeof(msg));

    if (state != RADIOLIB_ERR_NONE)
    {
        DEBUG_PRINTF("[RadioBase] Send failed: %d\n", state);
        radio->startReceive();
    }

    loraDisplay->setNetworkId(networkId);
    loraDisplay->setNumberOfNodes(numberOfNodes);
}

void Configurator::setNetworkTopology(bool forward)
{
    if (forward)
    {
        numberOfNodes++;
        if (numberOfNodes > MAX_NUMBER_OF_NODES)
        {
            numberOfNodes = 0;
            networkId++;
            if (networkId >= MAX_NETWORK_ID)
            {
                networkId = 0;
                numberOfNodes = 0;
            }
        }
    }
    else
    {
        if (numberOfNodes == 0)
        {
            if (networkId == 0)
            {
                networkId = MAX_NETWORK_ID - 1;
                numberOfNodes = MAX_NUMBER_OF_NODES - 1;
            }
            else
            {
                networkId--;
                numberOfNodes = MAX_NUMBER_OF_NODES - 1;
            }
        }
        else
        {
            numberOfNodes--;
        }
    }
    loggerManager->setNetworkTopology(networkIdToString(networkId), numberOfNodes);
    loraDisplay->setNetworkId(networkId);
    loraDisplay->setNumberOfNodes(numberOfNodes);
}

void Configurator::confirmSetup(time_t startTime)
{
    config.networkId = networkId;
    config.numberOfNodes = numberOfNodes;
    config.startTimeUnix = startTime;

    startTimeUnix = startTime;
    loggerManager->setNetworkTopology(networkIdToString(networkId), numberOfNodes);
}