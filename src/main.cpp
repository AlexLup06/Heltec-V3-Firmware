#include <Arduino.h>

#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <SPI.h> // include libraries
#include <lib/LoRa.h>
#include <mesh/MeshRouter.h>
#include <ota/devota.h>
#include <assets/whale.h>

#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels

#define OLED_RESET 16       // Reset pin # (or -1 if sharing Arduino reset pin)
#define SCREEN_ADDRESS 0x3C ///< See datasheet for Address; 0x3D for 128x64, 0x3C for 128x32

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
MeshRouter meshRouter;
TaskHandle_t otaTask;

const long frequency = 868000000; // LoRa Frequency

const int csPin = 18;    // LoRa radio chip select
const int resetPin = 14; // LoRa radio reset
const int irqPin = 35;

int received = 0;
int sent = 0;
bool sendNext = false;
bool drawNext = false;
String lastMessage = "";
int lastRssi = 0;
unsigned long lastSendTime = 0;
bool autosend = false;

long recSize = 0;
long sendSize = 0;
void onReceive(int packetSize);
void onTxDone();
void LoRa_txMode();
void sendStuff();
void LoRa_rxMode();
void LoRa_sendMessage(String message);
void tx();
void printScreen();
void onTimer();
void drawCentreString(const char *buf, int x, int y);
static double_t UPDATE_STATE = -1;

long startTime = 0;
long endTime = 0;
long lastRec = 0;

hw_timer_t *timer = NULL;
uint8_t buf[255];
uint8_t recBuf[255];

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

  lastSendTime = millis();
  Serial.begin(115200);

  for (int i = 0; i < 255; i++)
  {
    buf[i] = i;
  }

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
  // put your setup code here, to run once:
  Wire.setPins(4, 15);

  if (!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS))
  {
    Serial.println(F("SSD1306 allocation failed"));
    while (true)
      ;
  }
  delay(200);

  SPI.begin(SCK, MISO, MOSI, SS);
  LoRa.setPins(SS, RST_LoRa, DIO0);

  if (!LoRa.begin(frequency, false))
  {
    Serial.println("LoRa init failed. Check your connections.");
    while (true)
      ;
  }
  //LoRa.setTxPower(15, 0);
  //LoRa.setTxPower(20, 0x80);
  LoRa.setSignalBandwidth(125E3);
  LoRa.setSpreadingFactor(7);

  // Eigenes Sync-Word setzen, damit Pakete anderer LoRa Netzwerke ignoriert werden
  LoRa.setSyncWord(0x12);

  attachInterrupt(0, sendStuff, RISING);

  display.clearDisplay();
  display.drawBitmap(24, 0, whale, 80, 62, WHITE);
  display.display();
  delay(1000);

  // Clear the buffer
  printScreen();

  // Init the Mesh Node
  meshRouter.initNode();
}

void sendStuff()
{
  //drawNext = true;
  sendSize = 0;
  recSize = 0;
  sendNext = true;
  startTime = millis();
}

void drawCentreString(const char *buf, int x, int y)
{
  int16_t x1, y1;
  uint16_t w, h;
  display.getTextBounds(buf, x, y, &x1, &y1, &w, &h); //calc width of new string
  display.setCursor(x - w / 2, y);
  display.print(buf);
}

void printUpdateScreen()
{
  display.println("Download Update..");

  int barWidth = UPDATE_STATE / 100 * 128;
  display.fillRect(0, 25, barWidth, 8, WHITE);

  String drawText = String(UPDATE_STATE, 2) + "%";

  char buf[10];
  drawText.toCharArray(buf, 10);

  drawCentreString(buf, 64, 40);

  display.display();
}
void printScreen()
{

  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(0, 10);

  if (UPDATE_STATE > -1)
  {
    printUpdateScreen();
    return;
  }

  float transfertime = (endTime - startTime) / 1000.0;
  if (transfertime == 0)
  {
    display.println("B/s: NaN");
  }
  else
  {
    float byterate = sendSize / transfertime;
    display.println("B/s: " + String(byterate) + " " + String(transfertime) + " s");
  }

  display.setCursor(0, 20);
  display.println(lastMessage);
  display.setCursor(0, 30);
  display.println("RSSI: " + String(lastRssi));
  display.setCursor(0, 40);
  display.println("Rec: " + String(received));
  display.setCursor(0, 50);
  display.println("Sent: " + String(sent));
  display.display();
}

void checkForSendPacket()
{

  if (sendNext)
  {
    sendNext = false;

    LoRa.beginPacket();
    NodeIdAnnounce_t *paket = (NodeIdAnnounce_t *)malloc(sizeof(NodeIdAnnounce_t));

    paket->messageType = MESSAGE_TYPE_NODE_ANNOUNCE;
    paket->nodeId = 0;
    memcpy(paket->deviceMac, meshRouter.macAdress, 6);

    LoRa.write((uint8_t *)paket, 255);
    LoRa.endPacket();
    free(paket);

    lastRec = millis();
    sendSize += 255;

    // Wait for Receiver Availability
    delay(10);
  }
}
void receiveAndPrintPacket()
{
  int packetSize = LoRa.parsePacket();

  if (packetSize)
  {
    uint8_t *receiveBuffer = (uint8_t *)malloc(packetSize);

    LoRa.readBytes(receiveBuffer, 255);

    meshRouter.OnReceivePacket(receiveBuffer);
  }
}

// FreeRTOS Task on Core 1
void loop()
{
  if (UPDATE_STATE > -1)
  {
    printScreen();
    delay(333);
  }
  else
  {
    // sends a packet, when button is pressed
    checkForSendPacket();
    //
    receiveAndPrintPacket();
  }
}
