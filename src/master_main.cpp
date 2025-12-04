#include "shared/Common.h"
#include <WiFi.h>
#include <time.h>
#include "esp_wifi.h"

void initTime();
void printLocalTime();
void changeConfigForward();
void changeConfigBackward();
void confirmConfig();

void setup()
{
    commonSetup();

    initTime();
    printLocalTime();

    button.onSingleClick(changeConfigForward);
    button.onDoubleClick(changeConfigBackward);
    button.onTripleClick(confirmConfig);
}

void loop()
{
    commonLoop();
}

void initTime()
{
    const time_t timestamp = 1764628858;

    struct timeval tv;
    tv.tv_sec = timestamp;
    tv.tv_usec = 0;

    settimeofday(&tv, nullptr);

    Serial.println("Time set!");
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
    time_t startTime = time(NULL) + START_DELAY_SEC;
    configurator.confirmSetup(startTime);
    DEBUG_PRINTF("Double-click detected - setting start time: %lu\n", startTime);
}