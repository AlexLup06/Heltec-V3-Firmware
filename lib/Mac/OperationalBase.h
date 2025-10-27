#pragma once

#include <Arduino.h>
#include <RadioLib.h>
#include "definitions.h"
#include "messages.h"
#include "RadioHandler.h"
#include "RadioBase.h"
#include "time.h"
#include "functions.h"

enum OperationMode
{
    CONFIG,
    OPERATIONAL,
};

class OperationalBase : public RadioBase
{
public:
    OperationMode operationMode = CONFIG;

    virtual ~OperationalBase() {}
    void handleConfigPacket(const uint8_t messageType, const uint8_t *rawPacket, const size_t packetSize, int rssi);

    void turnOnOperationMode();
    bool isInConfigMode();
    void handleConfigMode(); // runs in main handle()

    virtual void updateNodeRssi(uint16_t id, int rssi) = 0;
    virtual void incrementSent() = 0;
    virtual uint16_t getNodeId() = 0;

    virtual String getProtocolName() = 0;

private:
    bool hasPropogatedTimeSync = false;
    bool hasSentTimeSync = false;
    bool hasReachedNextMinute = false;
    bool hasSentNodeIndicator = false;
    bool shouldExitConfig = false;
    bool hasSentConfigMessage = false;
    bool hasReceivedConfigMessage = false;
    bool hasScheduledConfigPropagation = false;
    bool hasPropogatedConfigMessage = false;
    
    uint32_t startTimeUnix = 0;
    int maxCW = 100;           // contention window size
    int slotTime = 50;         // ms
    int chosenSlot = 0;        // random slot index
    int nextMinuteTarget = -1; // for waiting until next-next minute
    unsigned long backoffTime = 0;
    unsigned long backoffTimeConfig = 0;
    unsigned long cycleStartMs = 0;

    void sendTimeSyncMessage();
    void sendNodeIndicatorMessage();
    void sendConfigMessage();

    void setClockFromTimestamp(uint32_t unixTime);
};