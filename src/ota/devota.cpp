#include <WiFi.h>
#include <ArduinoHttpClient.h>
#include <Update.h>

/**
 * Over-The-Air Update für Entwicklungszwecke.
 * Dieser Code wird als Task auf dem Kern 0 ausgeführt.
 * Alle 3 Sekunden wir ein GET Request an einen Server geschickt, welcher die aktuelle Software Version als txt Datei enthält.
 * Wenn die Version neuer als die installierte ist, wird die Firmware heruntergeladen und in den speicher geschrieben.
 * Danach wird das Board zurückgesetzt.
 */
const char *ssid = "WirHabenKeinWlan";
const char *password = "aberwirhabeneinpasswort";

static double_t *updateState;

void update_progress(int cur, int total)
{
    *updateState = (double_t)cur * 100.0 / (double_t)total;
}

void setupOta(void *pvParameters)
{
    updateState = (double_t *)pvParameters;

    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid, password);

    while (WiFi.waitForConnectResult() != WL_CONNECTED)
    {
    }

    for (;;)
    {
        WiFiClient wifiClient;
        HttpClient client(wifiClient, "192.168.0.200", 80);

        client.get("/esp32/revision.txt");

        if (client.responseStatusCode() != 200)
        {
            client.stop();
            continue;
        }

        char buf[64];
        int bufIndex = 0;
        while (client.available() && bufIndex < 64)
        {
            char curChar = client.read();
            if (curChar == '\n')
            {
                bufIndex = 0;
                for (int i = 0; i < sizeof(buf); i++)
                {
                    buf[i] = 0;
                }
            }
            else
            {
                buf[bufIndex++] = curChar;
            }
        }
        client.stop();

        String response(buf);
        int remoteVersion = response.toInt();
        bool isNewVersionAvailable = remoteVersion > BUILD_REVISION;

        if (isNewVersionAvailable)
        {

            Serial.println("New Version Available: " + String(isNewVersionAvailable) + " current: " + String(BUILD_REVISION) + " remote: " + String(remoteVersion));

            HttpClient firmwareClient(wifiClient, "192.168.0.200", 80);
            firmwareClient.get("/esp32/firmware.bin");

            if (firmwareClient.responseStatusCode() != 200)
            {
                firmwareClient.stop();
                continue;
            }
            long length = firmwareClient.contentLength();
            int sketchFreeSpace = ESP.getFreeSketchSpace();

            if (length > sketchFreeSpace)
            {
                firmwareClient.stop();
                continue;
            }

            // Magic byte check
            if (firmwareClient.peek() != 0xE9)
            {
                firmwareClient.stop();
                continue;
            }

            update_progress(0, length);
            if (!Update.begin(length, U_FLASH, LED, 0))
            {
                firmwareClient.stop();
                continue;
            }

            Update.onProgress(update_progress);
            Update.write(firmwareClient);
            Update.end();

            update_progress(length, length);
            firmwareClient.stop();

            ESP.restart();
        }

        delay(3000);
        yield();
    }
}