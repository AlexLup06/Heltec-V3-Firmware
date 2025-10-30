#include "LoRaDisplay.h"

void LoRaDisplay::init()
{
  pinMode(36, OUTPUT);
  digitalWrite(36, LOW);
  delay(100);

  pinMode(21, OUTPUT);
  digitalWrite(21, LOW);
  delay(20);
  digitalWrite(21, HIGH);
  delay(20);

  Wire.begin(17, 18);
  Wire.setClock(100000);

  if (!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS))
  {
    DEBUG_PRINTLN("SSD1306 init failed");
    for (;;)
    {
    }
  }

  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  display.println("OLED Ready");
  display.display();

  messageReceivedCount = 0;
  messageSentCount = 0;
  pageIndex = 0;
  lastPageSwitch = millis();
}

void LoRaDisplay::updateNode(uint16_t id, int rssi)
{
  messageReceivedCount++;

  for (uint8_t i = 0; i < nodeCount; i++)
  {
    if (nodes[i].id == id)
    {
      nodes[i].rssi = rssi;
      return;
    }
  }

  if (nodeCount < MAX_NODES)
  {
    nodes[nodeCount++] = {id, rssi};
  }
  else
  {
    for (uint8_t i = 1; i < MAX_NODES; i++)
      nodes[i - 1] = nodes[i];
    nodes[MAX_NODES - 1] = {id, rssi};
  }
}

void LoRaDisplay::incrementSent()
{
  messageSentCount++;
}

void LoRaDisplay::incrementReceived()
{
  messageReceivedCount++;
}

void LoRaDisplay::drawHeader()
{
  display.setCursor(0, 0);
  display.print("Node");
  display.setCursor(48, 0);
  display.print("RSSI");
  display.drawLine(0, 10, SCREEN_WIDTH, 10, SSD1306_WHITE);

  int16_t x, y;
  uint16_t w, h;
  display.getTextBounds("Rx:0000", 0, 0, &x, &y, &w, &h);
  display.setCursor(SCREEN_WIDTH - w - 2, 0);
  display.printf("Rx:%lu", messageReceivedCount);
  display.drawLine(SCREEN_WIDTH - w - 6, 0, SCREEN_WIDTH - w - 6, 10, SSD1306_WHITE);
}

void LoRaDisplay::drawRows()
{
  uint8_t totalPages = (nodeCount + ROWS_PER_PAGE - 1) / ROWS_PER_PAGE;
  uint8_t start = pageIndex * ROWS_PER_PAGE;
  uint8_t end = min<uint8_t>(start + ROWS_PER_PAGE, nodeCount);

  for (uint8_t i = start; i < end; i++)
  {
    int y = 14 + (i - start) * 9;
    display.setCursor(0, y);
    display.printf("%3d", nodes[i].id);
    display.setCursor(48, y);
    display.printf("%4d", nodes[i].rssi);
  }

  display.drawLine(0, SCREEN_HEIGHT - 12, SCREEN_WIDTH, SCREEN_HEIGHT - 12, SSD1306_WHITE);

  display.setCursor(0, SCREEN_HEIGHT - 8);

  uint32_t total = ESP.getHeapSize();
  uint32_t free = ESP.getFreeHeap();
  uint32_t used = total - free;
  display.print("RAM:");
  display.print(used / 1024);
  display.print("/");
  display.print(total / 1024);
  display.print("K");

  int16_t x, y;
  uint16_t w, h;
  display.getTextBounds("Tx:0000", 0, 0, &x, &y, &w, &h);
  display.setCursor(SCREEN_WIDTH - w - 2, SCREEN_HEIGHT - 8);
  display.printf("Tx:%lu", messageSentCount);
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

  // --- If only one page, update every 1 second ---
  // unsigned long interval = (totalPages > 1) ? PAGE_INTERVAL_MS : 1000;

  if (now - lastPageSwitch >= PAGE_INTERVAL_MS)
  {
    if (totalPages > 1)
    {
      pageIndex = (pageIndex + 1) % totalPages;
    }

    lastPageSwitch = now;
    render();
  }
}
