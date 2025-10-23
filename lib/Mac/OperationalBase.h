#pragma once

#include <Arduino.h>
#include <RadioLib.h>
#include "definitions.h"
#include "messages.h"
#include "RadioHandler.h"
#include "time.h"

enum OperationMode
{
    CONFIG,
    OPERATIONAL,
};

class OperationalBase
{
public:
    OperationMode operationMode = CONFIG;

    virtual ~OperationalBase() {}
    virtual void applyModemConfig(uint8_t spreading_factor, uint8_t transmission_power, uint32_t frequency, uint32_t bandwidth) = 0;
    void handleConfigPacket(const uint8_t messageType, const uint8_t *rawPacket, const uint8_t packetSize);

    void turnOnConfigMode();
    void turnOnOperationMode();
    bool isInConfigMode();
    void startTimeIR();
    void handleStartTime();
    void handlePropagateConfigMessage(); // needs to run in main handle()

    bool isStartTimePassed();

    virtual String getProtocolName() = 0;

private:
    uint32_t startTime = -1;
    bool hasPropogatedTimeSync = false;
    uint8_t maxCW = 50;
    uint8_t slotTime = 200;
    uint32_t backoffTime = -1;
    void setClockFromTimestamp(uint32_t unixTime);
};