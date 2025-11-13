#pragma once
#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include "functions.h"

struct NodeInfo
{
  uint16_t id;
  int rssi;
};

class LoRaDisplay
{
public:
  void init();
  void updateNode(uint16_t id, int rssi);
  void incrementSent();
  void incrementReceived();
  void render();

  void suspend();
  void resume();

  void loop();

private:
  static const uint8_t MAX_NODES = 32;
  static const uint8_t ROWS_PER_PAGE = 4;
  bool displayAvailable = false;

  bool displaySuspended = false;

  unsigned long messageReceivedCount = 0;
  unsigned long messageSentCount = 0;

  NodeInfo nodes[MAX_NODES];
  uint8_t nodeCount = 0;
  uint8_t pageIndex = 0;
  unsigned long lastPageSwitch = 0;

  Adafruit_SSD1306 display = Adafruit_SSD1306(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

  void drawHeader();
  void drawRows();
};
