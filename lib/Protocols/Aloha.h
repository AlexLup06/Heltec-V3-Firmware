#pragma once

#include <Arduino.h>
#include "MacBase.h"
#include "Messages.h"
#include "definitions.h"
#include "FSMA.h"

class Aloha : public MacBase
{
protected:
    void onCRCerrorIR() override;

public:
    enum State
    {
        LISTENING,
        RECEIVING,
        TRANSMITTING
    };

    cFSM fsm;

    void initProtocol() override;
    void handleWithFSM();
    void clearMacData();
    void onPreambleDetectedIR() override;
    void handleUpperPacket(MessageToSend_t *msg) override;
    void handleProtocolPacket(
        const uint8_t messageType,
        const uint8_t *packet,
        const size_t packetSize,
        const int rssi,
        bool isMission) override;
    void handleLeaderFragment(const BroadcastLeaderFragmentPacket_t *packet, const size_t packetSize, bool isMission);
    void handleFragment(const BroadcastFragmentPacket_t *packet, const size_t packetSize, bool isMission);
    String getProtocolName();
};
