#include "shared/Common.h"
#include <WiFi.h>
#include <time.h>
#include "esp_wifi.h"

const char *WIFI_SSID = "Alex Iphone";
const char *WIFI_PASSWORD = "test123456";
const char *ntpServer = "pool.ntp.org";
const long gmtOffset_sec = 0;
const int daylightOffset_sec = 0;

void connectToWiFi();
void disableWiFiFully();

void initTime();
void printLocalTime();

void changeConfigForward();
void changeConfigBackward();
void confirmConfig();

void setup()
{
    commonSetup();

    connectToWiFi();
    initTime();
    printLocalTime();
    disableWiFiFully();

    button.onSingleClick(changeConfigForward);
    button.onDoubleClick(changeConfigBackward);
    button.onTripleClick(confirmConfig);
}

void loop()
{
    commonLoop();
}

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

void disableWiFiFully()
{
    WiFi.disconnect(true, true);
    delay(200);

    esp_wifi_stop();
    esp_wifi_deinit();
    WiFi.mode(WIFI_OFF);
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
        Serial.println("No time available");
    }
}

void changeConfigForward()
{
    configurator.setNetworkTopology(true);
}

void changeConfigBackward()
{
    configurator.setNetworkTopology(false);
}

void confirmConfig()
{
    uint64_t startTime = unixTimeMs() + START_DELAY_SEC * 1000;
    configurator.confirmSetup(startTime);
    DEBUG_PRINTF("Double-click detected - setting start time: %llu\n", startTime);

    time_t t = (time_t)(startTime / 1000ULL);
    struct tm timeinfo;
    localtime_r(&t, &timeinfo);
    uint16_t ms = startTime % 1000ULL;
    Serial.printf("Start Time: %02d:%02d:%02d.%03u %02d/%02d/%04d\n",
                  timeinfo.tm_hour,
                  timeinfo.tm_min,
                  timeinfo.tm_sec,
                  ms,
                  timeinfo.tm_mday,
                  timeinfo.tm_mon + 1,
                  timeinfo.tm_year + 1900);
}