#include "MacController.h"

MacController::MacController()
    : currentMac(ALOHA),
      onMacSwitch(nullptr),
      onMacFinish(nullptr),
      waitingForNext(false),
      firstRun(true),
      waitMode(false),
      switchTime(0),
      macStartTime(0)
{
}

void MacController::setMac(MacBase *m)
{
    mac = m;
}

void MacController::setSwitchCallback(MacSwitchCallback cb)
{
    onMacSwitch = cb;
}

void MacController::setFinishCallback(MacFinishCallback cb)
{
    onMacFinish = cb;
}

MacProtocol MacController::getCurrent() const
{
    return currentMac;
}

bool MacController::isInWaitMode() const
{
    return waitMode;
}

void MacController::markFinished()
{
    if (waitingForNext)
        return;

    DEBUG_PRINTLN("MAC finished");
    waitingForNext = true;
    switchTime = millis() + SWITCH_DELAY_MS;
    waitMode = true;

    if (onMacFinish)
        onMacFinish(currentMac);
}

void MacController::update()
{
    unsigned long now = millis();

    if (firstRun)
    {
        DEBUG_PRINTLN("First MACController update");
        macStartTime = now;
        waitMode = false;
        firstRun = false;
        mac->initRun();
    }

    if (!waitingForNext && (now - macStartTime >= RUN_DURATION_MS))
    {
        markFinished();
    }

    if (waitingForNext && now >= switchTime)
    {
        DEBUG_PRINTLN("MAC switched");
        waitingForNext = false;
        currentMac = static_cast<MacProtocol>((currentMac + 1) % MAC_COUNT);
        macStartTime = now;
        waitMode = false;

        if (onMacSwitch)
            onMacSwitch(currentMac);
    }
}

String MacController::macIdToString(MacProtocol macProtocol) const
{
    switch (macProtocol)
    {
    case MESH_ROUTER:
        return "LoRaMeshRouter";
    case CAD_ALOHA:
        return "LoRaCADAloha";
    case ALOHA:
        return "LoRaAloha";
    case CSMA:
        return "LoRaCSMA";
    case RS_MITRA:
        return "LoRaRSMitra";
    case IRS_MITRA:
        return "LoRaIRSMitra";
    case MIRS:
        return "LoRaMIRS";
    default:
        return "InvalidMAC";
    }
}
