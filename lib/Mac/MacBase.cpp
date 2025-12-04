#include "MacBase.h"

void MacBase::finish()
{
    clearMacData();
    finishRadioBase();
    finishProtocol();
}

void MacBase::clearMacData()
{
    nodeAnnounceTime = 0;
    clearQueue();
    clearIncompletePacketLists();
    finishReceiving();
    finishCurrentTransmission();
    msgScheduler.clear();
}

void MacBase::initRun()
{
    initProtocol();
    nodeAnnounceTime = millis();
}

void MacBase::handle()
{
    handleIRQFlags();
    SelfMessage *msg = msgScheduler.popNextReady();
    handleWithFSM(msg);
}

void MacBase::init(MacContext macCtx, uint8_t _nodeId)
{
    assignRadio(macCtx.radio);
    loggerManager = macCtx.loggerManager;
    loraDisplay = macCtx.loraDisplay;
    setOnSendCallback([this]()
                      { loraDisplay->incrementSent(); });
    nodeId = _nodeId;

    nodeAnnounceTime = millis();
}

void MacBase::finishCurrentTransmission()
{
    if (currentTransmission != nullptr)
    {
        free(currentTransmission->data);
        free(currentTransmission);
        currentTransmission = dequeuePacket();
    }
}

void MacBase::handleLowerPacket(const uint8_t messageType, uint8_t *packet, const size_t packetSize, float rssi)
{
    DEBUG_PRINTF("[MacBase] Received %s\n", msgIdToString(messageType));
    if (messageType == MESSAGE_TYPE_BROADCAST_CONFIG)
    {
        return;
    }

    if (messageType == MESSAGE_TYPE_BROADCAST_NODE_ANNOUNCE)
    {
        loraDisplay->updateNode(packet[1], rssi);
    }

    if (isReceivedPacketReady)
    {
        DEBUG_PRINTF("[Mac Base] We are currently handeling another packet: %s\n", msgIdToString(messageType));
        return;
    }

    isReceivedPacketReady = true;
    bool isMission = decapsulate(packet);

    receivedPacket = (ReceivedPacket *)malloc(sizeof(ReceivedPacket));
    receivedPacket->isMission = isMission;
    receivedPacket->messageType = packet[0];
    receivedPacket->size = packetSize;
    receivedPacket->payload = (uint8_t *)malloc(packetSize);

    memcpy(receivedPacket->payload, packet, packetSize);
}

void MacBase::finishReceiving()
{

    if (isReceivedPacketReady)
    {
        DEBUG_PRINTF("[Mac Base] Delete recived packet: %s\n", msgIdToString(receivedPacket->payload[0]));
        customPacketQueue.printQueue();
        isReceivedPacketReady = false;
        free(receivedPacket->payload);
        free(receivedPacket);
        receivedPacket = nullptr;
        setReceivingVar(false);
    }
    preambleTimedOutFlag = false;
}

void MacBase::handlePacketResult(Result result, bool withRTS, bool withContinuousRTS)
{
    customPacketQueue.printQueue();

    if (result.isComplete)
    {
        DEBUG_PRINTLN("[Mac Base] packet complete");
        if (result.sendUp)
        {
            FragmentedPacket *fragmentedPacket = result.completePacket;
            if (!result.isMission || fragmentedPacket->source == nodeId)
            {
                return;
            }

            DEBUG_PRINTLN("[Mac Base] Packet is mission so propagate");
            createMessage(
                fragmentedPacket->payload,
                fragmentedPacket->packetSize,
                fragmentedPacket->source,
                withRTS,
                true,
                withContinuousRTS,
                fragmentedPacket->id);

            ReceivedCompleteMission_data receivedMission = ReceivedCompleteMission_data();
            receivedMission.missionId = fragmentedPacket->id;
            receivedMission.source = fragmentedPacket->source;
            receivedMission.time = time(NULL);
            loggerManager->log(Metric::ReceivedCompleteMission_V, receivedMission);
        }
        else
        {
            loggerManager->increment(Metric::Collisions_S);
        }

        removeIncompletePacket(result.completePacket->source, result.isMission);
    }
}

void MacBase::onReceiveIR()
{
    standby();

    size_t len = getPacketLength();
    uint8_t *receiveBuffer = (uint8_t *)malloc(len);
    if (!receiveBuffer)
    {
        DEBUG_PRINTLN("[Mac Base] ERROR: malloc failed!");
        return;
    }

    int state = readData(receiveBuffer, len);
    float rssi = getRSSI();

    if (state == RADIOLIB_ERR_NONE)
    {
        loraDisplay->incrementReceived();
        handleLowerPacket(receiveBuffer[0], receiveBuffer, len, rssi);
    }
    else
    {
        DEBUG_PRINTF("[Mac Base] Receive failed: %d\n", state);
    }
    free(receiveBuffer);
    startReceive();
}

void MacBase::logReceivedStatistics(const uint8_t *data, const size_t len, bool isMission)
{
    ReceivedBytes_data receivedByes = ReceivedBytes_data();
    receivedByes.bytes = len;
    loggerManager->log(Metric::ReceivedBytes_V, receivedByes);
    DEBUG_PRINTF("[Mac Base] Log received bytes: %d\n", len);

    ReceivedEffectiveBytes_data receivedEffectiveBytes = ReceivedEffectiveBytes_data();
    if ((data[0] & 0x7F) == MESSAGE_TYPE_BROADCAST_FRAGMENT)
    {
        receivedEffectiveBytes.bytes = len - BROADCAST_FRAGMENT_METADATA_SIZE;
        loggerManager->log(Metric::ReceivedEffectiveBytes_V, receivedEffectiveBytes);
        DEBUG_PRINTF("[Mac Base] Log received effective bytes: %d\n", len - BROADCAST_FRAGMENT_METADATA_SIZE);
    }

    if ((data[0] & 0x7F) == MESSAGE_TYPE_BROADCAST_LEADER_FRAGMENT)
    {
        receivedEffectiveBytes.bytes = len - BROADCAST_LEADER_FRAGMENT_METADATA_SIZE;
        loggerManager->log(Metric::ReceivedEffectiveBytes_V, receivedEffectiveBytes);
        DEBUG_PRINTF("[Mac Base] Log received effective bytes: %d\n", len - BROADCAST_LEADER_FRAGMENT_METADATA_SIZE);
    }
}