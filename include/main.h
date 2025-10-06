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

// Toggle Serial Debug
// #define DEBUG_LORA_SERIAL

LoraDisplay loraDisplay;
TaskHandle_t hostTask;

int currentMac = MESH_ROUTER;

OperationalBase *macProtocol = nullptr;
MeshRouter meshRouter;

HostSerialHandlerParams_t hostSerialHandlerParams;
unsigned long lastScreenDraw = 0;
static double_t UPDATE_STATE = -1;
hw_timer_t *timer = NULL;

MeshRouter *getMeshRouter();
void onButtonPress();
void onDio1IR();
void onMacChanged(MacProtocol newMac);
void onMacFinished(MacProtocol finished);
void setup();
void loop();