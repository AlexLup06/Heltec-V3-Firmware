#pragma once
#include <Arduino.h>
#include "config.h"
#include "functions.h"
#include "MacBase.h"
#include "LoggerManager.h"
#include "LoRaDisplay.h"
#include "MessageSimulator.h"

enum MacProtocol
{
    ALOHA,
    CAD_ALOHA,
    CSMA,
    MESH_ROUTER,
    RS_MITRA,
    IRS_MITRA,
    MIRS,
    RS_MITRANR,
    MAC_COUNT
};

class MacController
{
public:
    using MacSwitchCallback = void (*)(MacProtocol newProtocol);
    using MacFinishCallback = void (*)(MacProtocol finishedProtocol);

    MacController();

    void init();

    void setMac(MacBase *m);
    void setCtx(LoggerManager *_loggerManager, MessageSimulator *_messageSimulator, LoRaDisplay *_loraDisplay, uint8_t _nodeId);

    void update();
    void markFinished();
    void collectData();

    void setSwitchCallback(MacSwitchCallback cb);
    void setFinishCallback(MacFinishCallback cb);

    uint8_t getRunCount() { return runCount; }
    void increaseRunCount();
    bool finishedAllRuns() { return runCount >= NUMBER_OF_RUNS; }

    MacProtocol getCurrent() const;
    const char *macIdToString(MacProtocol macProtocol) const;

    bool isInWaitMode() const;

private:
    MessageSimulator *messageSimulator;
    LoRaDisplay *loraDisplay;
    MacProtocol currentMac;
    MacSwitchCallback onMacSwitch;
    MacFinishCallback onMacFinish;

    MacBase *mac = nullptr;
    uint8_t runCount = 0;
    uint16_t missionMessagesPerMin[NUMBER_OF_RUNS];

    LoggerManager *loggerManager = nullptr;

    uint8_t nodeId;

    bool waitingForNext;
    bool firstRun;
    bool waitMode;
    bool collectedMidWait = false;

    unsigned long switchTime;
    unsigned long macStartTime;

    static constexpr unsigned long RUN_DURATION_MS = SIMULATION_DURATION_SEC * 1000UL;
    static constexpr unsigned long SWITCH_DELAY_MS = MAC_PROTOCOL_SWITCH_DELAY_SEC * 1000UL;
};
