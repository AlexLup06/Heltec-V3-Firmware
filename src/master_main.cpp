#include "shared/Common.h"
#include <WiFi.h>
#include <time.h>

const char *WIFI_SSID = "YourHotspotName";
const char *WIFI_PASSWORD = "YourHotspotPassword";
const char *ntpServer = "pool.ntp.org";
const long gmtOffset_sec = 0;
const int daylightOffset_sec = 0;

void connectToWiFi();
void initTime();
void printLocalTime();
void sendTimeSync();
void sendConfigMessage();

void setup()
{
    commonSetup(true); // master mode

    connectToWiFi();
    initTime();
    printLocalTime();

    button.onDoubleClick(sendTimeSync);
    button.onTripleClick(sendConfigMessage);
    button.onQuadrupleClick(dumpFilesOverSerial);
}

void loop()
{
    commonLoop();
}

// --- Function Definitions ---

void connectToWiFi()
{
    Serial.printf("Connecting to Wi-Fi: %s\n", WIFI_SSID);
    WiFi.mode(WIFI_STA);
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

    unsigned long start = millis();
    while (WiFi.status() != WL_CONNECTED)
    {
        delay(500);
        Serial.print(".");
        if (millis() - start > 15000)
        {
            Serial.println("\nWiFi connection failed!");
            return;
        }
    }

    Serial.println("\nWiFi connected!");
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());
}

void initTime()
{
    configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
    Serial.println("Getting time from NTP...");

    struct tm timeinfo;
    for (int i = 0; i < 20; i++)
    {
        if (getLocalTime(&timeinfo))
        {
            Serial.println("Time synchronized.");
            return;
        }
        delay(500);
    }
    Serial.println("Failed to get time from NTP.");
}

void printLocalTime()
{
    struct tm timeinfo;
    if (getLocalTime(&timeinfo))
    {
        Serial.printf("Current time: %02d:%02d:%02d %02d/%02d/%04d\n",
                      timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec,
                      timeinfo.tm_mday, timeinfo.tm_mon + 1, timeinfo.tm_year + 1900);
    }
    else
    {
        Serial.println("No time available.");
    }
}

void sendTimeSync()
{
    Serial.println("Double-click detected — sending time sync...");
    printLocalTime();
    configurator.sendTimeSync();
}

void sendConfigMessage()
{
    Serial.println("Triple-click detected — sending config message...");
    time_t startTime = time(nullptr) + 60;
    configurator.setStartTime(startTime);
    configurator.sendConfigMessage(startTime);
}