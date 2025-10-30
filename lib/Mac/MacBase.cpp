#include "MacBase.h"

void MacBase::finish()
{
}

void MacBase::clearMacData()
{
}

void MacBase::initRun()
{
    nodeAnnounceTime = millis();
}

void MacBase::handle()
{
    if (nodeAnnounceTime < millis())
    {
        createNodeAnnouncePacket(macAdress, nodeId);
        nodeAnnounceTime += 5000;
    }
    handleWithFSM();
}

void MacBase::init(MacContext macCtx)
{
    assignRadio(macCtx.radio);
    loggerManager = macCtx.loggerManager;
    loraDisplay = macCtx.loraDisplay;

    esp_read_mac(macAdress, ESP_MAC_WIFI_STA);

    nodeAnnounceTime = millis();
    nodeId = macAdress[5];
}

void MacBase::finishCurrentTransmission()
{
    free(currentTransmission->data);
    free(currentTransmission);
    currentTransmission = dequeuePacket();
}

void MacBase::handleLowerPacket(const uint8_t messageType, uint8_t *packet, const size_t packetSize, float rssi)
{
    if (messageType == MESSAGE_TYPE_BROADCAST_CONFIG)
        return;

    if (messageType == MESSAGE_TYPE_BROADCAST_NODE_ANNOUNCE)
    {
        loraDisplay->updateNode(packet[1], rssi);
    }

    isReceivedPacketReady = true;
    bool isMission = decapsulate(packet);

    receivedPacket = (ReceivedPacket_t *)malloc(sizeof(ReceivedPacket_t));
    receivedPacket->isMission = isMission;
    receivedPacket->messageType = messageType;
    receivedPacket->size = packetSize;
    memcpy(receivedPacket->payload, packet, packetSize);

    handleWithFSM();

    isReceivedPacketReady = false;
    free(receivedPacket);
}

void MacBase::handlePacketResult(Result result, bool withRTS)
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
            createMessage(fragmentedPacket->payload, fragmentedPacket->packetSize, fragmentedPacket->source, withRTS, true);
        }
    }
}

void MacBase::onReceiveIR()
{
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