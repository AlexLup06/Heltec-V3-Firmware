//
// Created by Julian on 25.01.2022.
//

#pragma once

#include <Arduino.h>
#include "mesh/MeshRouter.h"
#include "macBase/OperationalBase.h"

// Structure defining parameters for a Host receiving serial data. It notifies the system when a packet is received with the 'ready' flag. 
typedef struct {
    SerialPayloadFloodPacket_t *serialFloodPacketHeader = nullptr;
    bool ready = false;
    String *debugString;
    OperationalBase *macProtocol;
} HostSerialHandlerParams_t;

void hostHandler(void *pvParameters);
