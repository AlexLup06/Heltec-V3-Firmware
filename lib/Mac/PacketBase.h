#pragma once

#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <iostream>
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

    uint8_t nodeId;

    IncompletePacketList incompleteMissionPackets{true};
    IncompletePacketList incompleteNeighbourPackets;

    template <typename T>
    void enqueueStruct(
        const T *packetStruct,
        size_t packetSize,
        int id,
        bool isHeader,
        bool isMission,
        bool isNodeAnnounce,
        uint8_t fullMsgChecksum,
        bool hasContinuousRTS = false);

    BroadcastLeaderFragmentPacket *createLeaderFragment(
        uint16_t packetId,
        uint16_t source,
        uint8_t checksum,
        const uint8_t *payload,
        uint16_t size,
        uint16_t leaderPayloadSize,
        bool isMission);
    BroadcastFragmentPacket *createFragment(
        uint16_t packetId,
        uint8_t fragmentId,
        const uint8_t *payload,
        size_t payloadSize,
        uint8_t source,
        bool hasLeaderFrag,
        bool isMission);

public:
    PacketBase(uint8_t _nodeId) : nodeId(_nodeId) {}
    ~PacketBase();

    CustomPacketQueue customPacketQueue;

    void clearQueue();
    void clearIncompletePacketLists();
    QueuedPacket *dequeuePacket();

    BroadcastRTSPacket *createRTS(uint16_t packetId, uint8_t sourceId, uint16_t payloadSize, uint8_t checksum, bool isMission);
    BroadcastContinuousRTSPacket *createContinuousRTS(uint16_t packetId, uint16_t source, uint8_t fragmentSize, bool isMission);
    BroadcastCTS *createCTS(uint8_t fragmentSize, uint8_t rtsSource);
    void createNodeAnnouncePacket(uint8_t nodeId);
    void createMessage(const uint8_t *payload, uint16_t payloadSize, uint8_t source, bool withRTS, bool isMission, bool withContinuousRTS, int id = -1);
    bool doesIncompletePacketExist(const uint8_t sourceId, const uint16_t id, bool isMission);
    bool dequeuedPacketWasLast();
    QueuedPacket *createNewContinuousRTS(QueuedPacket *queuedPacket);
    QueuedPacket *createNewRTS(QueuedPacket *queuedPacket);
    void removeIncompletePacket(uint8_t source, bool isMission);

    FragmentedPacket *getIncompletePacketBySource(uint8_t source, bool isMission);

    bool createIncompletePacket(
        const uint16_t id,
        const uint16_t size,
        const uint8_t source,
        const int16_t hopId,
        const uint8_t messageType,
        const uint8_t checksum,
        bool isMission);
    Result addToIncompletePacket(
        const uint16_t id,
        const uint16_t source,
        const uint8_t fragment,
        const uint16_t packetSize,
        const uint8_t *payload,
        bool isMission,
        bool isLeaderFragment);

    void encapsulate(MessageTypeBase *msg, bool isMission);
    bool decapsulate(uint8_t *packet);
};

#include "PacketBase.tpp"