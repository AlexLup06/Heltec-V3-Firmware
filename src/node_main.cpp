#include <Arduino.h>
#include <RadioLib.h>
#include <config.h>

#include <LoRaDisplay.h>
#include <MeshRouter.h>
#include <Aloha.h>
#include <RadioHandler.h>
#include <MacController.h>
#include <OperationalBase.h>
#include <DataLogger.h>
#include <MessageSimulator.h>

// --- Globals ---
LoRaDisplay loraDisplay;
SX1262Public radio = SX1262Public(new Module(LORA_CS, LORA_DIO1, LORA_RST, LORA_BUSY));
DataLogger dataLogger;
MessageSimulator messageSimulator;

int currentMac = MacProtocol::MESH_ROUTER;
MacBase *macProtocol = nullptr;
MeshRouter meshRouter;
Aloha aloha;

unsigned long lastScreenDraw = 0;
static double_t UPDATE_STATE = -1;
hw_timer_t *timer = NULL;

void onDio1IR() { macProtocol->receiveDio1Interrupt(); }
void onMacChanged(MacProtocol newMac)
{
    DEBUG_LORA_SERIAL("Switched MAC protocol to %d\n", newMac);
    macProtocol->init();
}

void onMacFinished(MacProtocol finished)
{
    DEBUG_LORA_SERIAL("Finished MAC %d — cooling down for 2 minutes\n", finished);
    macProtocol->finish();
    // TODO: assign new mac
    switch (finished)
    {
    case MacProtocol::MESH_ROUTER:
        // macProtocol=&cadAloha;
        break;
    case MacProtocol::CAD_ALOHA:
        macProtocol = &aloha;
        break;
    case MacProtocol::ALOHA:
        // macProtocol=&csma;
        break;
    case MacProtocol::CSMA:
        // macProtocol=&rsimitra;
        break;
    case MacProtocol::RS_IMITRA:
        // macProtocol=&mirs;
        break;
    case MacProtocol::MIRS:
        // we just simulated everyhting for the topology
        break;
    default:
        break;
    }
}

void setup()
{
    macProtocol = &meshRouter;
    meshRouter.assignRadio(&radio);
    // csma.radio = &radio;
    // aloha.radio = &radio;
    // ..

    meshRouter.dataLogger = &dataLogger;
    // csma.dataLogger = &dataLogger;
    // aloha.dataLogger = &dataLogger;
    // ..

    meshRouter.loraDisplay = &loraDisplay;
    // csma.loraDisplay = &loraDisplay;
    // aloha.loraDisplay = &loraDisplay;
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

    macProtocol->initRadio(LORA_FREQUENCY, LORA_SPREADINGFACTOR, LORA_OUTPUT_POWER, LORA_BANDWIDTH);
    radio.setDio1Action(onDio1IR);
    radio.startReceive();

    macProtocol->init();
    loraDisplay.init();

    setMacSwitchCallback(onMacChanged);
    setMacFinishCallback(onMacFinished);
}

void loop()
{
    loraDisplay.loop();
    macProtocol->handle();
    updateMacController(macProtocol->isInConfigMode());
    messageSimulator.simulateMessages();

    if (lastScreenDraw + OLED_SCREEN_UPDATE_INTERVAL_MS < millis())
    {
        lastScreenDraw = millis();
        // TODO: print neighbour rssi table
    }

    if (messageSimulator.packetReady)
    {
        meshRouter.handleUpperPacket(messageSimulator.messageToSend);
        messageSimulator.packetReady = false;
    }

    yield();
}
