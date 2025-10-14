#include "main.h"

MeshRouter *getMeshRouter() { return &meshRouter; }
void onButtonPress() { loraDisplay.nextScreen(); }
void onDio1IR() { macProtocol->receiveDio1Interrupt(); }

void onMacChanged(MacProtocol newMac)
{
    Serial.printf("Switched MAC protocol to %d\n", newMac);
    macProtocol->init();
}

void onMacFinished(MacProtocol finished)
{
    Serial.printf("Finished MAC %d — cooling down for 2 minutes\n", finished);
    macProtocol->finish();
    // TODO: assign new mac
    switch (finished)
    {
    case MacProtocol::MESH_ROUTER:
        // macProtocol=&;
        break;
    case MacProtocol::CAD_ALOHA:
        // macProtocol=&cadAloha;
        break;
    case MacProtocol::ALOHA:
        // macProtocol=&aloha;
        break;
    case MacProtocol::CSMA:
        // macProtocol=&csma;
        break;
    case MacProtocol::RS_IMITRA:
        // macProtocol=&rsimitra;
        break;
    case MacProtocol::MIRS:
        // macProtocol=&mirs;
        break;
    default:
        break;
    }
}

void setup()
{
    macProtocol = &meshRouter;
    meshRouter.radio = &radio;
    // csma.radio = &radio;
    // aloha.radio = &radio;
    // ..

    meshRouter.dataLogger = &dataLogger;
    // csma.dataLogger = &dataLogger;
    // aloha.dataLogger = &dataLogger;
    // ..

    esp_log_level_set("*", ESP_LOG_ERROR);

    // Setup Serial
    Serial.setRxBufferSize(2048);
    Serial.setDebugOutput(false);
    Serial.begin(115200);

    while (!Serial)
    {
        ; // wait for serial port to connect. Needed for native USB
    }

    attachInterrupt(0, onButtonPress, RISING);

    initRadio(LORA_FREQUENCY, LORA_SPREADINGFACTOR, LORA_OUTPUT_POWER, LORA_BANDWIDTH);
    radio.setDio1Action(onDio1IR);
    radio.startReceive();

    macProtocol->init();
    loraDisplay.initDisplay();

    setMacSwitchCallback(onMacChanged);
    setMacFinishCallback(onMacFinished);

    // Display LoRa communication network characteristics on SSD1306 screen.
    hostSerialHandlerParams.debugString = &loraDisplay.lastSerialChar;
    hostSerialHandlerParams.macProtocol = macProtocol;

    meshRouter.debugString = &loraDisplay.lastSerialChar;
    meshRouter.displayQueueLength = &loraDisplay.queueLength;

    // Start Host Serial Handler on Core 0
    xTaskCreatePinnedToCore(
        hostHandler,
        "hostHandler",
        10000,
        (void *)&hostSerialHandlerParams,
        0,
        &hostTask,
        0);
}

// FreeRTOS Task on Core 1
void loop()
{
    macProtocol->handle();
    updateMacController(macProtocol->isInConfigMode());

    if (lastScreenDraw + OLED_SCREEN_UPDATE_INTERVAL_MS < millis())
    {
        lastScreenDraw = millis();
        // TODO: print neighbour rssi table
    }

    if (hostSerialHandlerParams.ready)
    {
        meshRouter.ProcessFloodSerialPacket(hostSerialHandlerParams.serialFloodPacketHeader);
        hostSerialHandlerParams.ready = false;
    }
    yield();
}
