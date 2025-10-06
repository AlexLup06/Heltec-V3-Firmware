#ifndef MACBASE_H
#define MACBASE_H

#include <Arduino.h>
#include "../helpers/IncompletePacketList.h"
#include "../helpers/CustomPacketQueue.h"
#include "config.h"

// Abstract base class for all MAC protocols
class MacBase
{
protected:
    // Common LoRa parameters and statistics
    uint8_t nodeId;
    uint16_t messageIdCounter;
    uint16_t missionIdCounter;

    QueuedPacket currentTransmission;

    IncompletePacketList incompleteMissionPackets;
    IncompletePacketList incompleteNeighbourPackets;
    CustomPacketQueue customPacketQueue;

public:
    MacBase(uint8_t id) : nodeId(id),
                          messageIdCounter(0),
                          missionIdCounter(0) {}
    virtual ~MacBase() {}

    void finish();
    
    void sendPacket();
    void sendRaw();
    void finishCurrentTransmission();

    void handleUpperPacket();
    virtual void handleWithFSM() = 0;
    virtual void handleLowerPacket(
        const uint8_t messageType,
        const uint8_t *packet,
        const uint8_t packetSize) = 0;

    void createNodeAnnouncePacket();
    void createBroadcastPacket(
        const uint8_t *payload,
        const uint8_t sourceId,
        const uint16_t messageSize,
        const bool withRTS,
        const bool isMission);
    void enqueuePacket(
        const uint8_t *packet,
        const uint8_t packetSize,
        const bool isHeader,
        const bool isNeighbour,
        const bool isNodeAnnounce);
    void encapsulate(
        const bool isMission,
        const uint8_t *packet);
    void decapsulate(
        const bool isMission,
        const uint8_t *packet);

    uint32_t getSendTimeByPacketSizeInUS(int packetBytes);
};

#endif // MACBASE_H