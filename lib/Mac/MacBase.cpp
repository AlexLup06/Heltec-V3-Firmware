#include "MacBase.h"

void MacBase::finish()
{
}

void MacBase::handle()
{
    if (isInConfigMode())
    {
        handlePropagateConfigMessage();
    }
    else
    {
        handleWithFSM();
    }
}

void MacBase::init()
{
    esp_read_mac(macAdress, ESP_MAC_WIFI_STA);

    nodeId = macAdress[5];
    turnOnOperationMode();
}

void MacBase::finishCurrentTransmission()
{
}

void MacBase::handleUpperPacket(MessageToSend_t *serialPayloadFloodPacket)
{
}

void MacBase::handleLowerPacket(const uint8_t messageType, const uint8_t *packet, const uint8_t packetSize, const int rssi)
{
    if (messageType == MESSAGE_TYPE_TIME_SYNC || messageType == MESSAGE_TYPE_OPERATION_MODE_CONFIG)
    {
        handleConfigPacket(messageType, packet, packetSize);
        return;
    }

    if (!isInConfigMode())
    {
        isReceivedPacketReady = true;
        receivedPacket = (ReceivedPacket_t *)malloc(sizeof(ReceivedPacket_t));
        receivedPacket->messageType = messageType;
        // TODO: assign payload
        receivedPacket->rssi = rssi;
        receivedPacket->size = packetSize;

        handleWithFSM();

        isReceivedPacketReady = false;
        free(receivedPacket);
    }
}

void MacBase::onReceiveIR()
{
    radio->standby();

    size_t len = radio->getPacketLength();
    uint8_t *receiveBuffer = (uint8_t *)malloc(len);
    if (!receiveBuffer)
    {
        Serial.println("ERROR: malloc failed!");
        return;
    }

    int state = radio->readData(receiveBuffer, len);
    float rssi = radio->getRSSI();

    if (state == RADIOLIB_ERR_NONE)
    {
        handleLowerPacket(receiveBuffer[0], receiveBuffer, len, rssi);
    }
    else
    {
        Serial.printf("Receive failed: %d\n", state);
    }
    free(receiveBuffer);
    radio->startReceive();
}
