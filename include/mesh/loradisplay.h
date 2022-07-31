#pragma once
#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <mesh/MeshRouter.h>

#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels

#define OLED_RESET 16       // Reset pin # (or -1 if sharing Arduino reset pin)
#define SCREEN_ADDRESS 0x3C ///< See datasheet for Address; 0x3D for 128x64, 0x3C for 128x32

class LoraDisplay
{
private:
    void drawInfoFooter(uint8_t nodeID);
    void drawSerialFooter();
    void drawUpdateFooter(double_t update);
    void drawWaitStatusFooter();


    uint8_t screenIndex = 1;

public:
    String lastSerialChar;
    unsigned long *waitTime;
    int queueLength;
    unsigned int *receivedBytes;

    void nextScreen();
    void initDisplay();
    void printRoutingTableScreen(RoutingTable_t **routingTable, uint8_t totalRoutes, uint8_t nodeID, double_t update);
};