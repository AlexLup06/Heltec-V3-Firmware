#include <mesh/loradisplay.h>

#define DEBUG_PRINT_SERIAL_DATA


Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

void LoraDisplay::initDisplay() {
    Wire.setPins(4, 15);

    if (!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
        Serial.println(F("SSD1306 allocation failed"));
        while (true);
    }
    delay(200);
    //display.clearDisplay();
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(WHITE);
    display.display();
}

void LoraDisplay::drawCentreString(const char *buf, int x, int y) {
    int16_t x1, y1;
    uint16_t w, h;
    display.getTextBounds(buf, x, y, &x1, &y1, &w, &h); //calc width of new string
    display.setCursor(x - w / 2, y);
    display.print(buf);
}

/**
 * Draws the Display based on the current Application state
 */

void LoraDisplay::render() {
}

void LoraDisplay::drawSplashScreen() {
    display.clearDisplay();
    display.drawBitmap(24, 0, whale, 80, 62, WHITE);
    display.display();
}

void LoraDisplay::printRoutingTableScreen(RoutingTable_t **routingTable, uint8_t totalRoutes, uint8_t nodeID,
                                          double_t update) {
    display.clearDisplay();
    display.setCursor(0, 0);
    display.print("Id");
    display.setCursor(28, 0);
    display.print("Hop");
    display.setCursor(56, 0);
    display.print("Mac");
    display.setCursor(103, 0);
    display.println("RSSI");
    display.drawFastHLine(0, 8, DISPLAY_WIDTH, WHITE);
    display.setCursor(0, 10);

    for (int i = 0; i < totalRoutes; i++) {
        char macHexStr[8] = {0};
        sprintf(macHexStr, "%02X%02X%02X", routingTable[i]->deviceMac[3], routingTable[i]->deviceMac[4],
                routingTable[i]->deviceMac[5]);
        display.setCursor(0, 10 + 8 * i);
        display.print(String(routingTable[i]->nodeId));
        display.setCursor(28, 10 + 8 * i);
        display.print(String(routingTable[i]->hop));
        display.setCursor(56, 10 + 8 * i);
        display.println(String(macHexStr));
        display.setCursor(103, 10 + 8 * i);
        display.println(String(routingTable[i]->rssi));
    }
    display.drawFastVLine(22, 0, 10 + totalRoutes * 8, WHITE);
    display.drawFastVLine(50, 0, 10 + totalRoutes * 8, WHITE);
    display.drawFastVLine(97, 0, 10 + totalRoutes * 8, WHITE);

    if (update > 0) {
        drawUpdateFooter(update);
    } else {
        switch (screenIndex) {
            case 0:
                drawInfoFooter(nodeID);
                break;
            case 1:
                drawSerialFooter();
                break;
        }
    }


    display.display();
}

void LoraDisplay::drawSerialFooter() {
    display.drawFastHLine(0, DISPLAY_HEIGHT - 10, DISPLAY_WIDTH, WHITE);
    display.setCursor(0, DISPLAY_HEIGHT - 18);
    display.println("SC");
    display.setCursor(0, DISPLAY_HEIGHT - 8);
    display.println(lastSerialChar);

    display.setCursor(90, DISPLAY_HEIGHT - 18);
    display.println("Code");
    display.setCursor(90, DISPLAY_HEIGHT - 8);
    display.println(String((uint8_t) lastSerialChar.charAt(lastSerialChar.length() - 1)));
};

void LoraDisplay::drawUpdateFooter(double_t update) {
    int barWidth = update / 100 * 128;
    display.fillRect(0, DISPLAY_HEIGHT - 10, barWidth, 10, WHITE);
    display.setCursor(0, DISPLAY_HEIGHT - 18);
    display.println("Updating...");

    display.setCursor(100, DISPLAY_HEIGHT - 18);
    display.println(String(update, 0) + "%");
};


void LoraDisplay::drawInfoFooter(uint8_t nodeID) {
    display.drawFastHLine(0, DISPLAY_HEIGHT - 10, DISPLAY_WIDTH, WHITE);
    display.setCursor(0, DISPLAY_HEIGHT - 18);
    display.println("ID");
    display.setCursor(0, DISPLAY_HEIGHT - 8);
    display.println(String(nodeID));

    display.setCursor(40, DISPLAY_HEIGHT - 18);
    display.println("Rev");
    display.setCursor(40, DISPLAY_HEIGHT - 8);
    display.println(String(BUILD_REVISION % 100000));

    display.setCursor(90, DISPLAY_HEIGHT - 18);
    display.println("Heap");
    display.setCursor(90, DISPLAY_HEIGHT - 8);
    display.println(String(ESP.getFreeHeap()));

}

Adafruit_SSD1306 *LoraDisplay::getDisplay() {
    return &display;
}

void LoraDisplay::nextScreen() {
    screenIndex++;
    if (screenIndex > 1) {
        screenIndex = 0;
    }

}
