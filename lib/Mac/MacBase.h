#pragma once

#include <Arduino.h>
#include "DataLogger.h"
#include "config.h"
#include "OperationalBase.h"
#include "RadioBase.h"
#include "PacketBase.h"
#include "LoRaDisplay.h"

#pragma pack(1)
typedef struct ReceivedPacket
{
    uint8_t messageType;
    int rssi;
    uint16_t size;
    uint8_t *payload;
    bool isMission;
} ReceivedPacket_t;
#pragma pack()

class MacBase : public OperationalBase, public PacketBase
{
protected:
    uint8_t macAdress[6];
    uint8_t nodeId;
    uint16_t messageIdCounter;
    uint16_t missionIdCounter;

    QueuedPacket *currentTransmission;

    uint32_t nodeAnnounceTime = -1;
    bool isReceivedPacketReady = false;
    ReceivedPacket *receivedPacket;

public:
    DataLogger *dataLogger;
    LoRaDisplay *loraDisplay;

    MacBase() : messageIdCounter(0), missionIdCounter(0) {}
    virtual ~MacBase() {}

    virtual void initProtocol() = 0;
    void init();
    void handle();
    void finish();
    virtual void handleWithFSM() = 0;
    virtual void clearMacData() = 0;

    void finishCurrentTransmission();
    void updateNodeRssi(uint16_t id, int rssi) override;
    void incrementSent() override;
    uint16_t getNodeId() override;

    virtual void handleProtocolPacket(const uint8_t messageType, const uint8_t *packet, const size_t packetSize, const int rssi, bool isMission) = 0;
    virtual void handleUpperPacket(MessageToSend_t *serialPayloadFloodPacket) = 0;
    void handlePacketResult(Result result);
    void handleLowerPacket(const uint8_t messageType, uint8_t *packet, const size_t packetSize, const int rssi);
    void onReceiveIR() override;
};
