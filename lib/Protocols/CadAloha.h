#pragma once

#include <Arduino.h>
#include "MacBase.h"
#include "Messages.h"
#include "definitions.h"
#include "FSMA.h"

class CadAloha : public MacBase
{
public:
    enum State
    {
        LISTENING,
        RECEIVING,
        TRANSMITTING
    };

    void handleWithFSM();
    void handleUpperPacket(MessageToSend_t *msg) override;
    void handleProtocolPacket(ReceivedPacket *receivedPacket) override;
    void handleLeaderFragment(const BroadcastLeaderFragmentPacket_t *packet, const size_t packetSize, bool isMission);
    void handleFragment(const BroadcastFragmentPacket_t *packet, const size_t packetSize, bool isMission);
    String getProtocolName();
};
