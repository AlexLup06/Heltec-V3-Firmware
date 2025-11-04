#pragma once

#include "MacBase.h"
#include "BackoffHandler.h"
#include "SelfMessageScheduler.h"

struct CTSData
{
    int fragmentSize = -1;
    int rtsSource = -1;
};

class RtsCtsBase : public MacBase
{
public:
protected:
    CTSData ctsData;
    int rtsSource;

    SelfMessage shortWaitTimer;
    SelfMessage waitForCTSTimer;
    SelfMessage transmissionStartTimer;
    SelfMessage transmissionEndTimer;
    SelfMessage ongoingTransmissionTimer;
    SelfMessage ctsBackoff;
    SelfMessage regularBackoff;

    bool initiateCTS = false;

    uint8_t ctsFS_MS = 18;
    uint8_t ctsCW = 16;
    BackoffHandler ctsBackoffHandler{ctsFS_MS, ctsCW, &msgScheduler, &ctsBackoff};

    uint8_t backoffFS_MS = 21;
    uint8_t backoffCW = 8;
    BackoffHandler regularBackoffHandler{backoffFS_MS, backoffCW, &msgScheduler, &regularBackoff};

    uint16_t sifs_MS = 9;

    bool isPacketFromRTSSource(ReceivedPacket *receivedPacket);
    bool isCTSForSameRTSSource(ReceivedPacket *receivedPacket);
    void sendCTS(bool waitForCTStimeout = false);
    void sendRTS();
    void clearRTSsource();
    void handleCTSTimeout();
    bool isOurCTS();
    bool withRTS(bool neighbourWithRTS = true);
    bool isFreeToSend();

    void finishRTSCTS();
    void initRTSCTS();
};