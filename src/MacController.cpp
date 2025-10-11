#include "MacController.h"

static MacProtocol currentMac = MESH_ROUTER;
static MacSwitchCallback onMacSwitch = nullptr;
static MacFinishCallback onMacFinish = nullptr;

static bool waitingForNext = false;
static unsigned long switchTime = 0;
static unsigned long macStartTime = 0;

static const unsigned long RUN_DURATION_MS = 10UL * 60UL * 1000UL; // 10 min
static const unsigned long SWITCH_DELAY_MS = 2UL * 60UL * 1000UL;  // 2 min

MacProtocol getCurrentMac() { return currentMac; }

void setMacSwitchCallback(MacSwitchCallback cb) { onMacSwitch = cb; }
void setMacFinishCallback(MacFinishCallback cb) { onMacFinish = cb; }

void markMacFinished()
{
    if (waitingForNext)
        return; // already waiting
    waitingForNext = true;
    switchTime = millis() + SWITCH_DELAY_MS;

    if (onMacFinish)
        onMacFinish(currentMac);
}

void updateMacController(bool isInConfigMode)
{
    unsigned long now = millis();

    // Check if current MAC has reached its 10-min run duration
    if (!waitingForNext && (now - macStartTime >= RUN_DURATION_MS))
    {
        markMacFinished();
    }

    // Check if the 2-min wait period has elapsed
    if (waitingForNext && !isInConfigMode)
    {
        waitingForNext = false;
        currentMac = static_cast<MacProtocol>((currentMac + 1) % MAC_COUNT);
        macStartTime = millis(); // reset timer for new MAC
        if (onMacSwitch)
            onMacSwitch(currentMac);
    }
}

void switchToMac(MacProtocol next)
{
    currentMac = next;
    macStartTime = millis();
    if (onMacSwitch)
        onMacSwitch(currentMac);
}
