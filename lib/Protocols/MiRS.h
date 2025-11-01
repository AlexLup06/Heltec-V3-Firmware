// Even when we receive a CTS we wait for our ctsBackoff to finish and send our CTS. if we get the packet from rts source meanwhile, then we abort ctsBackoff

#pragma once

#include <Arduino.h>
#include "MacBase.h"
#include "Messages.h"
#include "definitions.h"
#include "FSMA.h"
#include "BackoffHandler.h"
#include "OneShotTimer.h"
#include "RtsCtsBase.h"

class MiRS : public RtsCtsBase
{
public:
    enum State
    {
        LISTENING,
        RECEIVING,
        BACKOFF,
        SEND_RTS,
        WAIT_CTS,
        CW_CTS,
        SEND_CTS,
        AWAIT_TRANSMISSION,
        TRANSMITTING
    };

    void handleWithFSM();

    void handleUpperPacket(MessageToSend_t *msg) override;
    void handleProtocolPacket(ReceivedPacket *receivedPacket) override;

    void handleRTS(const BroadcastRTSPacket_t *packet, const size_t packetSize, bool isMission);
    void handleLeaderFragment(const BroadcastLeaderFragmentPacket_t *packet, const size_t packetSize, bool isMission);
    void handleContinuousRTS(const BroadcastContinuousRTSPacket_t *packet, const size_t packetSize, bool isMission);
    void handleFragment(const BroadcastFragmentPacket_t *packet, const size_t packetSize, bool isMission);
    void handleCTS(const BroadcastCTS_t *packet, const size_t packetSize, bool isMission);
    
    String getProtocolName();

private:
    uint8_t backoffFS_MS = 21;
    uint8_t backoffCW = 8;
    Backoff regularBackoff{backoffFS_MS, backoffCW};
};
