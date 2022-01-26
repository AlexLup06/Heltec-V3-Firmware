#include "HostHandler.h"
#include <Arduino.h>
#include "mesh/MeshRouter.h"

#define SERIAL_REC_HEADER 0
#define SERIAL_REC_PAYLOAD 1
#define SERIAL_WAIT_PROCESS 2

uint8_t serialStatus = SERIAL_REC_HEADER;
uint16_t serialIndex = 0;

HostSerialHandlerParams_t *hostHandlerParams;

void serialReceive() {
    int availableBytes = Serial.available();
    if (availableBytes) {

        if (serialStatus == SERIAL_REC_HEADER && availableBytes >= 5) {
            hostHandlerParams->serialPaket = (SerialPaket_t *) malloc(sizeof(SerialPaket_t));
            Serial.read((uint8_t *) hostHandlerParams->serialPaket, 5);

            *hostHandlerParams->debugString = String("H S:") + String(hostHandlerParams->serialPaket->size);
            hostHandlerParams->serialPaket->payload = (uint8_t *) malloc(hostHandlerParams->serialPaket->size);
            serialStatus = SERIAL_REC_PAYLOAD;
            availableBytes = Serial.available();
        }


        if (serialStatus == SERIAL_REC_PAYLOAD && availableBytes) {
            int bytesToRead =
                    availableBytes + serialIndex > hostHandlerParams->serialPaket->size ?
                    hostHandlerParams->serialPaket->size - serialIndex
                                                                                        : availableBytes;
            serialIndex += Serial.read(hostHandlerParams->serialPaket->payload + serialIndex, bytesToRead);


            *hostHandlerParams->debugString =
                    String(serialIndex) + "/" + String(hostHandlerParams->serialPaket->size) + " " +
                    String(availableBytes);


            if (serialIndex >= hostHandlerParams->serialPaket->size) {

                // Paket completed!
                *hostHandlerParams->debugString = String("Rec! S:") + String(hostHandlerParams->serialPaket->size);


                if (Serial.available()) {
                    // Debug
                    *hostHandlerParams->debugString = String("!!! ") + String(Serial.available());
                } else {
                    // Mark Paket as ready
                    hostHandlerParams->ready = true;
                    serialStatus = SERIAL_WAIT_PROCESS;
                    serialIndex = 0;
                }
            }
        }
    }

    if (serialStatus == SERIAL_WAIT_PROCESS && !hostHandlerParams->ready) {
        // Paket Processed!
        hostHandlerParams->serialPaket = nullptr;
        serialStatus = SERIAL_REC_HEADER;
    }
}

void hostHandler(void *pvParameters) {
    hostHandlerParams = (HostSerialHandlerParams_t *) pvParameters;

    while (true) {
        serialReceive();
        yield();
    }
}