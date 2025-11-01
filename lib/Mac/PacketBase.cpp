#include "PacketBase.h"

PacketBase::~PacketBase() { clearQueue(); }

FragmentedPacket *PacketBase::getIncompletePacketBySource(uint8_t source, bool isMission)
{
    if (isMission)
    {
        return incompleteMissionPackets.getPacketBySource(source);
    }
    else
    {
        return incompleteNeighbourPackets.getPacketBySource(source);
    }
}

void PacketBase::removeIncompletePacket(uint8_t source, bool isMission)
{
    if (isMission)
    {
        incompleteMissionPackets.removePacketBySource(source);
    }
    else
    {
        incompleteNeighbourPackets.removePacketBySource(source);
    }
}

bool PacketBase::createIncompletePacket(
    const uint16_t id,
    const uint16_t size,
    const uint8_t source,
    const int16_t hopId,
    const uint8_t messageType,
    const uint8_t checksum,
    bool isMission)
{

    if (isMission && incompleteMissionPackets.isNewIdHigher(source, id))
    {
        return incompleteMissionPackets.createIncompletePacket(id, size, source, hopId, messageType, checksum);
    }

    if (!isMission && incompleteNeighbourPackets.isNewIdHigher(source, id))
    {
        return incompleteNeighbourPackets.createIncompletePacket(id, size, source, hopId, messageType, checksum);
    }
    return false;
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
        return incompleteMissionPackets.isNewIdSame(sourceId, id);
    }
    else
    {
        return incompleteNeighbourPackets.isNewIdSame(sourceId, id);
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

void PacketBase::createNodeAnnouncePacket(uint8_t nodeId)
{
    BroadcastNodeIdAnnounce_t *pkt = (BroadcastNodeIdAnnounce_t *)malloc(sizeof(BroadcastNodeIdAnnounce_t));
    pkt->messageType = MESSAGE_TYPE_BROADCAST_NODE_ANNOUNCE;
    pkt->nodeId = nodeId;
    pkt->respond = 0;
    enqueueStruct(pkt, sizeof(BroadcastNodeIdAnnounce_t), false, false, true, 0);
    free(pkt);
}

void PacketBase::createMessage(const uint8_t *payload, uint16_t payloadSize, uint8_t source, bool withRTS, bool isMission, bool withContinuousRTS)
{
    uint16_t msgId = isMission ? MISSION_ID_COUNT++ : NEIGHBOUR_ID_COUNT++;
    uint8_t checksum = crc8(payload, payloadSize);

    if (withRTS)
    {
        BroadcastRTSPacket_t *rts = createRTS(msgId, source, payloadSize, checksum, isMission);
        encapsulate(rts, isMission);
        enqueueStruct(rts, sizeof(BroadcastRTSPacket_t), true, isMission, false, checksum);
        free(rts);

        uint8_t fragmentId = 0;
        while (payloadSize > 0)
        {

            uint16_t packetPayloadSize = std::min<uint16_t>(payloadSize, LORA_MAX_FRAGMENT_PAYLOAD);
            uint16_t packetSize = packetPayloadSize + BROADCAST_FRAGMENT_METADATA_SIZE;

            if (withContinuousRTS && fragmentId > 0)
            {
                BroadcastContinuousRTSPacket_t *continuousRTS = createContinuousRTS(msgId, source, packetSize, isMission);
                encapsulate(continuousRTS, isMission);
                enqueueStruct(continuousRTS, sizeof(BroadcastRTSPacket_t), true, isMission, false, checksum);
                free(continuousRTS);
            }

            BroadcastFragmentPacket_t *fragment = createFragment(msgId, fragmentId++, payload, packetPayloadSize, source, false, isMission);
            encapsulate(fragment, isMission);
            enqueueStruct(fragment, packetSize, false, isMission, false, checksum, withContinuousRTS);
            free(fragment);

            payload += packetPayloadSize;
            payloadSize -= packetPayloadSize;
        }
    }
    else
    {
        uint16_t leaderPayloadSize = std::min<uint16_t>(payloadSize, LORA_MAX_FRAGMENT_LEADER_PAYLOAD);
        uint16_t leaderPacketSize = leaderPayloadSize + BROADCAST_LEADER_FRAGMENT_METADATA_SIZE;
        BroadcastLeaderFragmentPacket_t *leaderFragment = createLeaderFragment(msgId, source, checksum, payload, payloadSize, leaderPayloadSize, isMission);
        encapsulate(leaderFragment, isMission);
        enqueueStruct(leaderFragment, leaderPacketSize, true, isMission, false, checksum);
        free(leaderFragment);

        payload += leaderPayloadSize;
        payloadSize -= leaderPayloadSize;

        uint8_t fragmentId = 1;
        while (payloadSize > 0)
        {
            uint16_t packetPayloadSize = std::min<uint16_t>(payloadSize, LORA_MAX_FRAGMENT_PAYLOAD);
            uint16_t packetSize = packetPayloadSize + BROADCAST_FRAGMENT_METADATA_SIZE;
            BroadcastFragmentPacket_t *fragment = createFragment(msgId, fragmentId++, payload, packetPayloadSize, source, true, isMission);
            encapsulate(fragment, isMission);
            enqueueStruct(fragment, packetSize, false, isMission, false, checksum);
            free(fragment);

            payload += packetPayloadSize;
            payloadSize -= packetPayloadSize;
        }
    }
}

BroadcastRTSPacket_t *PacketBase::createRTS(uint16_t packetId, uint8_t sourceId, uint16_t payloadSize, uint8_t checksum, bool isMission)
{
    BroadcastRTSPacket_t *pkt = (BroadcastRTSPacket_t *)malloc(sizeof(BroadcastRTSPacket_t));
    pkt->messageType = MESSAGE_TYPE_BROADCAST_RTS;
    pkt->source = sourceId;
    pkt->id = packetId;
    pkt->size = payloadSize;
    pkt->hopId = nodeId;
    pkt->checksum = checksum;

    encapsulate(pkt, isMission);
    return pkt;
}

BroadcastContinuousRTSPacket_t *PacketBase::createContinuousRTS(uint16_t packetId, uint16_t source, uint8_t fragmentSize, bool isMission)
{
    BroadcastContinuousRTSPacket_t *pkt = (BroadcastContinuousRTSPacket_t *)malloc(sizeof(BroadcastContinuousRTSPacket_t));
    pkt->messageType = MESSAGE_TYPE_BROADCAST_CONTINUOUS_RTS;
    pkt->id = packetId;
    pkt->source = source;
    pkt->hopId = nodeId;
    pkt->fragmentSize = fragmentSize;

    encapsulate(pkt, isMission);
    return pkt;
}

BroadcastCTS_t *PacketBase::createCTS(uint8_t fragmentSize, uint8_t rtsSource)
{
    BroadcastCTS_t *pkt = (BroadcastCTS_t *)malloc(sizeof(BroadcastCTS_t));
    pkt->messageType = MESSAGE_TYPE_BROADCAST_CTS;
    pkt->fragmentSize = fragmentSize;
    pkt->rtsSource = rtsSource;
    return pkt;
}

BroadcastLeaderFragmentPacket_t *PacketBase::createLeaderFragment(uint16_t packetId, uint16_t source, uint8_t checksum, const uint8_t *payload, uint16_t size, uint16_t leaderPayloadSize, bool isMission)
{
    BroadcastLeaderFragmentPacket_t *pkt = (BroadcastLeaderFragmentPacket_t *)malloc(sizeof(BroadcastLeaderFragmentPacket_t));
    pkt->messageType = MESSAGE_TYPE_BROADCAST_LEADER_FRAGMENT;
    pkt->source = source;
    pkt->id = packetId;
    pkt->size = size;
    pkt->checksum = checksum;

    // Copy only the part that is actually used
    memcpy(pkt->payload, payload, leaderPayloadSize);

    encapsulate(pkt, isMission);
    return pkt;
}

BroadcastFragmentPacket_t *PacketBase::createFragment(uint16_t packetId, uint8_t fragmentId, const uint8_t *payload, size_t payloadSize, uint8_t source, bool hasLeaderFrag, bool isMission)
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

    encapsulate(pkt, isMission);
    return pkt;
}

