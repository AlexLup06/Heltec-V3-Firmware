#include "Configurator.h"

void Configurator::setCtx(LoRaDisplay *_loraDisplay, LoggerManager *_loggerManager, SX1262Public *_radio)
{
    loraDisplay = _loraDisplay;
    loggerManager = _loggerManager;
    radio = _radio;
}

void Configurator::receiveDio1Interrupt()
{
    if (isMaster)
        return;

    // only handle Packets if we are not Master
    uint16_t irq = radio->getIrqFlags();
    radio->clearIrqFlags(irq);

    if (irq & RADIOLIB_SX126X_IRQ_RX_DONE)
    {
        radio->standby();

        size_t len = radio->getPacketLength();
        uint8_t *receiveBuffer = (uint8_t *)malloc(len);
        if (!receiveBuffer)
        {
            DEBUG_PRINTLN("ERROR: malloc failed!");
            return;
        }

        int state = radio->readData(receiveBuffer, len);
        float rssi = radio->getRSSI();

        if (state == RADIOLIB_ERR_NONE)
        {
            handleConfigPacket(receiveBuffer[0], receiveBuffer, len, rssi);
        }
        else
        {
            DEBUG_PRINTF("Receive failed: %d\n", state);
        }
        free(receiveBuffer);
        radio->startReceive();
    }
}

void Configurator::handleConfigPacket(const uint8_t messageType, const uint8_t *rawPacket, const size_t packetSize, int rssi)
{
    switch (messageType)
    {
    case MESSAGE_TYPE_OPERATION_MODE_CONFIG:
    {
        OperationConfig_t *packet = (OperationConfig_t *)rawPacket;

        // Avoid duplicates
        if (hasPropogatedConfigMessage)
            break;

        hasPropogatedConfigMessage = true;

        // Schedule propagation with random backoff
        int cw = random(0, maxCW);
        backoffTimeConfig = millis() + cw * slotTime;

        // Store the received start time
        startTimeUnix = packet->startTime;
        shouldExitConfig = true;

        DEBUG_PRINTF("[CONFIG] Received CONFIG message: startTime=%lu (UTC)\n", packet->startTime);
        DEBUG_PRINTF("[CONFIG] Will propagate after backoff (cw=%d)\n", cw);
        break;
    }
    case MESSAGE_TYPE_TIME_SYNC:
    {
        TimeSync_t *packet = (TimeSync_t *)rawPacket;
        uint32_t currentTime = packet->currentTime + getSendTimeByPacketSizeInUS(5) + 2;
        setClockFromTimestamp(currentTime);

        if (!hasPropogatedTimeSync)
        {
            hasPropogatedTimeSync = true;
            int cw = random(0, maxCW);
            backoffTime = millis() + cw * slotTime;
        }

        break;
    }
    case MESSAGE_TYPE_NODE_INDICATOR:
    {
        Node_Indicator_t *packet = (Node_Indicator_t *)rawPacket;
        uint16_t source = packet->source;
        loraDisplay->updateNode(source, rssi);
        break;
    }
    default:
        break;
    }
}

