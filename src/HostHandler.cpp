/*
 * Die hier enthaltene Logik wartet auf Datenpakete vom Host System. Es werden ausdrücklich nur Binäre Daten vom Typ SerialPaket_t erwartet.
 * Dieser Typ ist 5 bytes groß. Der Pointer auf den ersten 5 Bytes wird direkt als diese Struktur interpretiert. Es folgt keine Prüfung auf Gültigkeit.
 * Anmerkung: Diese Implementierung ist sehr anfällig für verlorene Bytes, da dann die Zustände von Gerät und Host nicht mehr Synchron sind.
 */

#include "HostHandler.h"
#include <Arduino.h>
#include "mesh/MeshRouter.h"

#define SERIAL_REC_HEADER 0
#define SERIAL_REC_PAYLOAD 1
#define SERIAL_WAIT_PROCESS 2

uint8_t serialStatus = SERIAL_REC_HEADER;
uint16_t serialIndex = 0;

HostSerialHandlerParams_t *hostHandlerParams;

SerialPaketHeader_t *serialPaketHeader;
uint8_t *receiveBuffer;

void onPacketReceived() {
    // Paket komplett empfangen

    switch (serialPaketHeader->serialPaketType) {
        case SERIAL_PAKET_TYPE_FLOOD_PAKET:
            auto floodPaket = (SerialPayloadFloodPaket_t *) malloc(sizeof(SerialPayloadFloodPaket_t));
            floodPaket->messageType = serialPaketHeader->serialPaketType;
            floodPaket->size = serialPaketHeader->size - 1; // Zusätzliches Header Field
            floodPaket->source = receiveBuffer[0];
            receiveBuffer++;
            floodPaket->payload = receiveBuffer;

            hostHandlerParams->serialFloodPaketHeader = floodPaket;
            hostHandlerParams->ready = true;
            serialStatus = SERIAL_WAIT_PROCESS;

            free(serialPaketHeader);
            break;
    }
}

void readAvailableSerialData() {
    int availableBytes = Serial.available();
    if (availableBytes) {
        if (serialStatus == SERIAL_REC_HEADER && availableBytes >= 3) {
            // Speicher für den Header alokieren und mit ersten 3 Bytes aus dem Serial füllen
            serialPaketHeader = (SerialPaketHeader_t *) malloc(3);
            Serial.read((uint8_t *) serialPaketHeader, 3);

            // Debug
            *hostHandlerParams->debugString = String("H.S:") + String(serialPaketHeader->size);

            // Speicher mit angegebener größe aus dem Header alokieren und pointer setzten
            receiveBuffer = (uint8_t *) malloc(serialPaketHeader->size);
            serialStatus = SERIAL_REC_PAYLOAD;
            availableBytes = Serial.available();
        }


        // Buffer mit Daten aus dem Serial Port füllen.
        if (serialStatus == SERIAL_REC_PAYLOAD && availableBytes) {
            // Wenn mehr Daten als verfügbar Bereit stehen, dann nur bis Paket-Ende einlesen. Ansonsten verfügbare Bytes lesen.
            int bytesToRead =
                    availableBytes + serialIndex > serialPaketHeader->size ?
                    serialPaketHeader->size - serialIndex
                                                                           : availableBytes;
            // serialIndex ist der aktuelle Index im Empfangs-Puffer.
            // Dieser wird um die empfangenen Bytes erhöht.
            serialIndex += Serial.read(receiveBuffer + serialIndex, bytesToRead);

            // Fortschritt des aktuellen Pakets im Display anzeigen
            *hostHandlerParams->debugString =
                    String(serialIndex) + "/" + String(serialPaketHeader->size) + " " +
                    String(availableBytes);


            if (serialIndex >= serialPaketHeader->size) {
                onPacketReceived();
            }
        }
    }

    /** Wenn Paket verarbeitet worden ist, dann ist serialStatus = SERIAL_WAIT_PROCESS und der andere Prozess hat das ready Attribut auf false gesetzt.
     * In diesem Fall wird wieder in den Initialen Status gewechselt, damit das nächste Paket empfangen werden kann.
     */
    if (serialStatus == SERIAL_WAIT_PROCESS && !hostHandlerParams->ready) {
        hostHandlerParams->serialFloodPaketHeader = nullptr;
        serialStatus = SERIAL_REC_HEADER;
        serialIndex = 0;
    }
}

/**
 * Diese Logik wird als RTOS Task auf dem Kern 0 ausgeführt.
 * Übergeben wird die Struktur HostSerialHandlerParams_t, in welcher empfangene Pakete abgelegt werden.
 * Die Bearbeitung der vollständig empfangenen Pakete erfolgt auf dem Kern 1, welcher die Logik für das Lora-Netzwerk ausführt.
 * @param pvParameters
 */
void hostHandler(void *pvParameters) {
    hostHandlerParams = (HostSerialHandlerParams_t *) pvParameters;
    *hostHandlerParams->debugString = "Waiting..";
    // Wir befinden uns in einem RTOS Task, daher ist diese Endlosschleife gültiges verhalten. Wir blockieren damit nicht den gesamten Kern.
    for (;;) {
        readAvailableSerialData();
        yield();
    }
}