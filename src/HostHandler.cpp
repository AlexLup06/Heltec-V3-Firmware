/*
* The logic contained here waits for data packets from the host system. Only binary data of the SerialPackage_t type is expressly expected.
 * This type is 5 bytes in size. The pointer to the first 5 bytes is interpreted directly as this structure. There is no validity check.
 * Note: This implementation is very prone to lost bytes as the device and host states are out of sync.
 * 
 * Die hier enthaltene Logik wartet auf Datenpakete vom Host System. Es werden ausdrücklich nur Binäre Daten vom Typ SerialPaket_t erwartet.
 * Dieser Typ ist 5 bytes groß. Der Pointer auf den ersten 5 Bytes wird direkt als diese Struktur interpretiert. Es folgt keine Prüfung auf Gültigkeit.
 * Anmerkung: Diese Implementierung ist sehr anfällig für verlorene Bytes, da dann die Zustände von Gerät und Host nicht mehr Synchron sind.
 */
#include "C:\Users\Anushka Gulati\source\repos\RobotNetwork\include\main.h"
#include "C:\Users\Anushka Gulati\source\repos\RobotNetwork\include\HostHandler.h"
#include <Arduino.h>
#include "C:\Users\Anushka Gulati\source\repos\RobotNetwork\include\mesh\MeshRouter.h"

#define SERIAL_MAGIC_BYTES 0
#define SERIAL_REC_HEADER 1
#define SERIAL_REC_PAYLOAD 2
#define SERIAL_WAIT_PROCESS 3

uint8_t serialStatus = SERIAL_MAGIC_BYTES;
uint16_t serialIndex = 0;
HostSerialHandlerParams_t *hostHandlerParams;

SerialPaketHeader_t *serialPaketHeader = nullptr;
uint8_t *receiveBuffer;

unsigned long lastDataReceived = millis();

