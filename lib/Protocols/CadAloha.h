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
    
    void finishProtocol() override;

    void handleWithFSM(SelfMessage *msg = nullptr) override;
    
    void handleUpperPacket(MessageToSend *msg) override;
    void handleProtocolPacket(ReceivedPacket *receivedPacket) override;
    void handleLeaderFragment(const BroadcastLeaderFragmentPacket *packet, const size_t packetSize, bool isMission);
    void handleFragment(const BroadcastFragmentPacket *packet, const size_t packetSize, bool isMission);
    String getProtocolName();

private:
    cFSM fsm;
};
