/*
 * The logic contained here waits for data packets from the host system. Only binary data of the SerialPackage_t type is expressly expected.
 * This type is 5 bytes in size. The pointer to the first 5 bytes is interpreted directly as this structure. There is no validity check.
 * Note: This implementation is very prone to lost bytes as the device and host states are out of sync.
 *
 * Die hier enthaltene Logik wartet auf Datenpakete vom Host System. Es werden ausdrücklich nur Binäre Daten vom Typ SerialPaket_t erwartet.
 * Dieser Typ ist 5 bytes groß. Der Pointer auf den ersten 5 Bytes wird direkt als diese Struktur interpretiert. Es folgt keine Prüfung auf Gültigkeit.
 * Anmerkung: Diese Implementierung ist sehr anfällig für verlorene Bytes, da dann die Zustände von Gerät und Host nicht mehr Synchron sind.
 */
#include <Arduino.h>

#include "HostHandler.h"

#define SERIAL_MAGIC_BYTES 0
#define SERIAL_REC_HEADER 1
#define SERIAL_REC_PAYLOAD 2
#define SERIAL_WAIT_PROCESS 3

uint8_t serialStatus = SERIAL_MAGIC_BYTES;
uint16_t serialIndex = 0;
HostSerialHandlerParams_t *hostHandlerParams;

SerialPacketHeader_t *serialPacketHeader = nullptr;
uint8_t *receiveBuffer;

unsigned long lastDataReceived = millis();

/* Once complete package received, checks type of packet and processes accordingly.
 */
void onPacketReceived()
{
    // Paket komplett empfangen

    switch (serialPacketHeader->serialPacketType)
    {
    case SERIAL_PACKET_TYPE_FLOOD_PACKET:
    {
        auto floodPacket = (SerialPayloadFloodPacket_t *)malloc(sizeof(SerialPayloadFloodPacket_t));
        memcpy(floodPacket, receiveBuffer, sizeof(SerialPayloadFloodPacket_t));

        floodPacket->payload = (uint8_t *)malloc(floodPacket->size);
        memcpy(floodPacket->payload, receiveBuffer + sizeof(SerialPayloadFloodPacket_t), floodPacket->size);

        hostHandlerParams->serialFloodPacketHeader = floodPacket;
        hostHandlerParams->ready = true;

        // free allocated memory
        free(serialPacketHeader);
        free(receiveBuffer);
        serialStatus = SERIAL_WAIT_PROCESS;
    }
    break;

    case SERIAL_PACKET_TYPE_STATUS_REQUEST:
    {
        // auto floodPacket = (SerialPayloadFloodPacket_t*)receiveBuffer;
        // free allocated memory
        free(serialPacketHeader);
        free(receiveBuffer);
        serialStatus = SERIAL_WAIT_PROCESS;
    }
    break;

    case SERIAL_PACKET_TYPE_CONFIGURATION_REQUEST:
    {
        auto floodPacket = (SerialPayloadFloodPacket_t *)malloc(sizeof(SerialPayloadFloodPacket_t));
        memcpy(floodPacket, receiveBuffer, sizeof(SerialPayloadFloodPacket_t));

        floodPacket->payload = (uint8_t *)malloc(floodPacket->size);
        memcpy(floodPacket->payload, receiveBuffer + sizeof(SerialPayloadFloodPacket_t), floodPacket->size);
        auto configRequestPacket = (SerialPacketConfig_Request_t *)floodPacket->payload;

        auto payloadBuffer = (uint8_t *)malloc(sizeof(SerialPacketConfig_Response_t));
        //*hostHandlerParams->debugString = String("NNID") + String(configRequestPacket->newNodeID)+String("Sz")+String(serialPacketHeader->size)+String("ST")+String(serialPacketHeader->serialPacketType);
        ((SerialPacketConfig_Response_t *)payloadBuffer)->newNodeID = hostHandlerParams->macProtocol->setNodeID(configRequestPacket->newNodeID);

        // reuse Serial Header for response
        serialPacketHeader->serialPacketType = SERIAL_PACKET_TYPE_CONFIGURATION_RESPONSE;
        serialPacketHeader->size = sizeof(SerialPacketConfig_Response_t);
        // send message
        {
            std::lock_guard<std::mutex> lck(*getSerialMutex());
            uint8_t magicBytes[] = {0xF0, 0x4L, 0x11, 0x9B, 0x39, 0xBC, 0xE4, 0xD2};
            Serial.write(magicBytes, 8);
            Serial.write((uint8_t *)serialPacketHeader, 3);
            Serial.write(payloadBuffer, serialPacketHeader->size);
        }

        // free allocated memory
        free(floodPacket->payload);
        free(floodPacket);
        free(serialPacketHeader);
        free(payloadBuffer);
        free(receiveBuffer);
        serialStatus = SERIAL_WAIT_PROCESS;
    }
    break;

    case SERIAL_PACKET_TYPE_APPLY_MODEM_CONFIG:
    {
        auto floodPacket = (SerialPayloadFloodPacket_t *)malloc(sizeof(SerialPayloadFloodPacket_t));
        memcpy(floodPacket, receiveBuffer, sizeof(SerialPayloadFloodPacket_t));

        floodPacket->payload = (uint8_t *)malloc(floodPacket->size);
        memcpy(floodPacket->payload, receiveBuffer + sizeof(SerialPayloadFloodPacket_t), floodPacket->size);
        auto modemConfig = (ModemConfig_t *)floodPacket->payload;

        hostHandlerParams->macProtocol->applyModemConfig(modemConfig->sf, modemConfig->transmissionPower, modemConfig->frequency, modemConfig->bandwidth);

        // free allocated memory
        free(floodPacket->payload);
        free(floodPacket);
        free(serialPacketHeader);
        free(receiveBuffer);
        serialStatus = SERIAL_WAIT_PROCESS;
    }
    break;
    default:
    {
        // free allocated memory
        free(serialPacketHeader);
        free(receiveBuffer);
        serialStatus = SERIAL_WAIT_PROCESS;
    }
    break;
    }
}

