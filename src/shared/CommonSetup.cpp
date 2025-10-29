#include "shared/Common.h"

void commonSetup(bool isMaster)
{
    macCtx.radio = &radio;
    macCtx.loggerManager = &loggerManager;
    macCtx.loraDisplay = &loraDisplay;
    macCtx.isMaster = isMaster;

    macProtocol = &meshRouter;

    esp_log_level_set("*", ESP_LOG_ERROR);

    Serial.setRxBufferSize(2048);
    Serial.setDebugOutput(false);
    Serial.begin(115200);

    while (!Serial)
    {
        ;
    }

    configurator.setCtx(&loraDisplay, &loggerManager, &radio);

    radio.init(LORA_FREQUENCY, LORA_SPREADINGFACTOR, LORA_OUTPUT_POWER, LORA_BANDWIDTH);
    radio.setDio1Action(onDio1IR);
    radio.startReceive();

    loggerManager.init();
    loraDisplay.init();

    setMacSwitchCallback(onMacChanged);
    setMacFinishCallback(onMacFinished);

    button.begin();
    button.onTripleClick(dumpFilesOverSerial);

    meshRouter.init(macCtx);
    aloha.init(macCtx);
}
