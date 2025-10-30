#pragma once
#include <Arduino.h>
#include "config.h"
#include "functions.h"

enum MacProtocol
{
    MESH_ROUTER,
    CAD_ALOHA,
    ALOHA,
    CSMA,
    RS_IMITRA,
    MIRS,
    MAC_COUNT
};

extern bool isInWaitMode;

typedef void (*MacSwitchCallback)(MacProtocol newProtocol);
typedef void (*MacFinishCallback)(MacProtocol finishedProtocol);

MacProtocol getCurrentMac();
void setMacSwitchCallback(MacSwitchCallback cb);
void setMacFinishCallback(MacFinishCallback cb);
void markMacFinished();
void updateMacController();