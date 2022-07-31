#include <config.h>
#include <main.h>
#include <lib/LoRa.h>
#include <mesh/loranode.h>
#include <mesh/loradisplay.h>
#include <HostHandler.h>
#include <Arduino.h>
#include <mesh/MeshRouter.h>

#ifdef USE_OTA_UPDATE_CHECKING
#include <ota/devota.h>
#endif

// Toggle Serial Debug
// #define DEBUG_LORA_SERIAL




unsigned long lastScreenDraw = 0;

void onButtonPress();

static double_t UPDATE_STATE = -1;

hw_timer_t *timer = NULL;

void receiveInterrupt(int size) {
    meshRouter.OnReceiveIR(size);
}

void cadInterrupt() {
    meshRouter.cad = true;
}

#ifdef USE_OTA_UPDATE_CHECKING
/**
* Start OTA Handler on Core 0
**/
void activateOTA() {
    xTaskCreatePinnedToCore(
            setupOta,
            "setupOta",
            10000,
            (void *) &UPDATE_STATE,
            0,
            &otaTask,
            0);
}
#endif


void setup() {
#ifdef USE_OTA_UPDATE_CHECKING
    activateOTA();
#endif
    esp_log_level_set("*", ESP_LOG_ERROR);

    // Setup Serial
    Serial.setRxBufferSize(2048);
    Serial.setDebugOutput(false);
    Serial.begin(115200);

    while (!Serial) { ; // wait for serial port to connect. Needed for native USB
    }
#ifdef DEBUG_LORA_SERIAL
    uint8_t mac[6];
    esp_read_mac(mac, ESP_MAC_WIFI_STA);
    char macHexStr[18] = {0};
    sprintf(macHexStr, "%02X:%02X:%02X:%02X:%02X:%02X", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);

    Serial.println(F("SSD1306 Starting..."));
    Serial.println("Board MAC: " + String(macHexStr));
    Serial.println("Build Revision: " + String(BUILD_REVISION));
#endif

    attachInterrupt(0, onButtonPress, RISING);

    // Init Lora Hardware
#ifdef DEBUG_LORA_SERIAL
    Serial.println("Init LoRa Hardware");
#endif
    loraNode.initLora();

    // Init the Mesh Node
#ifdef DEBUG_LORA_SERIAL
    Serial.println("Init Mesh Node");
#endif
    meshRouter.initNode();


    // Init Display
#ifdef DEBUG_LORA_SERIAL
    Serial.println("Init Display");
#endif
    loraDisplay.initDisplay();


    LoRa.onReceive(receiveInterrupt);
    LoRa.onCad(cadInterrupt);

    hostSerialHandlerParams.debugString = &loraDisplay.lastSerialChar;
    meshRouter.debugString = &loraDisplay.lastSerialChar;
    meshRouter.displayQueueLength = &loraDisplay.queueLength;
    loraDisplay.waitTime = &meshRouter.blockSendUntil;
    loraDisplay.receivedBytes = &meshRouter.receivedBytes;

    loraDisplay.printRoutingTableScreen(meshRouter.routingTable, meshRouter.totalRoutes, meshRouter.NodeID,
                                        UPDATE_STATE);

    /**
     * Start Host Serial Handler on Core 0
    **/
    xTaskCreatePinnedToCore(
            hostHandler,
            "hostHandler",
            10000,
            (void *) &hostSerialHandlerParams,
            0,
            &hostTask,
            0);
}

void onButtonPress() {
    loraDisplay.nextScreen();
}

// FreeRTOS Task on Core 1
void loop() {
    // check lora interrupt raised
    if(LoRa.DIO_RISE){
        LoRa.DIO_RISE = false;
        LoRa.handleDio0Rise();
    }
    // Handle Route Loop
    meshRouter.handle();

    if (lastScreenDraw + OLED_SCREEN_UPDATE_INTERVAL_MS < millis()) {
        lastScreenDraw = millis();
        loraDisplay.printRoutingTableScreen(meshRouter.routingTable, meshRouter.totalRoutes, meshRouter.NodeID,
                                            UPDATE_STATE);
    }

    if (UPDATE_STATE > 0 && meshRouter.OPERATING_MODE != OPERATING_MODE_UPDATE_IDLE) {
        LoRa.idle();
        meshRouter.OPERATING_MODE = OPERATING_MODE_UPDATE_IDLE;

        // Wir brauchen den Serial Host task nicht mehr
        vTaskDelete(hostTask);
    }

    if (hostSerialHandlerParams.ready) {
        // Paket from Serial is ready
        // *hostSerialHandlerParams.debugString = "PROCESSED";
        meshRouter.ProcessFloodSerialPaket(hostSerialHandlerParams.serialFloodPaketHeader);
        hostSerialHandlerParams.ready = false;
    }
    yield();
}
