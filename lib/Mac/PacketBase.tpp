#pragma once
#include "PacketBase.h"

template <typename T>
void PacketBase::enqueueStruct(
    const T *packetStruct,
    size_t packetSize,
    int id,
    bool isHeader,
    bool isMission,
    bool isNodeAnnounce,
    uint8_t fullMsgChecksum,
    bool hasContinuousRTS)
{
    uint8_t *buffer = (uint8_t *)malloc(packetSize);
    memcpy(buffer, packetStruct, packetSize);

    QueuedPacket *pkt = (QueuedPacket *)malloc(sizeof(QueuedPacket));

    pkt->id=id;
    pkt->data = buffer;
    pkt->packetSize = packetSize;
    pkt->isHeader = isHeader;
    pkt->isMission = isMission;
    pkt->isNodeAnnounce = isNodeAnnounce;
    pkt->sendTrys = 0;
    pkt->hasContinuousRTS = hasContinuousRTS;
    pkt->fullMsgChecksum = fullMsgChecksum;

    customPacketQueue.enqueuePacket(pkt);
}
