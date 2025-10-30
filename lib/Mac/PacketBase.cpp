#include "PacketBase.h"

PacketBase::PacketBase() {}
PacketBase::~PacketBase() { clearQueue(); }

bool PacketBase::createIncompletePacket(
    const uint16_t id,
    const uint16_t size,
    const uint8_t source,
    const uint8_t messageType,
    const uint8_t checksum,
    bool isMission)
{
    if (isMission)
    {
        return incompleteMissionPackets.createIncompletePacket(id, size, source, messageType, checksum);
    }
    else
    {
        return incompleteNeighbourPackets.createIncompletePacket(id, size, source, messageType, checksum);
    }
}

Result PacketBase::addToIncompletePacket(
    const uint16_t id,
    const uint16_t source,
    const uint8_t fragment,
    const uint16_t packetSize,
    const uint8_t *payload,
    bool isMission,
    bool isLeaderFragment)
{
    uint16_t payloadSize = isLeaderFragment ? packetSize - BROADCAST_LEADER_FRAGMENT_METADATA_SIZE : packetSize - BROADCAST_FRAGMENT_METADATA_SIZE;
    if (isMission)
    {
        return incompleteMissionPackets.addToIncompletePacket(id, source, fragment, payloadSize, payload);
    }
    else
    {
        return incompleteNeighbourPackets.addToIncompletePacket(id, source, fragment, payloadSize, payload);
    }
}

bool PacketBase::doesIncompletePacketExist(const uint8_t sourceId, const uint16_t id, bool isMission)
{
    if (isMission)
    {
        return incompleteMissionPackets.doesIncompletePacketExist(sourceId, id);
    }
    else
    {
        return incompleteNeighbourPackets.doesIncompletePacketExist(sourceId, id);
    }
}

bool PacketBase::dequeuedPacketWasLast()
{
    auto *firstPacket = customPacketQueue.getFirstPacket();
    if (firstPacket == nullptr)
        return true;
    return firstPacket->isHeader || firstPacket->isNodeAnnounce;
}

// ---------------------- Packet Creation ----------------------

void PacketBase::createNodeAnnouncePacket(const uint8_t *mac, uint8_t nodeId)
{
    BroadcastNodeIdAnnounce_t *pkt = (BroadcastNodeIdAnnounce_t *)malloc(sizeof(BroadcastNodeIdAnnounce_t));
    pkt->messageType = MESSAGE_TYPE_BROADCAST_NODE_ANNOUNCE;
    pkt->nodeId = nodeId;
    pkt->respond = 0;
    memcpy(pkt->deviceMac, mac, 6);
    enqueueStruct(pkt, sizeof(BroadcastNodeIdAnnounce_t), false, false, true);
    free(pkt);
}

void PacketBase::createMessage(const uint8_t *payload, uint16_t payloadSize, int source, bool withRTS, bool isMission)
{
    uint16_t msgId = isMission ? MISSION_ID_COUNT++ : NEIGHBOUR_ID_COUNT++;
    uint8_t checksum = crc8(payload, payloadSize);

    if (withRTS)
    {
        BroadcastRTSPacket_t *rts = createRTS(msgId, source, payloadSize, checksum);
        if (isMission)
            encapsulate(rts);
        enqueueStruct(rts, sizeof(BroadcastRTSPacket_t), true, isMission, false);
        free(rts);

        uint8_t fragmentId = 0;
        while (payloadSize > 0)
        {
            uint16_t packetPayloadSize = std::min<uint16_t>(payloadSize, LORA_MAX_FRAGMENT_PAYLOAD);
            uint16_t packetSize = packetPayloadSize + BROADCAST_FRAGMENT_METADATA_SIZE;
            BroadcastFragmentPacket_t *fragment = createFragment(msgId, fragmentId++, payload, packetPayloadSize, source, false);
            if (isMission)
                encapsulate(fragment);
            enqueueStruct(fragment, packetSize, false, isMission, false);
            free(fragment);

            payload += packetPayloadSize;
            payloadSize -= packetPayloadSize;
        }
    }
    else
    {
        uint16_t leaderPayloadSize = std::min<uint16_t>(payloadSize, LORA_MAX_FRAGMENT_LEADER_PAYLOAD);
        uint16_t leaderPacketSize = leaderPayloadSize + BROADCAST_LEADER_FRAGMENT_METADATA_SIZE;
        BroadcastLeaderFragmentPacket_t *leaderFragment = createLeaderFragment(msgId, source, checksum, payload, payloadSize, leaderPayloadSize);
        if (isMission)
            encapsulate(leaderFragment);
        enqueueStruct(leaderFragment, leaderPacketSize, true, isMission, false);
        free(leaderFragment);

        payload += leaderPayloadSize;
        payloadSize -= leaderPayloadSize;

        uint8_t fragmentId = 1;
        while (payloadSize > 0)
        {
            uint16_t packetPayloadSize = std::min<uint16_t>(payloadSize, LORA_MAX_FRAGMENT_PAYLOAD);
            uint16_t packetSize = packetPayloadSize + BROADCAST_FRAGMENT_METADATA_SIZE;
            BroadcastFragmentPacket_t *fragment = createFragment(msgId, fragmentId++, payload, packetPayloadSize, source, true);
            if (isMission)
                encapsulate(fragment);
            enqueueStruct(fragment, packetSize, false, isMission, false);
            free(fragment);

            payload += packetPayloadSize;
            payloadSize -= packetPayloadSize;
        }
    }
}

