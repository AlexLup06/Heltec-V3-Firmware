#pragma once

#include <Arduino.h>
#include <RadioLib.h>
#include "LoggerManager.h"
#include "LoRaDisplay.h"
#include "RadioHandler.h"
#include "definitions.h"
#include "messages.h"
#include "RadioHandler.h"
#include "time.h"
#include "functions.h"

enum OperationMode
{
    CONFIG,
    OPERATIONAL,
};

class Configurator
{
public:
    OperationMode operationMode = CONFIG;

    uint16_t nodeId;

    virtual ~Configurator() {}
    void handleConfigPacket(const uint8_t messageType, const uint8_t *rawPacket, const size_t packetSize);
    void setCtx(LoRaDisplay *_loraDisplay, SX1262Public *_radio, uint8_t _nodeId);

    void turnOnOperationMode();
    bool isInConfigMode();
    void handleConfigMode(); // runs in main handle()
    void handleDioInterrupt();
    void receiveDio1Interrupt();
    void setStartTime(time_t startTime);
    void sendBroadcastConfig();

private:
    LoRaDisplay *loraDisplay;
    LoggerManager *loggerManager;
    SX1262Public *radio;

    uint16_t irqFlag = 0b0000000000000000;

    bool hasSentConfigMessage = false;

    uint32_t startTimeUnix = 0;
    int maxCW = 100;
    int slotTime = 50;
    int chosenSlot = 0;
    unsigned long cycleStartMs = 0;

    void setClockFromTimestamp(uint32_t unixTime);
};