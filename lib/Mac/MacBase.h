#pragma once

#include <Arduino.h>
#include "LoggerManager.h"
#include "LoRaDisplay.h"
#include "config.h"
#include "RadioBase.h"
#include "PacketBase.h"
#include "FSMA.h"
#include "SelfMessageScheduler.h"

struct ReceivedPacket
{
    uint8_t messageType;
    uint16_t size;
    uint8_t *payload;
    bool isMission;
};

struct MacContext
{
    SX1262Public *radio = nullptr;
    LoggerManager *loggerManager = nullptr;
    LoRaDisplay *loraDisplay = nullptr;
};

class MacBase : public RadioBase, public PacketBase
{
protected:
    QueuedPacket *currentTransmission;

    uint32_t nodeAnnounceTime = -1;
    bool isReceivedPacketReady = false;
    ReceivedPacket *receivedPacket;

public:
    LoggerManager *loggerManager;
    LoRaDisplay *loraDisplay;
    SelfMessageScheduler msgScheduler;

    MacBase() {}
    virtual ~MacBase() {}

    void init(MacContext macCtx, uint8_t nodeId);
    void initRun();
    void handle();
    void finish();
    void clearMacData();
    virtual void initProtocol() {};
    virtual void finishProtocol() {};
    virtual void handleWithFSM(SelfMessage *msg = nullptr) = 0;

    void finishCurrentTransmission();
    void finishReceiving();

    virtual void handleProtocolPacket(ReceivedPacket *receivedPacket) = 0;
    virtual void handleUpperPacket(MessageToSend *serialPayloadFloodPacket) = 0;
    void handlePacketResult(Result result, bool withRTS, bool withContinuousRTS);
    void handleLowerPacket(const uint8_t messageType, uint8_t *packet, const size_t packetSize, float rssi);
    void onReceiveIR() override;

    virtual String getProtocolName() = 0;
};
