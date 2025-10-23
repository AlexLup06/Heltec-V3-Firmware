#pragma once

#include <cstdint>
#include "CustomPacketQueue.h"

class PacketBase
{
private:
    CustomPacketQueue customPacketQueue;

public:
    PacketBase();
    ~PacketBase();

    void createRTS();
    void createCTS();
    void createLeaderFragment();
    void createFragment();

    void clearQueue();
    QueuedPacket dequeuePacket();

    void createNodeAnnouncePacket();
    void enqueuePacket(const uint8_t *packet, const uint8_t packetSize, const bool isHeader, const bool isNeighbour, const bool isNodeAnnounce);

    void encapsulate(const bool isMission, const uint8_t *packet);
    void decapsulate(const bool isMission, const uint8_t *packet);
};
