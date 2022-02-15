//
// Created by Julian on 25.01.2022.
//

#ifndef ESP32_LORA_TEST_HOSTHANDLER_H
#define ESP32_LORA_TEST_HOSTHANDLER_H

#include <Arduino.h>
#include "mesh/MeshRouter.h"

typedef struct {
    SerialPayloadFloodPaket_t *serialFloodPaketHeader = nullptr;
    bool ready = false;
    String *debugString;
} HostSerialHandlerParams_t;

void hostHandler(void *pvParameters);


#endif //ESP32_LORA_TEST_HOSTHANDLER_H
