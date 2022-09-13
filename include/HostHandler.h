//
// Created by Julian on 25.01.2022.
//

#ifndef ESP32_LORA_TEST_HOSTHANDLER_H
#define ESP32_LORA_TEST_HOSTHANDLER_H

#include <Arduino.h>
#include "mesh/MeshRouter.h"

// Structure defining parameters for a Host receiving serial data. It notifies the system when a packet is received with the 'ready' flag. 
typedef struct {
    SerialPayloadFloodPacket_t *serialFloodPacketHeader = nullptr;
    bool ready = false;
    String *debugString;
} HostSerialHandlerParams_t;

void hostHandler(void *pvParameters);


#endif //ESP32_LORA_TEST_HOSTHANDLER_H
