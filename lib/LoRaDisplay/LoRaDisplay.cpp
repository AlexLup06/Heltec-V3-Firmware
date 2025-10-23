#include "LoRaDisplay.h"

void LoRaDisplay::init()
{
  Wire.setPins(4, 15); // Heltec V3 SDA/SCL
  if (!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS))
  {
    Serial.println(F("SSD1306 init failed"));
    for (;;)
    {
    }
  }
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.display();
}

void LoRaDisplay::updateNode(uint8_t id, int rssi)
{
  for (uint8_t i = 0; i < nodeCount; i++)
  {
    if (nodes[i].id == id)
    {
      nodes[i].rssi = rssi;
      render();
      return;
    }
  }
  if (nodeCount < MAX_NODES)
  {
    nodes[nodeCount++] = {id, rssi};
  }
  else
  {
    // FIFO overwrite
    for (uint8_t i = 1; i < MAX_NODES; i++)
      nodes[i - 1] = nodes[i];
    nodes[MAX_NODES - 1] = {id, rssi};
  }
  render();
}

void LoRaDisplay::drawHeader()
{
  display.setCursor(0, 0);
  display.print("Node");
  display.setCursor(48, 0);
  display.print("RSSI");
  display.drawLine(0, 10, SCREEN_WIDTH, 10, SSD1306_WHITE);
}

void LoRaDisplay::drawRows()
{
  uint8_t totalPages = (nodeCount + ROWS_PER_PAGE - 1) / ROWS_PER_PAGE;
  uint8_t start = pageIndex * ROWS_PER_PAGE;
  uint8_t end = min<uint8_t>(start + ROWS_PER_PAGE, nodeCount);

  for (uint8_t i = start; i < end; i++)
  {
    int y = 12 + (i - start) * 9;
    display.setCursor(0, y);
    display.printf("%3d", nodes[i].id);
    display.setCursor(48, y);
    display.printf("%4d", nodes[i].rssi);
  }

  // --- Bottom line ---
  display.setCursor(0, SCREEN_HEIGHT - 8);
  
  uint32_t total = ESP.getHeapSize();
  uint32_t free = ESP.getFreeHeap();
  uint32_t used = total - free;

  display.setCursor(0, SCREEN_HEIGHT - 8);
  display.printf("RAM:%lu/%luK", used / 1024, total / 1024);

  if (totalPages > 1)
  {
    display.setCursor(SCREEN_WIDTH - 30, SCREEN_HEIGHT - 8);
    display.printf("%d/%d", pageIndex + 1, totalPages);
  }
}

void LoRaDisplay::render()
{
  display.clearDisplay();
  drawHeader();
  drawRows();
  display.display();
}

void LoRaDisplay::loop()
{
  unsigned long now = millis();
  uint8_t totalPages = (nodeCount + ROWS_PER_PAGE - 1) / ROWS_PER_PAGE;
  if (totalPages > 1 && now - lastPageSwitch >= PAGE_INTERVAL_MS)
  {
    pageIndex = (pageIndex + 1) % totalPages;
    lastPageSwitch = now;
    render();
  }
}
