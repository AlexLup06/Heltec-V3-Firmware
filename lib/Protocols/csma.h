#pragma once

#include <Arduino.h>
#include "MacBase.h"
#include "Messages.h"
#include "definitions.h"
#include "FSMA.h"
#include "BackoffHandler.h"
#include "SelfMessage.h"

class Csma : public MacBase
{
public:
    enum State
    {
        LISTENING,
        RECEIVING,
        BACKOFF,
        TRANSMITTING
    };

    void initProtocol() override;
    void finishProtocol() override;

    void handleWithFSM(SelfMessage *msg = nullptr) override;
    void handleUpperPacket(MessageToSend *msg) override;
    void handleProtocolPacket(ReceivedPacket *receivedPacket) override;
    void handleLeaderFragment(const BroadcastLeaderFragmentPacket *packet, const size_t packetSize, bool isMission);
    void handleFragment(const BroadcastFragmentPacket *packet, const size_t packetSize, bool isMission);
    const char *getProtocolName() override;

private:
    cFSM fsm;
    SelfMessage backoff;
    BackoffHandler backoffHandler{200, 16, &msgScheduler, &backoff};
};
