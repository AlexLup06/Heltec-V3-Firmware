#pragma once

#include <cstdint>
#include <cstdlib>
#include <cstring>
#include "CustomPacketQueue.h"
#include "definitions.h"
#include "messages.h"
#include "functions.h"
#include "IncompletePacketList.h"

class PacketBase
{
private:
    uint16_t MISSION_ID_COUNT = 0;
    uint16_t NEIGHBOUR_ID_COUNT = 0;

    CustomPacketQueue customPacketQueue;

    IncompletePacketList incompleteMissionPackets;
    IncompletePacketList incompleteNeighbourPackets;

    template <typename T>
    void enqueueStruct(const T &packetStruct,
                       bool isHeader,
                       bool isMission,
                       bool isNodeAnnounce);

    BroadcastRTSPacket_t createRTS(uint16_t packetId, uint8_t sourceId, uint16_t payloadSize, uint8_t checksum);
    BroadcastLeaderFragmentPacket_t createLeaderFragment(
        uint16_t packetId,
        uint16_t source,
        uint8_t checksum,
        const uint8_t *payload,
        uint16_t size,
        uint16_t leaderPayloadSize);
    BroadcastFragmentPacket_t createFragment(uint16_t packetId, uint8_t fragmentId, const uint8_t *payload, uint16_t payloadSize, bool hasLeaderFrag);

public:
    PacketBase();
    ~PacketBase();

    void clearQueue();
    QueuedPacket *dequeuePacket();

    BroadcastCTS_t createCTS(uint16_t size);
    NodeIdAnnounce_t createNodeAnnouncePacket(const uint8_t *mac, uint8_t nodeId);
    void createMessage(const uint8_t *payload, uint16_t payloadSize, int source, bool withRTS, bool isMission);

    void createIncompletePacket(
        const uint16_t id,
        const uint16_t size,
        const uint8_t source,
        const uint8_t messageType,
        const uint8_t checksum,
        bool isMission);
    Result addToIncompletePacket(
        const uint16_t id,
        const uint16_t source,
        const uint8_t fragment,
        const uint16_t payloadSize,
        const uint8_t *payload,
        bool isMission,
        bool isLeaderFragment);

    void encapsulate(MessageTypeBase &msg);
    bool decapsulate(uint8_t *packet);
};
