#pragma once

#include <Arduino.h>
#include "IncompletePacketList.h"
#include "DataLogger.h"
#include "config.h"
#include "OperationalBase.h"
#include "RadioBase.h"
#include "PacketBase.h"

#pragma pack(1)
typedef struct ReceivedPacket
{
    uint8_t messageType;
    int rssi;
    uint16_t size;
    uint8_t *payload;
} ReceivedPacket_t;
#pragma pack()

class MacBase : public OperationalBase, public RadioBase, public PacketBase
{
protected:
    uint8_t macAdress[6];
    uint8_t nodeId;
    uint16_t messageIdCounter;
    uint16_t missionIdCounter;

    DataLogger *dataLogger;

    QueuedPacket *currentTransmission;

    IncompletePacketList incompleteMissionPackets;
    IncompletePacketList incompleteNeighbourPackets;

    bool isReceivedPacketReady = false;
    ReceivedPacket *receivedPacket;

public:
    MacBase() : messageIdCounter(0), missionIdCounter(0) {}
    virtual ~MacBase() {}

    virtual void init() = 0;
    void handle();
    void finish();
    virtual void handleWithFSM() = 0;
    virtual void clearMacData() = 0;
    virtual uint8_t setNodeID(uint8_t newNodeID) = 0;

    virtual void sendRaw(const uint8_t *rawPacket, const uint8_t size) = 0;
    void finishCurrentTransmission();

    virtual void handleProtocolPacket(const uint8_t messageType, const uint8_t *packet, const uint8_t packetSize, const int rssi) = 0; // this is FSM
    virtual void handleUpperPacket(MessageToSend_t *serialPayloadFloodPacket);
    void handleLowerPacket(const uint8_t messageType, const uint8_t *packet, const uint8_t packetSize, const int rssi);
    void onReceiveIR() override;
};
