#include "MacBase.h"

void MacBase::finish()
{
    clearMacData();
    finishRadioBase();
    fsm.setState(0);
    DEBUG_PRINTLN("Cleared Data and ready for next run");
}

void MacBase::clearMacData()
{
    clearQueue();
    clearIncompletePacketLists();

    if (receivedPacket != nullptr)
    {
        free(receivedPacket->payload);
        free(receivedPacket);
    }
    isReceivedPacketReady = false;
    finishCurrentTransmission();
}

void MacBase::initRun()
{
    nodeAnnounceTime = millis();
}

void MacBase::handle()
{
    handleDio1Interrupt();
    if (nodeAnnounceTime < millis())
    {
        createNodeAnnouncePacket(nodeId);
        nodeAnnounceTime = millis() + 5000;
    }
    handleWithFSM();
}

void MacBase::init(MacContext macCtx, uint8_t _nodeId)
{
    assignRadio(macCtx.radio);
    loggerManager = macCtx.loggerManager;
    loraDisplay = macCtx.loraDisplay;
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
    if (messageType == MESSAGE_TYPE_BROADCAST_CONFIG)
    {
        return;
    }

    if (messageType == MESSAGE_TYPE_BROADCAST_NODE_ANNOUNCE)
    {
        loraDisplay->updateNode(packet[1], rssi);
        return;
    }

    isReceivedPacketReady = true;
    bool isMission = decapsulate(packet);

    receivedPacket = (ReceivedPacket *)malloc(sizeof(ReceivedPacket));
    receivedPacket->isMission = isMission;
    receivedPacket->messageType = messageType;
    receivedPacket->size = packetSize;
    receivedPacket->payload = (uint8_t *)malloc(packetSize);

    memcpy(receivedPacket->payload, packet, packetSize);

    handleWithFSM();

    isReceivedPacketReady = false;
    free(receivedPacket->payload);
    free(receivedPacket);
    receivedPacket = nullptr;
}

void MacBase::handlePacketResult(Result result, bool withRTS, bool withContinuousRTS)
{
    if (result.isComplete)
    {
        if (result.sendUp)
        {
            FragmentedPacket *fragmentedPacket = result.completePacket;
            if (!result.isMission || fragmentedPacket->source == nodeId)
            {
                return;
            }
            createMessage(fragmentedPacket->payload, fragmentedPacket->packetSize, fragmentedPacket->source, withRTS, true, withContinuousRTS);
        }

        removeIncompletePacket(result.completePacket->source, result.isMission);
    }
}

void MacBase::onReceiveIR()
{
    DEBUG_PRINTLN("[MacBase] onReceiveIR");
    standby();

    size_t len = getPacketLength();
    uint8_t *receiveBuffer = (uint8_t *)malloc(len);
    if (!receiveBuffer)
    {
        DEBUG_PRINTLN("ERROR: malloc failed!");
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
        DEBUG_PRINTF("Receive failed: %d\n", state);
    }
    free(receiveBuffer);
    startReceive();
}