/* Once complete package received, checks type of packet and processes accordingly.
*/
void onPacketReceived() {
    // Paket komplett empfangen

    switch (serialPaketHeader->serialPaketType) {
    case SERIAL_PAKET_TYPE_FLOOD_PAKET:
    {
        auto floodPaket = (SerialPayloadFloodPaket_t*)malloc(sizeof(SerialPayloadFloodPaket_t));
        memcpy(floodPaket, receiveBuffer, sizeof(SerialPayloadFloodPaket_t));

        floodPaket->payload = (uint8_t*)malloc(floodPaket->size);
        memcpy(floodPaket->payload, receiveBuffer + sizeof(SerialPayloadFloodPaket_t), floodPaket->size);

        hostHandlerParams->serialFloodPaketHeader = floodPaket;
        hostHandlerParams->ready = true;

        //free allocated memory
        free(serialPaketHeader);
        free(receiveBuffer);
        serialStatus = SERIAL_WAIT_PROCESS;
    }
        break;

    case SERIAL_PACKET_TYPE_RSSI_REQUEST:
    {
        auto floodPaket = (SerialPayloadFloodPaket_t*)malloc(sizeof(SerialPayloadFloodPaket_t));
        memcpy(floodPaket, receiveBuffer, sizeof(SerialPayloadFloodPaket_t));

        floodPaket->payload = (uint8_t*)malloc(floodPaket->size);
        memcpy(floodPaket->payload, receiveBuffer + sizeof(SerialPayloadFloodPaket_t), floodPaket->size);

        auto rssiRequestPaket = (SerialPacketRSSI_Request_t*)floodPaket->payload;

        auto payloadBuffer = (uint8_t*)malloc(sizeof(SerialPacketRSSI_Response_t));
        ((SerialPacketRSSI_Response_t*)payloadBuffer)->nodeID = rssiRequestPaket->nodeID;
        ((SerialPacketRSSI_Response_t*)payloadBuffer)->nodeRSSI = getMeshRouter()->getRSSI(rssiRequestPaket->nodeID);

        //reuse Serial Header for response
        serialPaketHeader->serialPaketType = SERIAL_PACKET_TYPE_RSSI_RESPONSE;
        serialPaketHeader->size = sizeof(SerialPacketRSSI_Response_t);
        //send message
        {
            std::lock_guard<std::mutex> lck(*getSerialMutex());
            uint8_t magicBytes[] = { 0xF0, 0x4L, 0x11, 0x9B, 0x39, 0xBC, 0xE4, 0xD2 };
            Serial.write(magicBytes, 8);
            Serial.write((uint8_t*)serialPaketHeader, 3);
            Serial.write(payloadBuffer, serialPaketHeader->size);
        }
        //free allocated memory
        free(floodPaket->payload);
        free(floodPaket);
        free(serialPaketHeader);
        free(payloadBuffer);
        free(receiveBuffer);
        serialStatus = SERIAL_WAIT_PROCESS;
    }
    break;

    case SERIAL_PACKET_TYPE_SNR_REQUEST:
    {
        auto payloadBuffer = (uint8_t*)malloc(sizeof(SerialPacketSNR_Response_t));
        ((SerialPacketSNR_Response_t*)payloadBuffer)->nodeSNR = getMeshRouter()->getSNR();

        //reuse Serial Header for response
        serialPaketHeader->serialPaketType = SERIAL_PACKET_TYPE_SNR_RESPONSE;
        serialPaketHeader->size = sizeof(SerialPacketSNR_Response_t);

        //send message
        {
            std::lock_guard<std::mutex> lck(*getSerialMutex());
            uint8_t magicBytes[] = { 0xF0, 0x4L, 0x11, 0x9B, 0x39, 0xBC, 0xE4, 0xD2 };
            Serial.write(magicBytes, 8);
            Serial.write((uint8_t*)serialPaketHeader, 3);
            Serial.write(payloadBuffer, serialPaketHeader->size);
        }
        //free allocated memory
        free(serialPaketHeader);
        free(payloadBuffer);
        free(receiveBuffer);
        serialStatus = SERIAL_WAIT_PROCESS;
    }
    break;

    case SERIAL_PACKET_TYPE_STATUS_REQUEST:
    {
        //auto floodPaket = (SerialPayloadFloodPaket_t*)receiveBuffer;
        //free allocated memory
        free(serialPaketHeader);
        free(receiveBuffer);
        serialStatus = SERIAL_WAIT_PROCESS;
    }
    break;

    case SERIAL_PACKET_TYPE_CONFIGURATION_REQUEST:
    {
        auto floodPaket = (SerialPayloadFloodPaket_t*)malloc(sizeof(SerialPayloadFloodPaket_t));
        memcpy(floodPaket, receiveBuffer, sizeof(SerialPayloadFloodPaket_t));

        floodPaket->payload = (uint8_t*)malloc(floodPaket->size);
        memcpy(floodPaket->payload, receiveBuffer + sizeof(SerialPayloadFloodPaket_t), floodPaket->size);
        auto configRequestPaket = (SerialPacketConfig_Request_t*)floodPaket->payload;

        auto payloadBuffer = (uint8_t*)malloc(sizeof(SerialPacketConfig_Response_t));
        //*hostHandlerParams->debugString = String("NNID") + String(configRequestPaket->newNodeID)+String("Sz")+String(serialPaketHeader->size)+String("ST")+String(serialPaketHeader->serialPaketType);
        ((SerialPacketConfig_Response_t*)payloadBuffer)->newNodeID = getMeshRouter()->setNodeID(configRequestPaket->newNodeID);

        //reuse Serial Header for response
        serialPaketHeader->serialPaketType = SERIAL_PACKET_TYPE_CONFIGURATION_RESPONSE;
        serialPaketHeader->size = sizeof(SerialPacketConfig_Response_t);
        //send message
        {
            std::lock_guard<std::mutex> lck(*getSerialMutex());
            uint8_t magicBytes[] = { 0xF0, 0x4L, 0x11, 0x9B, 0x39, 0xBC, 0xE4, 0xD2 };
            Serial.write(magicBytes, 8);
            Serial.write((uint8_t*)serialPaketHeader, 3);
            Serial.write(payloadBuffer, serialPaketHeader->size);
        }
        
        //free allocated memory
        free(floodPaket->payload);
        free(floodPaket);
        free(serialPaketHeader);
        free(payloadBuffer);
        free(receiveBuffer);
        serialStatus = SERIAL_WAIT_PROCESS;
    }
    break;

    case SERIAL_PACKET_TYPE_APPLY_MODEM_CONFIG:
    {
        auto floodPaket = (SerialPayloadFloodPaket_t*)malloc(sizeof(SerialPayloadFloodPaket_t));
        memcpy(floodPaket, receiveBuffer, sizeof(SerialPayloadFloodPaket_t));

        floodPaket->payload = (uint8_t*)malloc(floodPaket->size);
        memcpy(floodPaket->payload, receiveBuffer + sizeof(SerialPayloadFloodPaket_t), floodPaket->size);
        auto modemConfig = (ModemConfig_t*)floodPaket->payload;

        getMeshRouter()->applyModemConfig(modemConfig->sf,modemConfig->transmissionPower,modemConfig->frequency,modemConfig->bandwidth);

        //free allocated memory
        free(floodPaket->payload);
        free(floodPaket);
        free(serialPaketHeader);
        free(receiveBuffer);
        serialStatus = SERIAL_WAIT_PROCESS;
    }
    break;
    default:
    {
        //free allocated memory
        free(serialPaketHeader);
        free(receiveBuffer);
        serialStatus = SERIAL_WAIT_PROCESS;
    }
    break;
    }
}

uint8_t magicBytes[] = {0xF0, 0x4L, 0x11, 0x9B, 0x39, 0xBC, 0xE4, 0xD2};
uint8_t magicBytesIndex = 0;

