#include <Arduino.h>

#include <lib/LoRa.h>
#include <mesh/MeshRouter.h>
#include <ota/devota.h>
#include <mesh/loranode.h>
#include <mesh/loradisplay.h>

MeshRouter meshRouter;
LoraNode loraNode;
LoraDisplay loraDisplay;
TaskHandle_t otaTask;

bool sendNext = false;
unsigned long lastScreenDraw = 0;

void sendStuff();
static double_t UPDATE_STATE = -1;

hw_timer_t *timer = NULL;

void receiveInterrupt(int size)
{
  meshRouter.OnReceiveIR(size);
}

void setup()
{

  /**
  * Start OTA Handler on Core 0 
  **/
  xTaskCreatePinnedToCore(
      setupOta,
      "setupOta",
      10000,
      (void *)&UPDATE_STATE,
      0,
      &otaTask,
      0);

  // Setup Serial
  Serial.begin(115200);

  while (!Serial)
  {
    ; // wait for serial port to connect. Needed for native USB
  }

  uint8_t mac[6];
  esp_read_mac(mac, ESP_MAC_WIFI_STA);
  char macHexStr[18] = {0};
  sprintf(macHexStr, "%02X:%02X:%02X:%02X:%02X:%02X", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);

  Serial.println(F("SSD1306 Starting..."));
  Serial.println("Board MAC: " + String(macHexStr));
  Serial.println("Build Revision: " + String(BUILD_REVISION));

  attachInterrupt(0, sendStuff, RISING);

  // Init Lora Hardware
  Serial.println("Init LoRa Hardware");
  loraNode.initLora();

  // Init the Mesh Node
  Serial.println("Init Mesh Node");
  meshRouter.initNode();

  // Init Display
  Serial.println("Init Display");
  loraDisplay.initDisplay();
  loraDisplay.printRoutingTableScreen(meshRouter.routingTable, meshRouter.totalRoutes, meshRouter.NodeID, UPDATE_STATE);

  LoRa.onReceive(receiveInterrupt);
}

void sendStuff()
{
  //drawNext = true;
  sendNext = true;
}

void checkForSendPacket()
{

  if (sendNext)
  {
    sendNext = false;

    meshRouter.announceNodeId(0);
    LoRa.receive();
  }
}
// FreeRTOS Task on Core 1
void loop()
{
  // Handle Route Loop

  meshRouter.handle();
  checkForSendPacket();

  if (lastScreenDraw + 333 < millis())
  {
    lastScreenDraw = millis();
    loraDisplay.printRoutingTableScreen(meshRouter.routingTable, meshRouter.totalRoutes, meshRouter.NodeID, UPDATE_STATE);
  }

  if (UPDATE_STATE > 0 && meshRouter.OPERATING_MODE != OPERATING_MODE_UPDATE_IDLE)
  {
    LoRa.idle();
    meshRouter.OPERATING_MODE = OPERATING_MODE_UPDATE_IDLE;
  }

  yield();
}
