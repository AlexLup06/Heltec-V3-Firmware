#ifndef OPERATIONALBASE_H
#define OPERATIONALBASE_H

#pragma once
#include <Arduino.h>
#include <RadioLib.h>
#include "helpers/definitions.h"
#include "helpers/Messages.h"

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

#endif