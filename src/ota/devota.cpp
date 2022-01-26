#include <Arduino.h>
#include <WiFi.h>
#include <ota/devota.h>
#include <WiFiUdp.h>
#include <HTTPUpdate.h>

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

void update_progress(int cur, int total) {
    *updateState = (double_t) cur * 100.0 / (double_t) total;
}

void setupOta(void *pvParameters) {
    updateState = (double_t *) pvParameters;

    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid, password);

    while (WiFi.waitForConnectResult() != WL_CONNECTED) {
    }

    IPAddress updateServer(192, 168, 0, 200);
    for (;;) {
        WiFiClient client;

        if (client.connect(updateServer, 80)) {

            // Make a HTTP request:
            client.println("GET /esp32/revision.txt HTTP/1.0");
            client.println("User-Agent: " + WiFi.macAddress());
            client.println("Referer: r" + String(BUILD_REVISION));
            client.println();
        }
        // Wait for Response
        delay(500);

        char buf[64];
        int bufIndex = 0;
        while (client.available() && bufIndex < 64) {
            char curChar = client.read();
            if (curChar == '\n') {
                bufIndex = 0;
                for (int i = 0; i < sizeof(buf); i++) {
                    buf[i] = 0;
                }
            } else {
                buf[bufIndex++] = curChar;
            }
        }
        client.stop();

        String response(buf);
        int remoteVersion = response.toInt();
        bool isNewVersionAvailable = remoteVersion > BUILD_REVISION;

        if (isNewVersionAvailable) {
#ifdef DEBUG_LORA_SERIAL
            Serial.println("New Version Available: " + String(isNewVersionAvailable) + " current: " + String(BUILD_REVISION) + " remote: " + String(remoteVersion));
#endif
            httpUpdate.onProgress(update_progress);
            httpUpdate.update(client, "192.168.0.200", 80, "/esp32/firmware.bin");
        }

        delay(3000);
        yield();
    }
}