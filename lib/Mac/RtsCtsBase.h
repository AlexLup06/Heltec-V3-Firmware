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

protected:
    uint8_t ctsFS_MS = 16 + 3;
    uint8_t ctsCW = 16;

    uint8_t backoffFS_MS = 19 + 3;
    uint8_t backoffCW = 16;

    uint16_t sifs_MS = 3;
    CTSData ctsData;
    int rtsSource;
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

    void handleCTSTimeout(bool withRetry);
    void sendCTS(bool instantFragment = true);
    void sendRTS();
    void clearRTSsource();

    bool isPacketFromRTSSource(ReceivedPacket *receivedPacket);
    bool isCTSForSameRTSSource(ReceivedPacket *receivedPacket);
    bool isOurCTS();
    bool isWithRTS(bool neighbourWithRTS = true);
    bool isFreeToSend();

    void finishRTSCTS();
    void initRTSCTS();

    bool isStrayCTS(ReceivedPacket *receivedPacket);
    void handleStrayCTS(ReceivedPacket *receivedPacket, bool instantFragment);

    bool isRTS(ReceivedPacket *receivedPacket);
    void handleUnhandeledRTS();

    bool isPacketNotFromRTSSource(ReceivedPacket *receivedPacket);

    void handleCTS(const BroadcastCTS *cts, const size_t packetSize, bool instantFragment);
};