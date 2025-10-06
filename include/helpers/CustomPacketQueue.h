#pragma once
#include <deque>
#include <cstdint>
#include <stdlib.h>
#include <algorithm>

// Simple struct to represent a queued packet
struct QueuedPacket {
    uint8_t* data;      // pointer to packet buffer
    uint8_t packetSize;       // size of the packet
    bool isHeader;      // optional flag for logic control
    bool isMission;   // optional flag for logic control
    bool isNodeAnnounce; // optional flag
};

class CustomPacketQueue {
public:
    CustomPacketQueue();
    ~CustomPacketQueue();

    bool enqueuePacket(QueuedPacket* pkt);
    void enqueuePacketAtPosition(QueuedPacket* pkt, int pos);
    void enqueueNodeAnnounce(QueuedPacket* pkt);
    QueuedPacket* dequeuePacket();
    void removePacketAtPosition(int pos);
    void removePacket(QueuedPacket* pkt);
    bool isEmpty() const;
    int size() const;

private:
    std::deque<QueuedPacket*> packetQueue;
};