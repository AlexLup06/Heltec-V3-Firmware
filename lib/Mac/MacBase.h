#pragma once

#include <Arduino.h>
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
private:
    std::map<uint16_t, unsigned long> lastTrajectoryTime;

    void recordTrajectory(uint16_t nodeId);
    uint16_t timeSinceLastTrajectory(uint16_t nodeId) const;
    void logTimeToLastTrajectory(uint8_t source);

protected:
    QueuedPacket *currentTransmission;

    uint32_t nodeAnnounceTime = 0;
    bool isReceivedPacketReady = false;
    ReceivedPacket *receivedPacket;
    void logReceivedStatistics(const uint8_t *data, const size_t len);

public:
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

    virtual const char *getProtocolName() = 0;
};
