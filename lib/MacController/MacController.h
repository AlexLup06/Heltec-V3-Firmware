#pragma once
#include <Arduino.h>

enum MacProtocol {
    MESH_ROUTER,
    CAD_ALOHA,
    ALOHA,
    CSMA,
    RS_IMITRA,
    MIRS,
    MAC_COUNT
};

typedef void (*MacSwitchCallback)(MacProtocol newProtocol);
typedef void (*MacFinishCallback)(MacProtocol finishedProtocol);

MacProtocol getCurrentMac();
void switchToMac(MacProtocol next);
void setMacSwitchCallback(MacSwitchCallback cb);
void setMacFinishCallback(MacFinishCallback cb);
void markMacFinished();
void updateMacController(bool isInConfigMode);