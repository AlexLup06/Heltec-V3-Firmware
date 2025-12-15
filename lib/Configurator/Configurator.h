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

struct Config
{
    uint64_t startTime_UNIX_MS = 0;
    uint8_t networkId = 0;
    uint8_t numberOfNodes = 0;
};

class Configurator
{
public:
    OperationMode operationMode = CONFIG;

    uint16_t nodeId;

    virtual ~Configurator() {}
    void handleConfigPacket(const uint8_t messageType, const uint8_t *rawPacket, const size_t packetSize);
    void setCtx(LoRaDisplay *_loraDisplay, SX1262Public *_radio, LoggerManager *_loggerManager, uint8_t _nodeId);

    void turnOnOperationMode();
    bool isInConfigMode();
    void handleConfigMode(); // runs in main handle()
    void handleDioInterrupt();
    void sendBroadcastConfig();
    void setNetworkTopology(bool forward);
    void confirmSetup(uint64_t _startTime);

private:
    LoRaDisplay *loraDisplay;
    LoggerManager *loggerManager;
    SX1262Public *radio;

    Config config;

    bool hasSentConfigMessage = false;
    bool settingNetworkTopology = true;

    uint8_t networkId = 0;
    uint8_t numberOfNodes = 0;

    uint64_t startTime_UNIX_MS = 0;
    int maxCW = 100;
    int slotTime = 50;
    int chosenSlot = 0;
    unsigned long cycleStartMs = 0;

    void setClockFromTimestamp(uint64_t unixTime_MS);
};