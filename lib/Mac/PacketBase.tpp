#pragma once
#include "PacketBase.h"

template <typename T>
void PacketBase::enqueueStruct(
    const T *packetStruct,
    size_t packetSize,
    uint8_t source,
    int id,
    uint8_t fullMsgChecksum,
    bool isHeader,
    bool isMission,
    bool isNodeAnnounce,
    bool hasContinuousRTS)
{
    uint8_t *buffer = (uint8_t *)malloc(packetSize);
    memcpy(buffer, packetStruct, packetSize);

    QueuedPacket *pkt = (QueuedPacket *)malloc(sizeof(QueuedPacket));

    pkt->data = buffer;
    pkt->packetSize = packetSize;
    pkt->id = id;
    pkt->source = nodeId;
    pkt->sendTrys = 0;
    pkt->fullMsgChecksum = fullMsgChecksum;
    pkt->isHeader = isHeader;
    pkt->isMission = isMission;
    pkt->isNodeAnnounce = isNodeAnnounce;
    pkt->hasContinuousRTS = hasContinuousRTS;

    customPacketQueue.enqueuePacket(pkt);
}
