#pragma once

#include <Arduino.h>
#include "LoggerManager.h"
#include "LoRaDisplay.h"
#include "config.h"
#include "RadioBase.h"
#include "PacketBase.h"

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

struct MacContext
{
    SX1262Public *radio = nullptr;
    LoggerManager *loggerManager = nullptr;
    LoRaDisplay *loraDisplay = nullptr;
    bool isMaster = true;
};

class MacBase : public RadioBase, public PacketBase
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
    LoggerManager *loggerManager;
    LoRaDisplay *loraDisplay;

    MacBase() : messageIdCounter(0), missionIdCounter(0) {}
    virtual ~MacBase() {}

    void init(MacContext macCtx);
    void handle();
    void finish();
    void clearMacData();
    virtual void handleWithFSM() = 0;

    void finishCurrentTransmission();

    virtual void handleProtocolPacket(const uint8_t messageType, const uint8_t *packet, const size_t packetSize, const int rssi, bool isMission) = 0;
    virtual void handleUpperPacket(MessageToSend_t *serialPayloadFloodPacket) = 0;
    void handlePacketResult(Result result, bool withRTS);
    void handleLowerPacket(const uint8_t messageType, uint8_t *packet, const size_t packetSize, const int rssi);
    void onReceiveIR() override;

    virtual String getProtocolName() = 0;
};
