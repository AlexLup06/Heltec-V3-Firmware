#pragma once

#include <Arduino.h>
#include "config.h"
#include "MacBase.h"
#include "Messages.h"
#include "definitions.h"

class MeshRouter : public MacBase
{
public:
    unsigned long blockSendUntil = 0; // Blocks senders until previous message sent
    unsigned long preambleAdd = 0;    // Increases blocking time causing the other senders to wait

    void handleWithFSM() override;

    void handleProtocolPacket(const uint8_t messageType, const uint8_t *packet, const size_t packetSize, const int rssi, bool isMission) override;
    void OnFloodHeaderPacket(BroadcastRTSPacket_t *packet, size_t packetSize, bool isMission);
    void OnFloodFragmentPacket(BroadcastFragmentPacket_t *packet, size_t packetSize, bool isMission);

    void handleUpperPacket(MessageToSend_t *serialPayloadFloodPacket) override;
    String getProtocolName() override;

protected:
    void onPreambleDetectedIR() override;
    void onCRCerrorIR() override;

private:
    long predictPacketSendTime(uint8_t size);
    void SenderWait(unsigned long delay);
};