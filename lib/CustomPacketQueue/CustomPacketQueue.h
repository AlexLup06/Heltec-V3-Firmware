#pragma once
#include <deque>
#include <cstdint>
#include <stdlib.h>
#include <algorithm>

struct QueuedPacket
{
    uint8_t *data;
    size_t packetSize;
    uint8_t source;
    uint16_t id;
    uint8_t sendTrys;
    uint8_t fullMsgChecksum;
    bool isHeader;
    bool isMission;
    bool isNodeAnnounce;
    bool hasContinuousRTS;
};

class CustomPacketQueue
{
public:
    CustomPacketQueue();
    ~CustomPacketQueue();

    bool enqueuePacket(QueuedPacket *pkt);
    void enqueuePacketAtPosition(QueuedPacket *pkt, int pos);
    void enqueueNodeAnnounce(QueuedPacket *pkt);
    QueuedPacket *dequeuePacket();
    const QueuedPacket *getFirstPacket() const;
    void removePacketAtPosition(int pos);
    void removePacket(QueuedPacket *pkt);
    bool isEmpty() const;
    int size() const;

private:
    std::deque<QueuedPacket *> packetQueue;
};