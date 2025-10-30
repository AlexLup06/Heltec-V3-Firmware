#include "shared/Common.h"
#include "WiFi.h"

void displayTask(void *pvParameters);

void commonSetup()
{
    esp_log_level_set("*", ESP_LOG_ERROR);

    Serial.setRxBufferSize(2048);
    Serial.setDebugOutput(false);
    Serial.begin(115200);

    while (!Serial)
    {
        ;
    }

    SPI.begin(9, 11, 10);
    SPI.setFrequency(4000000);
    delay(1000);

    loggerManager.init();
    loraDisplay.init();

    xTaskCreatePinnedToCore(
        displayTask,
        "DisplayTask",
        4096,
        &loraDisplay,
        1,
        NULL,
        0);

    radio.init(LORA_FREQUENCY, LORA_SPREADINGFACTOR, LORA_OUTPUT_POWER, LORA_BANDWIDTH);
    radio.setDio1Action(onDio1IR);
    radio.startReceive();
    radio.setOnCallback(incrementCb);

    setMacSwitchCallback(onMacChanged);
    setMacFinishCallback(onMacFinished);

    button.begin();
    button.onTripleClick(dumpFilesOverSerial);

    // setup MAC
    macCtx.radio = &radio;
    macCtx.loggerManager = &loggerManager;
    macCtx.loraDisplay = &loraDisplay;

    // macProtocol = &meshRouter;
    macProtocol = &csma;

    configurator.setCtx(&loraDisplay, &radio);

    meshRouter.init(macCtx);
    aloha.init(macCtx);
    cadAloha.init(macCtx);
    csma.init(macCtx);
}

void displayTask(void *pvParameters)
{
    LoRaDisplay *displayObj = (LoRaDisplay *)pvParameters;

    for (;;)
    {
        displayObj->loop();
        vTaskDelay(pdMS_TO_TICKS(500));
    }
}
