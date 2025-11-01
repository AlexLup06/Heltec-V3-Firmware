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

    uint8_t macAdress[6];
    esp_read_mac(macAdress, ESP_MAC_WIFI_STA);
    nodeId = macAdress[5];

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

    macController.setSwitchCallback(onMacChanged);
    macController.setFinishCallback(onMacFinished);

    button.begin();
    button.onTripleClick(dumpFilesOverSerial);

    // setup MAC
    macCtx.radio = &radio;
    macCtx.loggerManager = &loggerManager;
    macCtx.loraDisplay = &loraDisplay;

    macProtocol = &aloha;
    macController.setMac(macProtocol);

    configurator.setCtx(&loraDisplay, &radio, nodeId);

    meshRouter.init(macCtx, nodeId);
    aloha.init(macCtx, nodeId);
    cadAloha.init(macCtx, nodeId);
    csma.init(macCtx, nodeId);
    rsmitra.init(macCtx, nodeId);
    irsmitra.init(macCtx, nodeId);
    mirs.init(macCtx, nodeId);
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
