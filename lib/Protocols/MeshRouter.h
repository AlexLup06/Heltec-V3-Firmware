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

    void handleWithFSM(SelfMessage *msg = nullptr) override;

    void finishProtocol() override;

    void handleProtocolPacket(ReceivedPacket *receivedPacket) override;
    void OnFloodHeaderPacket(BroadcastRTSPacket *packet, size_t packetSize, bool isMission);
    void OnFloodFragmentPacket(BroadcastFragmentPacket *packet, size_t packetSize, bool isMission);

    void handleUpperPacket(MessageToSend *serialPayloadFloodPacket) override;
    const char *getProtocolName() override;

protected:
    void onPreambleDetectedIR() override;

private:
    long predictPacketSendTime(uint8_t size);
    void SenderWait(unsigned long delay);
};