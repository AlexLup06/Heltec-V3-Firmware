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

private:
    uint8_t ctsFS_MS = 18;
    uint8_t ctsCW = 16;

    uint8_t backoffFS_MS = 21;
    uint8_t backoffCW = 16;

protected:
    CTSData ctsData;
    int rtsSource;
    uint16_t sifs_MS = 9;
    bool initiateCTS = false;

    SelfMessage shortWaitTimer;
    SelfMessage waitForCTSTimer;
    SelfMessage transmissionStartTimer;
    SelfMessage transmissionEndTimer;
    SelfMessage ongoingTransmissionTimer;
    SelfMessage ctsBackoff;
    SelfMessage regularBackoff;

    BackoffHandler ctsBackoffHandler{ctsFS_MS, ctsCW, &msgScheduler, &ctsBackoff};
    BackoffHandler regularBackoffHandler{backoffFS_MS, backoffCW, &msgScheduler, &regularBackoff};

    void handleCTSTimeout();
    void sendCTS(bool waitForCTStimeout = false);
    void sendRTS();
    void clearRTSsource();

    bool isPacketFromRTSSource(ReceivedPacket *receivedPacket);
    bool isCTSForSameRTSSource(ReceivedPacket *receivedPacket);
    bool isOurCTS();
    bool isWithRTS(bool neighbourWithRTS = true);
    bool isFreeToSend();

    void finishRTSCTS();
    void initRTSCTS();
};