#pragma once
#include "PacketBase.h"

template <typename T>
void PacketBase::enqueueStruct(const T &packetStruct,
                               bool isHeader,
                               bool isMission,
                               bool isNodeAnnounce)
{
    uint8_t *buffer = (uint8_t *)malloc(sizeof(T));
    memcpy(buffer, &packetStruct, sizeof(T));

    QueuedPacket *pkt = new QueuedPacket;
    pkt->data = buffer;
    pkt->packetSize = sizeof(T);
    pkt->isHeader = isHeader;
    pkt->isMission = isMission;
    pkt->isNodeAnnounce = isNodeAnnounce;

    customPacketQueue.enqueuePacket(pkt);
}
