#include "MacBase.h"

void MacBase::finish()
{
}

void MacBase::handle()
{
    if (isInConfigMode())
    {
        handleConfigMode();
    }
    else
    {
        if (nodeAnnounceTime < millis())
        {
            createNodeAnnouncePacket(macAdress, nodeId);
            nodeAnnounceTime += 5000;
        }
        handleWithFSM();
    }
}

void MacBase::init()
{
    esp_read_mac(macAdress, ESP_MAC_WIFI_STA);

    nodeAnnounceTime = millis();
    nodeId = macAdress[5];

    turnOnOperationMode();
    initProtocol();
}

void MacBase::finishCurrentTransmission()
{
    free(currentTransmission);
    currentTransmission = dequeuePacket();
}

void MacBase::updateNodeRssi(uint16_t id, int rssi)
{
    loraDisplay->updateNode(id, rssi);
}

void MacBase::incrementSent()
{
    loraDisplay->incrementSent();
}

uint16_t MacBase::getNodeId()
{
    return nodeId;
}

void MacBase::handleLowerPacket(const uint8_t messageType, uint8_t *packet, const size_t packetSize, const int rssi)
{
    if (messageType == MESSAGE_TYPE_TIME_SYNC || messageType == MESSAGE_TYPE_OPERATION_MODE_CONFIG)
    {
        handleConfigPacket(messageType, packet, packetSize, rssi);
        return;
    }

    if (!isInConfigMode())
    {
        isReceivedPacketReady = true;
        bool isMission = decapsulate(packet);

        receivedPacket = (ReceivedPacket_t *)malloc(sizeof(ReceivedPacket_t));
        receivedPacket->isMission = isMission;
        receivedPacket->messageType = messageType;
        receivedPacket->rssi = rssi;
        receivedPacket->size = packetSize;
        memcpy(receivedPacket->payload, packet, packetSize);

        handleWithFSM();

        isReceivedPacketReady = false;
        free(receivedPacket);
    }
}

void MacBase::handlePacketResult(Result result)
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
            createMessage(fragmentedPacket->payload, fragmentedPacket->packetSize, fragmentedPacket->source, false, true);
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
        DEBUG_LORA_SERIAL("ERROR: malloc failed!");
        return;
    }

    int state = readData(receiveBuffer, len);
    float rssi = getRSSI();

    if (state == RADIOLIB_ERR_NONE)
    {
        handleLowerPacket(receiveBuffer[0], receiveBuffer, len, rssi);
    }
    else
    {
        DEBUG_LORA_SERIAL("Receive failed: %d\n", state);
    }
    free(receiveBuffer);
    startReceive();
}
