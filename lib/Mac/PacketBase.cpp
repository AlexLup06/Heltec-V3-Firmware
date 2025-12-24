#include "PacketBase.h"

PacketBase::PacketBase()
{
    incompleteMissionPackets.setLogFragmentCallback(
        [this](uint16_t id, uint8_t src, int16_t hop)
        {
            this->logReceivedFragmentsIdStatistics(id, true, src, hop);
        });

    incompleteNeighbourPackets.setLogFragmentCallback(
        [this](uint16_t id, uint8_t src, int16_t hop)
        {
            this->logReceivedFragmentsIdStatistics(id, false, src, hop);
        });
}

PacketBase::~PacketBase() { clearQueue(); }

void PacketBase::logReceivedFragmentsIdStatistics(uint16_t id, bool isMission, uint8_t sourceId, uint8_t hopId)
{
    if (!isMission)
    {
        ReceivedNeighbourIdFragment_data receivedNeighbourIdFragment = ReceivedNeighbourIdFragment_data();
        receivedNeighbourIdFragment.missionId = id;
        receivedNeighbourIdFragment.sourceId = sourceId;
        loggerManager->log(Metric::ReceivedNeighbourIdFragment_V, receivedNeighbourIdFragment);
    }
    if (isMission)
    {
        ReceivedMissionIdFragment_data receivedMissionIdFragment = ReceivedMissionIdFragment_data();
        receivedMissionIdFragment.missionId = id;
        receivedMissionIdFragment.hopId = hopId;
        receivedMissionIdFragment.sourceId = sourceId;
        loggerManager->log(Metric::ReceivedMissionIdFragment_V, receivedMissionIdFragment);
    }
}

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
    if (source == nodeId)
    {
        DEBUG_PRINTF("We DO NO create incomplete packet because source == nodeId: id=%u, source=%u, hopId=%d, type=%s, checksum=%u, isMission: %s\n",
                     id, source, hopId, msgIdToString(messageType), checksum, isMission ? "True" : "False");
        return false;
    }

    if (isMission && incompleteMissionPackets.isNewIdHigher(source, id))
    {
        return incompleteMissionPackets.createIncompletePacket(id, size, source, hopId, messageType, checksum);
    }

    if (!isMission && incompleteNeighbourPackets.isNewIdHigher(source, id))
    {
        return incompleteNeighbourPackets.createIncompletePacket(id, size, source, hopId, messageType, checksum);
    }

    DEBUG_PRINTF("We DO NO create incomplete packet: id=%u, source=%u, hopId=%d, type=%s, checksum=%u, isMission: %s\n",
                 id, source, hopId, msgIdToString(messageType), checksum, isMission ? "True" : "False");

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
    return firstPacket->isHeader;
}

// ---------------------- Packet Creation ----------------------

void PacketBase::createMessage(const uint8_t *payload, uint16_t payloadSize, uint8_t source, bool withRTS, bool isMission, bool withContinuousRTS, int id)
{
    uint16_t msgId = id;
    if (id == -1)
        msgId = isMission ? MISSION_ID_COUNT++ : NEIGHBOUR_ID_COUNT++;

    uint8_t checksum = crc8(payload, payloadSize);

    if (withRTS)
    {
        BroadcastRTS *rts = createRTS(msgId, source, payloadSize, checksum, isMission);
        encapsulate(rts, isMission);
        enqueueStruct(rts, sizeof(BroadcastRTS), source, msgId, checksum, true, isMission, false);
        free(rts);

        uint8_t fragmentId = 0;
        while (payloadSize > 0)
        {

            uint16_t packetPayloadSize = std::min<uint16_t>(payloadSize, LORA_MAX_FRAGMENT_PAYLOAD);
            uint16_t packetSize = packetPayloadSize + BROADCAST_FRAGMENT_METADATA_SIZE;

            bool thisFragHasContinuousRTS = false;
            if (withContinuousRTS && fragmentId > 0)
            {
                BroadcastContinuousRTS *continuousRTS = createContinuousRTS(msgId, source, packetSize, isMission);
                encapsulate(continuousRTS, isMission);
                enqueueStruct(continuousRTS, sizeof(BroadcastRTS), source, msgId, checksum, true, isMission, false);
                free(continuousRTS);
                thisFragHasContinuousRTS = true;
            }

            BroadcastFragment *fragment = createFragment(msgId, fragmentId++, payload, packetPayloadSize, source, false, isMission);
            encapsulate(fragment, isMission);
            enqueueStruct(fragment, packetSize, source, msgId, checksum, false, isMission, false, thisFragHasContinuousRTS);
            free(fragment);

            payload += packetPayloadSize;
            payloadSize -= packetPayloadSize;
        }
    }
    else
    {
        uint16_t leaderPayloadSize = std::min<uint16_t>(payloadSize, LORA_MAX_FRAGMENT_LEADER_PAYLOAD);
        uint16_t leaderPacketSize = leaderPayloadSize + BROADCAST_LEADER_FRAGMENT_METADATA_SIZE;
        BroadcastLeaderFragment *leaderFragment = createLeaderFragment(msgId, source, checksum, payload, payloadSize, leaderPayloadSize, isMission);
        encapsulate(leaderFragment, isMission);
        enqueueStruct(leaderFragment, leaderPacketSize, source, msgId, checksum, true, isMission, false);
        free(leaderFragment);

        payload += leaderPayloadSize;
        payloadSize -= leaderPayloadSize;

        uint8_t fragmentId = 1;
        while (payloadSize > 0)
        {
            uint16_t packetPayloadSize = std::min<uint16_t>(payloadSize, LORA_MAX_FRAGMENT_PAYLOAD);
            uint16_t packetSize = packetPayloadSize + BROADCAST_FRAGMENT_METADATA_SIZE;
            BroadcastFragment *fragment = createFragment(msgId, fragmentId++, payload, packetPayloadSize, source, true, isMission);
            encapsulate(fragment, isMission);
            enqueueStruct(fragment, packetSize, source, msgId, checksum, false, isMission, false);
            free(fragment);

            payload += packetPayloadSize;
            payloadSize -= packetPayloadSize;
        }
    }
}

