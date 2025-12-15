#pragma once
#include <bitset>
#include <vector>
#include <memory>
#include <cstdint>
#include <algorithm>
#include <cstring>
#include <cassert>
#include <functional>
#include "definitions.h"
#include "functions.h"

using std::pair;
using std::vector;

class PacketBase;
struct FragmentedPacket
{
    uint16_t id;
    uint16_t packetSize;
    uint8_t checksum;
    uint8_t source;
    int16_t hopId;
    uint8_t numOfFragments;

    bool isMission;
    bool withLeaderFrag;
    bool receivedFragments[16] = {false};
    bool corrupted = false;
    uint16_t received = 0;
    uint8_t *payload;
};

struct Result
{
    bool isComplete = false;
    bool sendUp = false;
    bool isMission = false;
    uint16_t bytesLeft = 0;
    FragmentedPacket *completePacket = nullptr;
};

class IncompletePacketList
{
public:
    using LogFunc = std::function<void(uint16_t id, uint8_t source, int16_t hop)>;

    explicit IncompletePacketList(bool isMissionList);

    ~IncompletePacketList() = default;

    void clear();

    FragmentedPacket *getPacketBySource(const uint16_t source);
    void removePacketBySource(const uint8_t source);
    bool createIncompletePacket(
        const uint16_t id,
        const uint16_t packetSize,
        const uint8_t source,
        const int16_t hopId,
        const uint8_t messageType,
        const uint8_t checksum);
    Result addToIncompletePacket(
        const uint16_t id,
        const uint16_t source,
        const uint8_t fragment,
        const uint16_t payloadSize,
        const uint8_t *payload);

    void updatePacketId(const uint8_t sourceId, const uint16_t newId);
    bool isNewIdSame(const uint8_t sourceId, const uint16_t newId) const;
    bool isNewIdHigher(const uint8_t sourceId, const uint16_t newId) const;

    bool isCorrupted(const FragmentedPacket *incompletePacket, const uint8_t fragment, const uint16_t payloadSize);
    int calcOffset(const FragmentedPacket *incompletePacket, const uint8_t fragment);

    void setLogFragmentCallback(LogFunc func);

private:
    vector<FragmentedPacket *> packets_;
    vector<pair<uint8_t, uint16_t>> latestIdsFromSource_;
    bool isMissionList_;
    LogFunc logFragmentFunc_;
};