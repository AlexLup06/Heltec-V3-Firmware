#include "MacController.h"

MacController::MacController()
    : currentMac(ALOHA),
      onMacSwitch(nullptr),
      onMacFinish(nullptr),
      waitingForNext(false),
      firstRun(true),
      waitMode(false),
      switchTime(0),
      macStartTime(0),
      nodeId(0)
{
}

void MacController::setMac(MacBase *m)
{
    mac = m;
}

void MacController::setCtx(LoggerManager *_loggerManager, MessageSimulator *_messageSimulator, LoRaDisplay *_loraDisplay, uint8_t _nodeId)
{
    loggerManager = _loggerManager;
    messageSimulator = _messageSimulator;
    loraDisplay = _loraDisplay;
    nodeId = _nodeId;
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

void MacController::collectData()
{
    loggerManager->saveAll();
    loggerManager->clearAll();
    loggerManager->saveCounters();
    loggerManager->resetCounters();
}

void MacController::markFinished()
{
    if (waitingForNext)
        return;

    Serial.printf("\n\nFinished MAC %s — cooling down for 2 minutes\n\n", macIdToString(currentMac));

    waitingForNext = true;
    switchTime = millis() + SWITCH_DELAY_MS;
    waitMode = true;

    if ((currentMac + 1) % MAC_COUNT == 0)
    {
        int nextMission = 60 / missionMessagesPerMin[runCount];
        messageSimulator->setTimeToNextMission(nextMission);
    }

    if (onMacFinish)
        onMacFinish(currentMac);
}

void MacController::init()
{
    Serial.println("[MacController] First MACController update");

    missionMessagesPerMin[0] = 2;
    missionMessagesPerMin[1] = 10;
    missionMessagesPerMin[2] = 15;
    missionMessagesPerMin[3] = 30;
    missionMessagesPerMin[4] = 60;
    missionMessagesPerMin[5] = 120;
    missionMessagesPerMin[6] = 240;

    unsigned long now = millis();
    macStartTime = now - 10;
    waitMode = false;
    firstRun = false;
    mac->initRun();

    delay(1000);
    loggerManager->setMetadata(runCount, mac->getProtocolName(), missionMessagesPerMin[0]);
    messageSimulator->setTimeToNextMission(missionMessagesPerMin[0]);
    loggerManager->init(nodeId);
}

void MacController::increaseRunCount()
{
    runCount++;
    loraDisplay->setRunNumber(runCount);
}

void MacController::update()
{
    unsigned long now = millis();

    if (firstRun)
    {
        init();
    }

    if (!waitingForNext && (now - macStartTime >= RUN_DURATION_MS))
    {
        markFinished();
    }

    if (waitingForNext)
    {

        unsigned long startDataCollection = switchTime - (SWITCH_DELAY_MS - 3000);

        if (!collectedMidWait && now >= startDataCollection)
        {
            DEBUG_PRINTLN("Mid-wait: collecting data...");
            collectData();
            collectedMidWait = true;

            loraDisplay->loop();
        }

        if (now >= switchTime)
        {
            waitingForNext = false;
            currentMac = static_cast<MacProtocol>((currentMac + 1) % MAC_COUNT);
            macStartTime = now;
            waitMode = false;
            collectedMidWait = false;
            loggerManager->setMetadata(runCount, mac->getProtocolName(), missionMessagesPerMin[runCount]);
            loggerManager->updateFilename();

            if (onMacSwitch)
                onMacSwitch(currentMac);
        }
    }
}

const char *MacController::macIdToString(MacProtocol macProtocol) const
{
    switch (macProtocol)
    {
    case MESH_ROUTER:
        return "LoRaMeshRouter";
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
    case RS_MITRANR:
        return "LoRaRSMiTraNR";
    case RS_MITRANAV:
        return "LoRaRSMiTraNAV";
    default:
        return "InvalidMAC";
    }
}