// called from main loop if in config mode
void Configurator::handleConfigMode()
{
    if (isMaster)
    {
        // TODO: just check if start time has been set and has passed; if it has then turnOnOperatingMode
        time_t nowUnix = time(NULL);
        if (nowUnix >= startTimeUnix && startTimeUnix != 0)
        {
            turnOnOperationMode();
        }
        return;
    }

    unsigned long nowMs = millis();
    time_t now = time(NULL);
    struct tm *tm_info = localtime(&now);

    if (!hasPropogatedTimeSync)
        return; // We have no valid time yet

    if (!hasSentTimeSync)
    {
        if (nowMs < backoffTime)
            return;
        sendTimeSync();
        hasSentTimeSync = true;
        DEBUG_PRINTLN("[CONFIG] Propagated TIME_SYNC after backoff");

        int nextMinute = (tm_info->tm_min + 2) % 60;
        nextMinuteTarget = nextMinute;
        hasReachedNextMinute = false;
        return;
    }

    // --- 3️⃣ Wait until the next full minute (not current) ---
    if (!hasReachedNextMinute)
    {
        if (tm_info->tm_min == nextMinuteTarget && tm_info->tm_sec == 0)
        {
            hasReachedNextMinute = true;
            cycleStartMs = nowMs;
            chosenSlot = random(0, maxCW);
            hasSentNodeIndicator = false;
            DEBUG_PRINTF("[CONFIG] Entered indicator cycle, first slot=%d\n", chosenSlot);
        }
        else
        {
            return; // still waiting for the next full minute
        }
    }

    // --- 4️⃣ Handle periodic NODE_INDICATOR cycles ---
    unsigned long cycleDuration = (unsigned long)maxCW * slotTime;
    unsigned long cycleElapsed = nowMs - cycleStartMs;

    // Start of a new cycle
    if (cycleElapsed >= cycleDuration)
    {
        // reset for next cycle
        cycleStartMs = nowMs;
        chosenSlot = random(0, maxCW);
        hasSentNodeIndicator = false;
        DEBUG_PRINTF("[CONFIG] New indicator cycle -> slot %d\n", chosenSlot);
        cycleElapsed = 0;
    }

    // Send our indicator message in our chosen slot
    if (!hasSentNodeIndicator && cycleElapsed >= (unsigned long)chosenSlot * slotTime)
    {
        sendNodeIndicatorMessage();
        hasSentNodeIndicator = true;
        DEBUG_PRINTF("[CONFIG] Sent NODE_INDICATOR at slot %d\n", chosenSlot);
    }

    // --- 4️⃣ Handle CONFIG message propagation ---
    if (hasPropogatedConfigMessage && !hasSentConfigMessage && millis() >= backoffTimeConfig)
    {
        sendConfigMessage(startTimeUnix);
        hasSentConfigMessage = true;
        DEBUG_PRINTF("[CONFIG] Propagated CONFIG message after backoff\n");
    }

    // --- 5️⃣ Leave CONFIG mode when startTime is reached ---
    time_t nowUnix = time(NULL);
    if (shouldExitConfig && nowUnix >= startTimeUnix)
    {
        DEBUG_PRINTF("[CONFIG] Start time reached (%lu) -> Switching to OPERATIONAL\n", startTimeUnix);
        turnOnOperationMode();
        shouldExitConfig = false;
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

void Configurator::sendTimeSync()
{
    TimeSync_t msg{};
    msg.currentTime = static_cast<uint32_t>(time(NULL)) + getSendTimeByPacketSizeInUS(sizeof(TimeSync_t)) / 1'000'000;

    radio->sendRaw((uint8_t *)&msg, sizeof(msg));
    loraDisplay->incrementSent();

    DEBUG_PRINTF("[CONFIG] Master sent TIME_SYNC t=%lu\n", msg.currentTime);
}

void Configurator::sendConfigMessage(time_t startTime)
{
    OperationConfig_t msg{};
    msg.startTime = static_cast<uint32_t>(startTime); // start in +1 minute

    radio->sendRaw((uint8_t *)&msg, sizeof(msg));
    loraDisplay->incrementSent();

    DEBUG_PRINTF("[CONFIG] Master sent CONFIG message startTime=%lu (UTC)\n", msg.startTime);
}

void Configurator::sendNodeIndicatorMessage()
{
    Node_Indicator_t msg{};
    msg.messageType = MESSAGE_TYPE_NODE_INDICATOR;
    msg.source = nodeId;

    radio->sendRaw((uint8_t *)&msg, sizeof(msg));
    loraDisplay->incrementSent();
    DEBUG_PRINTF("[SEND] NODE_INDICATOR id=%u\n", msg.source);
}