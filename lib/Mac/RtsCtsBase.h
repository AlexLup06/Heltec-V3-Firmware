#pragma once

#include "MacBase.h"
#include "BackoffHandler.h"
#include "OneShotTimer.h"

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

    OneShotTimer shortWaitTimer;
    OneShotTimer waitForCTSTimer;
    OneShotTimer initiateCTSTimer;
    OneShotTimer transmissionStartTimer;
    OneShotTimer transmissionEndTimer;
    OneShotTimer ongoingTransmissionTimer;

    uint8_t ctsFS_MS = 18;
    uint8_t ctsCW = 16;

    uint16_t sifs_MS = 2;
    Backoff ctsBackoff{ctsFS_MS, ctsCW};

    bool isPacketFromRTSSource(ReceivedPacket *receivedPacket);
    bool isCTSForSameRTSSource(ReceivedPacket *receivedPacket);
    void sendCTS();
    void sendRTS();
    void clearRTSsource();
    void handleCTSTimeout();
    bool isOurCTS();
    bool withRTS(bool neighbourWithRTS = true);
    bool isFreeToSend();
};