void readAvailableSerialData() {
    // Check for available Bytes for reading
    uint16_t availableBytes = Serial.available();

    if (availableBytes) {
        while (availableBytes > 0 && serialStatus == SERIAL_MAGIC_BYTES) {
            if (magicBytesIndex < 8) {
                uint8_t read = Serial.read();
                availableBytes--;
                if (read == magicBytes[magicBytesIndex]) {
                    magicBytesIndex++;
                }
                else if (magicBytesIndex > 0 && read == magicBytes[0]) {
                    magicBytesIndex = 1;
                }
                else {
                    magicBytesIndex = 0;
                }
            }
            else {
                serialStatus = SERIAL_REC_HEADER;
                availableBytes = Serial.available();
                magicBytesIndex = 0;
            }
        }


        if (serialStatus == SERIAL_REC_HEADER && availableBytes >= sizeof(SerialPaketHeader_t)) {
            // Allocate memory for the header and fill it with the first 3 bytes from the serial
            // Speicher für den Header alokieren und mit ersten 3 Bytes aus dem Serial füllen
            serialPaketHeader = (SerialPaketHeader_t*)malloc(sizeof(SerialPaketHeader_t));
            Serial.read((uint8_t*)serialPaketHeader, sizeof(SerialPaketHeader_t));

            // Debug
            //*hostHandlerParams->debugString = String("H.S:") + String(serialPaketHeader->size);

            // Allocate memory with specified size from headerand set pointer
            // If less than PAKET_SIZE + 10000 heap is available after allocation, then discard the packet!
            // Speicher mit angegebener größe aus dem Header alokieren und pointer setzten
            // Wenn nach allokation weniger als PAKET_SIZE + 10000 Heap verfügbar ist, dann verwerfe das Paket!
            if (xPortGetFreeHeapSize() - serialPaketHeader->size < serialPaketHeader->size + 10000) {
                serialStatus = SERIAL_MAGIC_BYTES;
                free(serialPaketHeader);
                serialPaketHeader = nullptr;
                serialIndex = 0;
                return;
            }
            receiveBuffer = (uint8_t*)malloc(serialPaketHeader->size);
            serialStatus = SERIAL_REC_PAYLOAD;
            availableBytes = Serial.available();
        }

        // Fill the buffer with data from the serial port.
        // Buffer mit Daten aus dem Serial Port füllen.
        if (serialStatus == SERIAL_REC_PAYLOAD && availableBytes) {
            // If more data is available than is available, then only read in to the end of the packet. Otherwise read available bytes.
            // Wenn mehr Daten als verfügbar Bereit stehen, dann nur bis Paket-Ende einlesen. Ansonsten verfügbare Bytes lesen.
            uint16_t bytesToRead =
                availableBytes + serialIndex > serialPaketHeader->size ?
                serialPaketHeader->size - serialIndex : availableBytes;
            //serialIndex is the current index in the receive buffer.
            // This is increased by the received bytes.
            // serialIndex ist der aktuelle Index im Empfangs-Puffer.
            // Dieser wird um die empfangenen Bytes erhöht.
            serialIndex += Serial.read(receiveBuffer + serialIndex, bytesToRead);

            // Show the progress of the current package in the display
            // Fortschritt des aktuellen Pakets im Display anzeigen
            //*hostHandlerParams->debugString =
            //        String(serialIndex) + "/" + String(serialPaketHeader->size) + " " +
            //        String(availableBytes);


            if (serialIndex >= serialPaketHeader->size) {
                onPacketReceived();
            }
        }
    }

    /** If packet has been processed, then serialStatus = SERIAL_WAIT_PROCESS and the other process has the ready attribute set to false.
     * In this case, the system switches back to the initial status so that the next packet can be received. */
    /** Wenn Paket verarbeitet worden ist, dann ist serialStatus = SERIAL_WAIT_PROCESS und der andere Prozess hat das ready Attribut auf false gesetzt.
     * In diesem Fall wird wieder in den Initialen Status gewechselt, damit das nächste Paket empfangen werden kann.
     */
    if (serialStatus == SERIAL_WAIT_PROCESS && !hostHandlerParams->ready) {
        free(hostHandlerParams->serialFloodPaketHeader);
        hostHandlerParams->serialFloodPaketHeader = nullptr;
        serialStatus = SERIAL_MAGIC_BYTES;
        serialIndex = 0;
    }
}

/* This logic is executed as an RTOS task on core 0.
 * The HostSerialHandlerParams_t structure is transferred, in which received packets are stored.
 * The fully received packets are processed on core 1, which executes the logic for the Lora network.
 * @param pvParameters
 */
/**
 * Diese Logik wird als RTOS Task auf dem Kern 0 ausgeführt.
 * Übergeben wird die Struktur HostSerialHandlerParams_t, in welcher empfangene Pakete abgelegt werden.
 * Die Bearbeitung der vollständig empfangenen Pakete erfolgt auf dem Kern 1, welcher die Logik für das Lora-Netzwerk ausführt.
 * @param pvParameters
 */
void hostHandler(void* pvParameters) {
    hostHandlerParams = (HostSerialHandlerParams_t*)pvParameters;
    *hostHandlerParams->debugString = "Waiting..";
    // We are in an RTOS task, so this infinite loop is valid behavior. We're not blocking the entire core with it.
    // Wir befinden uns in einem RTOS Task, daher ist diese Endlosschleife gültiges verhalten. Wir blockieren damit nicht den gesamten Kern.
    for (;;) {
        readAvailableSerialData();
        yield();
    }
}