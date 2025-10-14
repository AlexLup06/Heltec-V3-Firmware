#pragma once

#include <Arduino.h>
#include <RadioLib.h>
#include "helpers/definitions.h"
#include "helpers/Messages.h"
#include "radioHandler/RadioHandler.h"

enum OperationMode
{
    CONFIG,
    OPERATIONAL,
};

class OperationalBase
{
public:
    OperationMode operationMode = CONFIG;
    SX1262Public *radio;

    virtual ~OperationalBase() {}
    virtual void init() = 0;
    virtual void handle() = 0;
    virtual void finish() = 0;
    virtual void applyModemConfig(uint8_t spreading_factor, uint8_t transmission_power, uint32_t frequency, uint32_t bandwidth) = 0;
    virtual uint8_t setNodeID(uint8_t newNodeID) = 0;
    void handleConfigPacket(const uint8_t messageType, const uint8_t *rawPacket, const uint8_t packetSize);

    void turnOnConfigMode();
    void turnOnOperationMode();
    bool isInConfigMode();

    void receiveDio1Interrupt();

    void startTimeIR();

    bool isStartTimePassed();

    virtual String getProtocolName() = 0;

protected:
    virtual void onReceiveIR() = 0;
    virtual void onPreambleDetectedIR() = 0;
    virtual void onCRCerrorIR() = 0;

private:
    uint32_t startTime = -1;
};