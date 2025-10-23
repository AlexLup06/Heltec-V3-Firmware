#pragma once
#ifdef ARDUINO
#include <Arduino.h>
#endif
#include <bitset>
#include <vector>
#include <memory>
#include <cstdint>
#include <algorithm>
#include <cstring>
#include <cassert>
#include "definitions.h"

using std::vector;
using std::pair;

// mock xPortGetFreeHeapSize for native builds
#ifndef ARDUINO
extern "C" size_t xPortGetFreeHeapSize()
{
    return 200000; // simulate large heap for malloc
}
#endif

#pragma pack(1)
typedef struct FragmentedPacket
{
    uint16_t id;
    uint16_t packetSize;
    uint8_t checksum;
    uint8_t source;
    uint8_t numOfFragments;

    bool withLeaderFrag;
    bool receivedFragments[16] = {false};
    bool corrupted = false;
    uint16_t received = 0;
    uint8_t *payload;
} FragmentedPacket_t;
#pragma pack()

struct Result
{
    bool isComplete = false;
    bool sendUp = false;
    bool isMission = false;
    FragmentedPacket_t *completePacket = nullptr;
};

class IncompletePacketList
{
public:
    explicit IncompletePacketList()
    {
        packets_.reserve(10); // We do not have more than 10 nodes in the network
    }

    ~IncompletePacketList() = default;

    FragmentedPacket_t *getPacketBySource(
        const uint16_t source);
    void createIncompletePacket(
        const uint16_t id,
        const uint16_t packetSize,
        const uint8_t source,
        const uint8_t messageType,
        const uint8_t checksum);
    void removePacketBySource(
        const uint8_t source);
    Result addToIncompletePacket(
        const uint16_t id,
        const uint16_t source,
        const uint8_t fragment,
        const uint16_t payloadSize,
        const uint8_t *payload);

    void updatePacketId(
        const uint8_t sourceId,
        const uint16_t newId);
    bool isNewIdLower(
        const uint8_t sourceId,
        const uint16_t newId) const;
    bool isNewIdSame(
        const uint8_t sourceId,
        const uint16_t newId) const;
    bool isNewIdHigher(
        const uint8_t sourceId,
        const uint16_t newId) const;

    bool isCorrupted(const FragmentedPacket_t *incompletePacket, const uint8_t fragment, const uint16_t payloadSize);
    int calcOffset(const FragmentedPacket_t *incompletePacket, const uint8_t fragment);

private:
    vector<FragmentedPacket_t *> packets_;
    vector<pair<uint8_t, uint16_t>> latestIdsFromSource_;
    bool isMissionList_;
};