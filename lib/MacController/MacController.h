#pragma once
#include <Arduino.h>
#include "config.h"
#include "functions.h"
#include "MacBase.h"

enum MacProtocol
{
    ALOHA,
    CAD_ALOHA,
    CSMA,
    MESH_ROUTER,
    RS_MITRA,
    IRS_MITRA,
    MIRS,
    MAC_COUNT
};

class MacController
{
public:
    using MacSwitchCallback = void (*)(MacProtocol newProtocol);
    using MacFinishCallback = void (*)(MacProtocol finishedProtocol);

    MacController();

    void setMac(MacBase *m);
    void update();
    void markFinished();

    void setSwitchCallback(MacSwitchCallback cb);
    void setFinishCallback(MacFinishCallback cb);

    uint8_t getRunCount() { return runCount; }
    void increaseRunCount() { runCount++; }
    bool finishedAllRuns() { return runCount >= NUMBER_OF_RUNS; }

    MacProtocol getCurrent() const;
    String macIdToString(MacProtocol macProtocol) const;

    bool isInWaitMode() const;

private:
    MacProtocol currentMac;
    MacSwitchCallback onMacSwitch;
    MacFinishCallback onMacFinish;

    MacBase *mac;

    bool waitingForNext;
    bool firstRun;
    bool waitMode;

    unsigned long switchTime;
    unsigned long macStartTime;

    uint8_t runCount = 0;

    static constexpr unsigned long RUN_DURATION_MS = SIMULATION_DURATION_SEC * 1000UL;
    static constexpr unsigned long SWITCH_DELAY_MS = MAC_PROTOCOL_SWITCH_DELAY_SEC * 1000UL;
};