BroadcastRTSPacket_t *PacketBase::createRTS(uint16_t packetId, uint8_t sourceId, uint16_t payloadSize, uint8_t checksum)
{
    BroadcastRTSPacket_t *pkt = (BroadcastRTSPacket_t *)malloc(sizeof(BroadcastRTSPacket_t));
    pkt->messageType = MESSAGE_TYPE_BROADCAST_RTS;
    pkt->source = sourceId;
    pkt->id = packetId;
    pkt->size = payloadSize;
    pkt->checksum = checksum;
    return pkt;
}

BroadcastCTS_t *PacketBase::createCTS(uint16_t size)
{
    BroadcastCTS_t *pkt = (BroadcastCTS_t *)malloc(sizeof(BroadcastCTS_t));
    pkt->messageType = MESSAGE_TYPE_BROADCAST_CTS;
    pkt->sizeOfFragment = size;
    return pkt;
}

BroadcastLeaderFragmentPacket_t *PacketBase::createLeaderFragment(uint16_t packetId, uint16_t source, uint8_t checksum, const uint8_t *payload, uint16_t size, uint16_t leaderPayloadSize)
{
    BroadcastLeaderFragmentPacket_t *pkt = (BroadcastLeaderFragmentPacket_t *)malloc(sizeof(BroadcastLeaderFragmentPacket_t));
    pkt->messageType = MESSAGE_TYPE_BROADCAST_LEADER_FRAGMENT;
    pkt->source = source;
    pkt->id = packetId;
    pkt->size = size;
    pkt->checksum = checksum;

    // Copy only the part that is actually used
    memcpy(pkt->payload, payload, leaderPayloadSize);
    return pkt;
}

BroadcastFragmentPacket_t *PacketBase::createFragment(uint16_t packetId, uint8_t fragmentId, const uint8_t *payload, size_t payloadSize, uint8_t source, bool hasLeaderFrag)
{
    BroadcastFragmentPacket_t *pkt = (BroadcastFragmentPacket_t *)malloc(sizeof(BroadcastFragmentPacket_t));
    pkt->messageType = MESSAGE_TYPE_BROADCAST_FRAGMENT;
    pkt->source = source;
    pkt->id = packetId;
    pkt->fragment = fragmentId;

    // Compute where this fragment starts in the original payload
    uint16_t leaderFragPayloadSize = hasLeaderFrag ? LORA_MAX_FRAGMENT_LEADER_PAYLOAD : 0;
    uint16_t offset = (hasLeaderFrag ? (fragmentId - 1) : fragmentId) * LORA_MAX_FRAGMENT_PAYLOAD + leaderFragPayloadSize;

    memcpy(pkt->payload, payload + offset, payloadSize);
    return pkt;
}

// ---------------------- Queue Management ----------------------

void PacketBase::clearQueue()
{
    while (!customPacketQueue.isEmpty())
    {
        QueuedPacket *pkt = customPacketQueue.dequeuePacket();
        if (pkt)
        {
            free(pkt->data);
        }
    }
}

QueuedPacket *PacketBase::dequeuePacket()
{
    return customPacketQueue.dequeuePacket();
}

// ---------------------- Encapsulation / Decapsulation ----------------------

void PacketBase::encapsulate(MessageTypeBase *msg)
{
    msg->setIsMission();
}

bool PacketBase::decapsulate(uint8_t *packet)
{
    bool isMission = packet[0] & 0x80;
    packet[0] = packet[0] & 0x7F;
    return isMission;
}
