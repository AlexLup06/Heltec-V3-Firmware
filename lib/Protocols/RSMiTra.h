// Even when we receive a CTS we wait for our ctsBackoff to finish and send our CTS. if we get the packet from rts source meanwhile, then we abort ctsBackoff

#pragma once

#include <Arduino.h>
#include "MacBase.h"
#include "Messages.h"
#include "definitions.h"
#include "FSMA.h"
#include "BackoffHandler.h"
#include "RtsCtsBase.h"

class RSMiTra : public RtsCtsBase
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
        READY_TO_SEND,
        TRANSMITTING
    };

    void initProtocol() override;
    void finishProtocol() override;

    void handleWithFSM(SelfMessage *msg = nullptr) override;

    void handleUpperPacket(MessageToSend *msg) override;
    void handleProtocolPacket(ReceivedPacket *receivedPacket) override;

    void handleRTS(const BroadcastRTSPacket *packet, const size_t packetSize, bool isMission);
    void handleContinuousRTS(const BroadcastContinuousRTSPacket *packet, const size_t packetSize, bool isMission);
    void handleFragment(const BroadcastFragmentPacket *packet, const size_t packetSize, bool isMission);
    void handleCTS(const BroadcastCTS *packet, const size_t packetSize, bool isMission);

    const char *getProtocolName() override;

private:
    cFSM fsm;
};
