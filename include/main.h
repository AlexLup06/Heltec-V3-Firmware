#pragma once

#include <Arduino.h>
#include <RadioLib.h>
#include <config.h>

#include "HostHandler.h"
#include "mesh/loradisplay.h"
#include "mesh/MeshRouter.h"
#include "radioHandler/RadioHandler.h"
#include "MacController.h"
#include "macBase/OperationalBase.h"
#include "helpers/DataLogger.h"

// Toggle Serial Debug
// #define DEBUG_LORA_SERIAL

LoraDisplay loraDisplay;
TaskHandle_t hostTask;

int currentMac = MacProtocol::MESH_ROUTER;
OperationalBase *macProtocol = nullptr;
MeshRouter meshRouter;
DataLogger dataLogger;

HostSerialHandlerParams_t hostSerialHandlerParams;
unsigned long lastScreenDraw = 0;
static double_t UPDATE_STATE = -1;
hw_timer_t *timer = NULL;