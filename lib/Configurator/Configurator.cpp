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

    if ((flags & RADIOLIB_SX126X_IRQ_RX_DONE) != 0 &&
        (flags & (RADIOLIB_SX126X_IRQ_HEADER_ERR | RADIOLIB_SX126X_IRQ_CRC_ERR)) == 0)
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
                loraDisplay->updateNode(receiveBuffer[3], rssi);
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

        uint64_t nowUnix_MS = unixTimeMs();
        if (packet->currentTime_UNIX_MS > 1765574916'000 && packet->currentTime_UNIX_MS < 1765920653'000 && nowUnix_MS < 1765574916'000)
        {
            Serial.printf("[CONFIG] Received CONFIG message: currentTime=%llu (UTC)\n",
                          (unsigned long long)packet->currentTime_UNIX_MS);

            setClockFromTimestamp(packet->currentTime_UNIX_MS);

            struct timeval tv;
            gettimeofday(&tv, nullptr);

            struct tm timeinfo;
            localtime_r(&tv.tv_sec, &timeinfo);

            Serial.printf("Current time: %02d:%02d:%02d.%03ld %02d/%02d/%04d\n",
                          timeinfo.tm_hour,
                          timeinfo.tm_min,
                          timeinfo.tm_sec,
                          tv.tv_usec / 1000, // milliseconds
                          timeinfo.tm_mday,
                          timeinfo.tm_mon + 1,
                          timeinfo.tm_year + 1900);
        }

        if (packet->startTime_UNIX_MS > 1765574916'000 &&
            packet->startTime_UNIX_MS < 1765920653'000 &&
            startTime_UNIX_MS < 1765574916'000 &&
            packet->networkId >= 0 &&
            packet->networkId <= 2 &&
            packet->numberOfNodes > 0 &&
            packet->numberOfNodes <= 8)
        {
            startTime_UNIX_MS = packet->startTime_UNIX_MS;
            networkId = packet->networkId;
            numberOfNodes = packet->numberOfNodes;

            config.startTime_UNIX_MS = packet->startTime_UNIX_MS;
            config.networkId = packet->networkId;
            config.numberOfNodes = packet->numberOfNodes;

            Serial.printf("[CONFIG] Received CONFIG message: startTime=%llu (UTC)\n", packet->startTime_UNIX_MS);
            loggerManager->setNetworkTopology(networkIdToString(networkId), numberOfNodes);

            time_t t = (time_t)(packet->startTime_UNIX_MS / 1000ULL);
            struct tm timeinfo;
            localtime_r(&t, &timeinfo);
            uint16_t ms = packet->startTime_UNIX_MS % 1000ULL;
            Serial.printf("Start Time: %02d:%02d:%02d.%03u %02d/%02d/%04d\n",
                          timeinfo.tm_hour,
                          timeinfo.tm_min,
                          timeinfo.tm_sec,
                          ms,
                          timeinfo.tm_mday,
                          timeinfo.tm_mon + 1,
                          timeinfo.tm_year + 1900);
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
    uint64_t nowUnix_MS = unixTimeMs();
    if (nowUnix_MS >= startTime_UNIX_MS && startTime_UNIX_MS > 0)
    {
        turnOnOperationMode();
    }

    unsigned long nowMs = millis();
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

void Configurator::setClockFromTimestamp(uint64_t unixTime_MS)
{
    struct timeval tv;

    tv.tv_sec = (time_t)(unixTime_MS / 1000ULL);
    tv.tv_usec = (suseconds_t)((unixTime_MS % 1000ULL) * 1000ULL);

    settimeofday(&tv, nullptr);
}

void Configurator::sendBroadcastConfig()
{
    BroadcastConfig msg{};
    msg.messageType = MESSAGE_TYPE_BROADCAST_CONFIG;
    msg.currentTime_UNIX_MS = unixTimeMs() + (uint64_t)(getToAByPacketSizeInUS(sizeof(BroadcastConfig)) / 1000);
    msg.source = nodeId;
    msg.startTime_UNIX_MS = config.startTime_UNIX_MS;
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

void Configurator::confirmSetup(uint64_t _startTime_UNIX_MS)
{
    config.networkId = networkId;
    config.numberOfNodes = numberOfNodes;
    config.startTime_UNIX_MS = _startTime_UNIX_MS;

    startTime_UNIX_MS = _startTime_UNIX_MS;
    loggerManager->setNetworkTopology(networkIdToString(networkId), numberOfNodes);
}