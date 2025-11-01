#pragma once

#include <Arduino.h>
#include "MacBase.h"
#include "Messages.h"
#include "definitions.h"
#include "FSMA.h"
#include "BackoffHandler.h"

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

    void handleWithFSM();
    void handleUpperPacket(MessageToSend_t *msg) override;
    void handleProtocolPacket(ReceivedPacket *receivedPacket) override;
    void handleLeaderFragment(const BroadcastLeaderFragmentPacket_t *packet, const size_t packetSize, bool isMission);
    void handleFragment(const BroadcastFragmentPacket_t *packet, const size_t packetSize, bool isMission);
    String getProtocolName();

protected:

private:
    Backoff backoff{200, 8};
};