BroadcastRTS *PacketBase::createRTS(uint16_t packetId, uint8_t sourceId, uint16_t payloadSize, uint8_t checksum, bool isMission)
{
    BroadcastRTS *pkt = (BroadcastRTS *)malloc(sizeof(BroadcastRTS));
    pkt->messageType = MESSAGE_TYPE_BROADCAST_RTS;
    pkt->source = sourceId;
    pkt->id = packetId;
    pkt->size = payloadSize;
    pkt->hopId = nodeId;
    pkt->checksum = checksum;

    encapsulate(pkt, isMission);
    return pkt;
}

BroadcastContinuousRTS *PacketBase::createContinuousRTS(uint16_t packetId, uint16_t source, uint8_t fragmentSize, bool isMission)
{
    BroadcastContinuousRTS *pkt = (BroadcastContinuousRTS *)malloc(sizeof(BroadcastContinuousRTS));
    pkt->messageType = MESSAGE_TYPE_BROADCAST_CONTINUOUS_RTS;
    pkt->id = packetId;
    pkt->source = source;
    pkt->hopId = nodeId;
    pkt->fragmentSize = fragmentSize;

    encapsulate(pkt, isMission);
    return pkt;
}

BroadcastCTS *PacketBase::createCTS(uint8_t fragmentSize, uint8_t rtsSource, uint8_t chosenSlot)
{
    BroadcastCTS *pkt = (BroadcastCTS *)malloc(sizeof(BroadcastCTS));
    pkt->messageType = MESSAGE_TYPE_BROADCAST_CTS;
    pkt->fragmentSize = fragmentSize;
    pkt->rtsSource = rtsSource;
    pkt->chosenSlot = chosenSlot;
    return pkt;
}

BroadcastLeaderFragment *PacketBase::createLeaderFragment(uint16_t packetId, uint16_t source, uint8_t checksum, const uint8_t *payload, uint16_t size, uint16_t leaderPayloadSize, bool isMission)
{
    BroadcastLeaderFragment *pkt = (BroadcastLeaderFragment *)malloc(sizeof(BroadcastLeaderFragment));
    pkt->messageType = MESSAGE_TYPE_BROADCAST_LEADER_FRAGMENT;
    pkt->source = source;
    pkt->hopId = nodeId;
    pkt->id = packetId;
    pkt->size = size;
    pkt->checksum = checksum;

    // Copy only the part that is actually used
    memcpy(pkt->payload, payload, leaderPayloadSize);

    encapsulate(pkt, isMission);
    return pkt;
}

BroadcastFragment *PacketBase::createFragment(uint16_t packetId, uint8_t fragmentId, const uint8_t *payload, size_t payloadSize, uint8_t source, bool hasLeaderFrag, bool isMission)
{
    BroadcastFragment *pkt = (BroadcastFragment *)malloc(sizeof(BroadcastFragment));
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

    BroadcastContinuousRTS *continuousRTS = createContinuousRTS(
        queuedPacket->id,
        queuedPacket->source,
        queuedPacket->packetSize,
        queuedPacket->isMission);

    QueuedPacket *pkt = (QueuedPacket *)malloc(sizeof(QueuedPacket));
    pkt->data = (uint8_t *)continuousRTS;
    pkt->packetSize = sizeof(BroadcastContinuousRTS);
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

    BroadcastRTS *rts = createRTS(
        queuedPacket->id,
        queuedPacket->source,
        queuedPacket->packetSize,
        queuedPacket->fullMsgChecksum,
        queuedPacket->isMission);

    QueuedPacket *pkt = (QueuedPacket *)malloc(sizeof(QueuedPacket));
    pkt->data = (uint8_t *)rts;
    pkt->packetSize = sizeof(BroadcastRTS);
    pkt->source = nodeId;
    pkt->id = rts->id;
    pkt->sendTrys = 0;
    pkt->fullMsgChecksum = queuedPacket->fullMsgChecksum;
    pkt->isHeader = true;
    pkt->isMission = queuedPacket->isMission;
    pkt->isNodeAnnounce = false;
    pkt->hasContinuousRTS = false;

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
            free(pkt);
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
    MISSION_ID_COUNT = 0;

    incompleteNeighbourPackets.clear();
    NEIGHBOUR_ID_COUNT = 0;
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
