#pragma once
#include <Arduino.h>
#include <deque>
#include <cstdint>
#include <stdlib.h>
#include <algorithm>
#include <functions.h>

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

    const QueuedPacket *getFirstPacket() const;
    QueuedPacket *dequeuePacket();
    void removePacketAtPosition(int pos);
    void removePacket(QueuedPacket *pkt);

    bool isEmpty() const;
    int size() const;
    void printQueue();

private:
    std::deque<QueuedPacket *> packetQueue;
};