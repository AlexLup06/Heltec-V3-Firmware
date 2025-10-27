#pragma once
#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include "functions.h"

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
#define SCREEN_ADDRESS 0x3C

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
  void render();

  void loop();

private:
  static const uint8_t MAX_NODES = 32;
  static const uint8_t ROWS_PER_PAGE = 6;
  static const uint16_t PAGE_INTERVAL_MS = 5000; // 5 s

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
