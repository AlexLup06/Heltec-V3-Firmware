#include "MacController.h"

bool isInWaitMode = false;

static MacProtocol currentMac = MESH_ROUTER;
static MacSwitchCallback onMacSwitch = nullptr;
static MacFinishCallback onMacFinish = nullptr;

static bool waitingForNext = false;
static unsigned long switchTime = 0;
static unsigned long macStartTime = 0;
static bool isFirstTimeRunning = true;

static const unsigned long RUN_DURATION_MS = SIMULATION_DURATION_MIN * 60UL * 1000UL;
static const unsigned long SWITCH_DELAY_MS = MAC_PROTOCOL_SWITCH_DELAY_MIN * 60UL * 1000UL;

MacProtocol getCurrentMac() { return currentMac; }

void setMacSwitchCallback(MacSwitchCallback cb) { onMacSwitch = cb; }
void setMacFinishCallback(MacFinishCallback cb) { onMacFinish = cb; }

void markMacFinished()
{

    if (waitingForNext)
        return; // already waiting
    DEBUG_PRINTLN("MAC finished");
    waitingForNext = true;
    switchTime = millis() + SWITCH_DELAY_MS;
    isInWaitMode = true;

    if (onMacFinish)
        onMacFinish(currentMac);
}

void updateMacController()
{
    // initialize MAC controller
    if (isFirstTimeRunning)
    {
        macStartTime = millis();
        isInWaitMode = false;
        isFirstTimeRunning = false;
    }

    unsigned long now = millis();

    // Check if current MAC has reached its 10-min run duration
    if (!waitingForNext && (now - macStartTime >= RUN_DURATION_MS))
    {
        markMacFinished();
    }

    if (waitingForNext && (now >= switchTime))
    {
        DEBUG_PRINTLN("MAC switched");
        waitingForNext = false;
        currentMac = static_cast<MacProtocol>((currentMac + 1) % MAC_COUNT);
        macStartTime = now;
        isInWaitMode = false;

        if (onMacSwitch)
            onMacSwitch(currentMac);
    }
}