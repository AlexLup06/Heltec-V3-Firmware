#include "shared/Common.h"
#include "WiFi.h"
#include "filesystemOperations.h"

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

    randomSeed(nodeId);

    loraDisplay.init();

    if (!LittleFS.begin(true))
    {
        Serial.printf("Failed to mount LittleFS!");
        return;
    }
    Serial.printf("LittleFS mounted successfully");

    radio.init(LORA_FREQUENCY, LORA_SPREADINGFACTOR, LORA_OUTPUT_POWER, LORA_BANDWIDTH);
    radio.startReceive();

    button.begin(&radio);
    button.onQuadrupleClick(dumpFilesOverSerial);
    button.onSingleLongClick(deleteAllBinFilesAndDirs);

    // setup MAC
    macCtx.radio = &radio;
    macCtx.loggerManager = &loggerManager;
    macCtx.loraDisplay = &loraDisplay;

    macProtocol = &aloha;
    macController.setMac(macProtocol);
    macController.setCtx(&loggerManager, &messageSimulator, &loraDisplay, nodeId);
    macController.setSwitchCallback(onMacChanged);
    macController.setFinishCallback(onMacFinished);

    configurator.setCtx(&loraDisplay, &radio, &loggerManager, nodeId);

    meshRouter.init(macCtx, nodeId);
    aloha.init(macCtx, nodeId);
    // cadAloha.init(macCtx, nodeId);
    csma.init(macCtx, nodeId);
    rsmitra.init(macCtx, nodeId);
    irsmitra.init(macCtx, nodeId);
    mirs.init(macCtx, nodeId);
    rsmitranr.init(macCtx, nodeId);

    xTaskCreatePinnedToCore(
        buttonTask,
        "Button",
        4096,
        NULL,
        3,
        NULL,
        1);

    xTaskCreatePinnedToCore(
        macTask,
        "MAC",
        4096,
        NULL,
        6,
        NULL,
        1);

    xTaskCreatePinnedToCore(
        radioIrqTask,
        "RadioIRQ",
        4096,
        &radio,
        10,
        NULL,
        1);
}