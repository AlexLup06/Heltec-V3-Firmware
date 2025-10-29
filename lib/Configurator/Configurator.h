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
    void handleConfigPacket(const uint8_t messageType, const uint8_t *rawPacket, const size_t packetSize, int rssi);
    void setCtx(LoRaDisplay *_loraDisplay, LoggerManager *_loggerManager, SX1262Public *_radio);

    void turnOnOperationMode();
    bool isInConfigMode();
    void handleConfigMode(); // runs in main handle()
    void setIsMaster(bool _isMaster) { isMaster = _isMaster; }
    void receiveDio1Interrupt();
    void setStartTime(time_t startTime);
    void sendTimeSync();
    void sendConfigMessage(time_t startTime);

private:
    LoRaDisplay *loraDisplay;
    LoggerManager *loggerManager;
    SX1262Public *radio;

    bool isMaster = false;

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
    int maxCW = 100;
    int slotTime = 50;
    int chosenSlot = 0;
    int nextMinuteTarget = -1;
    unsigned long backoffTime = 0;
    unsigned long backoffTimeConfig = 0;
    unsigned long cycleStartMs = 0;

    void sendNodeIndicatorMessage();
    void setClockFromTimestamp(uint32_t unixTime);
};