uint8_t magicBytes[] = {0xF0, 0x4L, 0x11, 0x9B, 0x39, 0xBC, 0xE4, 0xD2};
uint8_t magicBytesIndex = 0;

void readAvailableSerialData()
{
    // Check for available Bytes for reading
    uint16_t availableBytes = Serial.available();

    if (availableBytes)
    {
        while (availableBytes > 0 && serialStatus == SERIAL_MAGIC_BYTES)
        {
            if (magicBytesIndex < 8)
            {
                uint8_t read = Serial.read();
                availableBytes--;
                if (read == magicBytes[magicBytesIndex])
                {
                    magicBytesIndex++;
                }
                else if (magicBytesIndex > 0 && read == magicBytes[0])
                {
                    magicBytesIndex = 1;
                }
                else
                {
                    magicBytesIndex = 0;
                }
            }
            else
            {
                serialStatus = SERIAL_REC_HEADER;
                availableBytes = Serial.available();
                magicBytesIndex = 0;
            }
        }

        if (serialStatus == SERIAL_REC_HEADER && availableBytes >= sizeof(SerialPacketHeader_t))
        {
            // Allocate memory for the header and fill it with the first 3 bytes from the serial
            // Speicher für den Header alokieren und mit ersten 3 Bytes aus dem Serial füllen
            serialPacketHeader = (SerialPacketHeader_t *)malloc(sizeof(SerialPacketHeader_t));
            Serial.read((uint8_t *)serialPacketHeader, sizeof(SerialPacketHeader_t));

            // Debug
            //*hostHandlerParams->debugString = String("H.S:") + String(serialPacketHeader->size);

            // Allocate memory with specified size from header and set pointer
            // If less than PACKET_SIZE + 10000 heap is available after allocation, then discard the packet!
            // Speicher mit angegebener größe aus dem Header alokieren und pointer setzten
            // Wenn nach allokation weniger als PACKET_SIZE + 10000 Heap verfügbar ist, dann verwerfe das Packet!
            if (xPortGetFreeHeapSize() - serialPacketHeader->size < serialPacketHeader->size + 10000)
            {
                serialStatus = SERIAL_MAGIC_BYTES;
                free(serialPacketHeader);
                serialPacketHeader = nullptr;
                serialIndex = 0;
                return;
            }
            receiveBuffer = (uint8_t *)malloc(serialPacketHeader->size);
            serialStatus = SERIAL_REC_PAYLOAD;
            availableBytes = Serial.available();
        }

        // Fill the buffer with data from the serial port.
        // Buffer mit Daten aus dem Serial Port füllen.
        if (serialStatus == SERIAL_REC_PAYLOAD && availableBytes)
        {
            // If more data is available than is available, then only read in to the end of the packet. Otherwise read available bytes.
            // Wenn mehr Daten als verfügbar Bereit stehen, dann nur bis Packet-Ende einlesen. Ansonsten verfügbare Bytes lesen.
            uint16_t bytesToRead =
                availableBytes + serialIndex > serialPacketHeader->size ? serialPacketHeader->size - serialIndex : availableBytes;
            // serialIndex is the current index in the receive buffer.
            //  This is increased by the received bytes.
            //  serialIndex ist der aktuelle Index im Empfangs-Puffer.
            //  Dieser wird um die empfangenen Bytes erhöht.
            serialIndex += Serial.read(receiveBuffer + serialIndex, bytesToRead);

            // Show the progress of the current package in the display
            // Fortschritt des aktuellen Packets im Display anzeigen
            //*hostHandlerParams->debugString =
            //        String(serialIndex) + "/" + String(serialPacketHeader->size) + " " +
            //        String(availableBytes);

            if (serialIndex >= serialPacketHeader->size)
            {
                onPacketReceived();
            }
        }
    }

    /** If packet has been processed, then serialStatus = SERIAL_WAIT_PROCESS and the other process has the ready attribute set to false.
     * In this case, the system switches back to the initial status so that the next packet can be received. */
    /** Wenn Paket verarbeitet worden ist, dann ist serialStatus = SERIAL_WAIT_PROCESS und der andere Prozess hat das ready Attribut auf false gesetzt.
     * In diesem Fall wird wieder in den Initialen Status gewechselt, damit das nächste Paket empfangen werden kann.
     */
    if (serialStatus == SERIAL_WAIT_PROCESS && !hostHandlerParams->ready)
    {
        free(hostHandlerParams->serialFloodPacketHeader);
        hostHandlerParams->serialFloodPacketHeader = nullptr;
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

// TODO: we have to simulate data packets
void hostHandler(void *pvParameters)
{
    hostHandlerParams = (HostSerialHandlerParams_t *)pvParameters;
    // We are in an RTOS task, so this infinite loop is valid behavior. We're not blocking the entire core with it.
    // Wir befinden uns in einem RTOS Task, daher ist diese Endlosschleife gültiges verhalten. Wir blockieren damit nicht den gesamten Kern.
    for (;;)
    {
        readAvailableSerialData();
        yield();
    }
}