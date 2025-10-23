#pragma once

#include <Arduino.h>
#include "MacBase.h"
#include "Messages.h"
#include "definitions.h"
#include "FSMA.h"

class Aloha : public MacBase
{
public:
    enum State
    {
        LISTENING,
        RECEIVING,
        TRANSMITTING
    };

    cFSM fsm;

    void handleWithFSM();
    void init();
    void clearMacData();
    void setNodeID();
    void onPreambleDetectedIR() override;
    void handleProtocolPacket(
        const uint8_t messageType,
        const uint8_t *packet,
        const uint8_t packetSize,
        const int rssi);

    void handleLeaderFragment(
        const uint8_t *packet,
        const uint8_t packetSize);
    void handleFragment(
        const uint8_t *packet,
        const uint8_t packetSize);
};
