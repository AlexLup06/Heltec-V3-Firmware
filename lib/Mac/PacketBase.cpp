#include "PacketBase.h"
#include <iostream>

PacketBase::PacketBase() {}
PacketBase::~PacketBase() { clearQueue(); }

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

void PacketBase::createIncompletePacket(
    const uint16_t id,
    const uint16_t size,
    const uint8_t source,
    const uint8_t messageType,
    const uint8_t checksum,
    bool isMission)
{
    if (isMission)
    {
        incompleteMissionPackets.createIncompletePacket(id, size, source, messageType, checksum);
    }
    else
    {
        incompleteNeighbourPackets.createIncompletePacket(id, size, source, messageType, checksum);
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

// ---------------------- Packet Creation ----------------------

void PacketBase::createMessage(const uint8_t *payload, uint16_t payloadSize, int source, bool withRTS, bool isMission)
{
    uint16_t msgId = isMission ? MISSION_ID_COUNT++ : NEIGHBOUR_ID_COUNT++;
    uint8_t checksum = crc8(payload, payloadSize);

    bool toMissionQueue = isMission;
    bool toNeighbourQueue = !isMission;

    if (withRTS)
    {
        BroadcastRTSPacket_t rts = createRTS(msgId, source, payloadSize, checksum);
        encapsulate(rts);
        enqueueStruct(rts, true, toMissionQueue, toNeighbourQueue);

        uint8_t fragmentId = 0;
        while (payloadSize > 0)
        {
            uint16_t packetPayloadSize = std::min<uint16_t>(payloadSize, LORA_MAX_FRAGMENT_PAYLOAD);
            BroadcastFragmentPacket_t fragment = createFragment(msgId, fragmentId++, payload, packetPayloadSize, false);
            encapsulate(fragment);
            enqueueStruct(fragment, false, toMissionQueue, toNeighbourQueue);

            payload += packetPayloadSize;
            payloadSize -= packetPayloadSize;
        }
    }
    else
    {
        uint16_t leaderPayloadSize = std::min<uint16_t>(payloadSize, LORA_MAX_FRAGMENT_LEADER_PAYLOAD);
        BroadcastLeaderFragmentPacket_t leaderFragment =
            createLeaderFragment(msgId, source, checksum, payload, payloadSize, leaderPayloadSize);
        encapsulate(leaderFragment);
        enqueueStruct(leaderFragment, true, toMissionQueue, toNeighbourQueue);

        payload += leaderPayloadSize;
        payloadSize -= leaderPayloadSize;

        uint8_t fragmentId = 1;
        while (payloadSize > 0)
        {
            uint16_t packetPayloadSize = std::min<uint16_t>(payloadSize, LORA_MAX_FRAGMENT_PAYLOAD);
            BroadcastFragmentPacket_t fragment = createFragment(msgId, fragmentId++, payload, packetPayloadSize, true);
            encapsulate(fragment);
            enqueueStruct(fragment, false, toMissionQueue, toNeighbourQueue);

            payload += packetPayloadSize;
            payloadSize -= packetPayloadSize;
        }
    }
}

BroadcastRTSPacket_t PacketBase::createRTS(uint16_t packetId, uint8_t sourceId, uint16_t payloadSize, uint8_t checksum)
{
    BroadcastRTSPacket_t pkt;
    pkt.id = packetId;
    pkt.source = sourceId;
    pkt.size = payloadSize;
    pkt.checksum = checksum;
    return pkt;
}

BroadcastCTS_t PacketBase::createCTS(uint16_t size)
{
    BroadcastCTS_t pkt;
    pkt.sizeOfFragment = size;
    return pkt;
}

BroadcastLeaderFragmentPacket_t PacketBase::createLeaderFragment(uint16_t packetId, uint16_t source, uint8_t checksum, const uint8_t *payload, uint16_t size, uint16_t leaderPayloadSize)
{
    BroadcastLeaderFragmentPacket_t pkt;
    pkt.id = packetId;
    pkt.source = source;
    pkt.size = size;
    pkt.checksum = checksum;

    memcpy(pkt.payload, payload, leaderPayloadSize);
    return pkt;
}

BroadcastFragmentPacket_t PacketBase::createFragment(uint16_t packetId, uint8_t fragmentId, const uint8_t *payload, uint16_t payloadSize, bool hasLeaderFrag)
{
    BroadcastFragmentPacket_t pkt;
    pkt.id = packetId;
    pkt.fragment = fragmentId;

    uint16_t leaderFragPayloadSize = hasLeaderFrag ? 247 : 0;
    uint16_t offset = (fragmentId - 1) * 251 + leaderFragPayloadSize;
    memcpy(pkt.payload, payload + offset, payloadSize);
    return pkt;
}

NodeIdAnnounce_t PacketBase::createNodeAnnouncePacket(const uint8_t *mac, uint8_t nodeId)
{
    NodeIdAnnounce_t pkt;
    memcpy(pkt.deviceMac, mac, 6);
    pkt.nodeId = nodeId;
    pkt.respond = 0;
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

void PacketBase::encapsulate(MessageTypeBase &msg)
{
    msg.setIsMission(true);
}

bool PacketBase::decapsulate(uint8_t *packet)
{
    bool isMission = packet[0] & 0x80;
    packet[0] = packet[0] & 0x7F;
    return isMission;
}