QueuedPacket *PacketBase::createNewContinuousRTS(QueuedPacket *queuedPacket)
{
    assert(!queuedPacket->isHeader && !queuedPacket->isNodeAnnounce);

    BroadcastContinuousRTSPacket_t *continuousRTS = createContinuousRTS(
        queuedPacket->id,
        queuedPacket->source,
        queuedPacket->packetSize,
        queuedPacket->isMission);

    QueuedPacket *pkt = (QueuedPacket *)malloc(sizeof(QueuedPacket));
    pkt->data = (uint8_t *)continuousRTS;
    pkt->packetSize = sizeof(BroadcastContinuousRTSPacket_t);
    pkt->isHeader = true;
    pkt->isMission = queuedPacket->isMission;
    pkt->isNodeAnnounce = false;
    pkt->sendTrys = 0;
    pkt->hasContinuousRTS = false;

    return pkt;
}
QueuedPacket *PacketBase::createNewRTS(QueuedPacket *queuedPacket)
{
    assert(!queuedPacket->isHeader && !queuedPacket->isNodeAnnounce);

    BroadcastRTSPacket_t *rts = createRTS(
        queuedPacket->id,
        queuedPacket->source,
        queuedPacket->packetSize,
        queuedPacket->fullMsgChecksum,
        queuedPacket->isMission);

    QueuedPacket *pkt = (QueuedPacket *)malloc(sizeof(QueuedPacket));
    pkt->data = (uint8_t *)rts;
    pkt->packetSize = sizeof(BroadcastRTSPacket_t);
    pkt->isHeader = true;
    pkt->isMission = queuedPacket->isMission;
    pkt->isNodeAnnounce = false;
    pkt->sendTrys = 0;
    pkt->hasContinuousRTS = false;
    pkt->fullMsgChecksum = queuedPacket->fullMsgChecksum;

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
    DEBUG_PRINTLN("Queue cleared");
}

QueuedPacket *PacketBase::dequeuePacket()
{
    return customPacketQueue.dequeuePacket();
}

// ---------------------- Incomplete Packetlist managment ----------------------

void PacketBase::clearIncompletePacketLists()
{
    incompleteMissionPackets.clear();
    DEBUG_PRINTLN("Cleared Incomplete Mission List");
    incompleteNeighbourPackets.clear();
    DEBUG_PRINTLN("Cleared Incomplete Neighbour List");
}

// ---------------------- Encapsulation / Decapsulation ----------------------

void PacketBase::encapsulate(MessageTypeBase *msg, bool isMission)
{
    if (isMission)
    {
        msg->setIsMission();
    }
}

bool PacketBase::decapsulate(uint8_t *packet)
{
    bool isMission = packet[0] & 0x80;
    packet[0] = packet[0] & 0x7F;
    return isMission;
}
