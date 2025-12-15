#include "IncompletePacketList.h"

IncompletePacketList::IncompletePacketList(bool isMissionList)
{
    isMissionList_ = isMissionList;
    packets_.reserve(10); // We do not have more than 10 nodes in the network
}

void IncompletePacketList::setLogFragmentCallback(LogFunc func)
{
    logFragmentFunc_ = std::move(func);
}

void IncompletePacketList::clear()
{
    for (auto *pkt : packets_)
    {
        if (pkt)
        {
            if (pkt->payload)
            {
                free(pkt->payload);
                pkt->payload = nullptr;
            }

            free(pkt);
        }
    }

    packets_.clear();
    latestIdsFromSource_.clear();
}

FragmentedPacket *IncompletePacketList::getPacketBySource(const uint16_t source)
{
    for (auto *packet : packets_)
    {
        if (packet && packet->source == source)
        {
            return packet;
        }
    }
    return nullptr;
}

void IncompletePacketList::removePacketBySource(const uint8_t source)
{
    auto it = remove_if(packets_.begin(), packets_.end(),
                        [source](FragmentedPacket *packet)
                        {
                            if (packet && packet->source == source)
                            {
                                if (packet->payload)
                                    free(packet->payload);
                                free(packet);
                                return true; // remove from vector
                            }
                            return false;
                        });
    packets_.erase(it, packets_.end());
}

bool IncompletePacketList::createIncompletePacket(
    const uint16_t id,
    const uint16_t messageSize,
    const uint8_t source,
    const int16_t hopId,
    const uint8_t messageType,
    const uint8_t checksum)
{
    auto *pkt = getPacketBySource(source);
    if (pkt != nullptr)
        assert(pkt->id <= id);
    removePacketBySource(source);

    DEBUG_PRINTF("Create packet: id=%u, size=%u, source=%u, hopId=%d, type=%s, checksum=%u, isMission: %s\n",
                 id, messageSize, source, hopId, msgIdToString(messageType), checksum, isMissionList_ ? "True" : "False");

    bool isLeaderFragment = messageType == MESSAGE_TYPE_BROADCAST_LEADER_FRAGMENT;
    int firstFragmentPayload = isLeaderFragment ? LORA_MAX_FRAGMENT_LEADER_PAYLOAD : LORA_MAX_FRAGMENT_PAYLOAD;
    int restMessageSize = messageSize - firstFragmentPayload;
    bool isOnlyOnePacket = restMessageSize <= 0;

    int numOfFragments = 1;
    if (!isOnlyOnePacket)
    {
        numOfFragments = 1 + restMessageSize / LORA_MAX_FRAGMENT_PAYLOAD;
    }

    auto *packet = (FragmentedPacket *)malloc(sizeof(FragmentedPacket));
    packet->id = id;
    packet->checksum = checksum;
    packet->packetSize = messageSize;
    memset(packet->receivedFragments, 0, sizeof(packet->receivedFragments));
    packet->source = source;
    packet->withLeaderFrag = messageType == MESSAGE_TYPE_BROADCAST_LEADER_FRAGMENT;
    packet->numOfFragments = numOfFragments;
    packet->corrupted = false;
    packet->received = 0;
    packet->payload = nullptr;
    packet->hopId = hopId;
    packet->isMission = isMissionList_;

    packet->payload = (uint8_t *)malloc(packet->packetSize);
    packets_.push_back(packet);
    updatePacketId(source, id);
    return true;
}

Result IncompletePacketList::addToIncompletePacket(
    const uint16_t id,
    const uint16_t source,
    const uint8_t fragment,
    const uint16_t payloadSize,
    const uint8_t *payload)
{
    Result result;

    DEBUG_PRINTF("[IncompletePacketList] Add to packet: id=%u, payloadSize=%u, source=%u\n", id, payloadSize, source);

    FragmentedPacket *incompletePacket = nullptr;
    for (auto *p : packets_)
    {
        if (p && p->id == id && p->source == source)
        {
            incompletePacket = p;
            break;
        }
    }

    if (!incompletePacket)
    {
        return result;
    }

    logFragmentFunc_(id, incompletePacket->source, incompletePacket->hopId);

    // already received this packet
    if (incompletePacket->receivedFragments[fragment])
    {
        DEBUG_PRINTLN("[IncompletePacketList] Already received this fragment");
        result.bytesLeft = incompletePacket->packetSize - incompletePacket->received;
        return result;
    }
    incompletePacket->receivedFragments[fragment] = true;

    if (isCorrupted(incompletePacket, fragment, payloadSize))
    {
        DEBUG_PRINTLN("[IncompletePacketList] Packet is corrupted");
        incompletePacket->corrupted = true;
        return result;
    }

    int offset = calcOffset(incompletePacket, fragment);
    memcpy(incompletePacket->payload + offset, payload, payloadSize);

    incompletePacket->received += payloadSize;
    result.bytesLeft = incompletePacket->packetSize - incompletePacket->received;

    assert(incompletePacket->received <= incompletePacket->packetSize);
    if (incompletePacket->received == incompletePacket->packetSize)
    {
        uint8_t calculatedChecksum = crc8(incompletePacket->payload, incompletePacket->packetSize);

        if (calculatedChecksum != incompletePacket->checksum)
            incompletePacket->corrupted = true;

        DEBUG_PRINTF("[IncompletePacketList] Packet is corrupted: %s\n", incompletePacket->corrupted ? "True" : "False");

        result.isComplete = true;
        result.sendUp = !incompletePacket->corrupted;
        result.completePacket = incompletePacket;
        result.isMission = isMissionList_;
    }

    return result;
}

int IncompletePacketList::calcOffset(const FragmentedPacket *incompletePacket, const uint8_t fragment)
{
    if (fragment == 0)
    {
        return 0;
    }

    if (incompletePacket->withLeaderFrag)
    {
        return LORA_MAX_FRAGMENT_LEADER_PAYLOAD + LORA_MAX_FRAGMENT_PAYLOAD * (fragment - 1);
    }
    else
    {
        return LORA_MAX_FRAGMENT_PAYLOAD * (fragment);
    }
}

bool IncompletePacketList::isCorrupted(const FragmentedPacket *incompletePacket, const uint8_t fragment, const uint16_t payloadSize)
{
    if (payloadSize > LORA_MAX_FRAGMENT_PAYLOAD)
        return true;

    if (incompletePacket->numOfFragments == 1)
    {
        if (incompletePacket->packetSize != payloadSize)
            return true;
        return false;
    }

    if (fragment == 0)
    {
        int firstFragmentSize = incompletePacket->withLeaderFrag ? LORA_MAX_FRAGMENT_LEADER_PAYLOAD : LORA_MAX_FRAGMENT_PAYLOAD;
        if (firstFragmentSize != payloadSize)
            return true;
        return false;
    }

    if (incompletePacket->numOfFragments > 2 && fragment > 0 && fragment < incompletePacket->numOfFragments - 1)
    {
        if (payloadSize != LORA_MAX_FRAGMENT_PAYLOAD)
            return true;
        return false;
    }

    if (incompletePacket->numOfFragments - 1 == fragment)
    {
        int firstFragmentSize = incompletePacket->withLeaderFrag ? LORA_MAX_FRAGMENT_LEADER_PAYLOAD : LORA_MAX_FRAGMENT_PAYLOAD;
        int packetSizeWithoutFirstAndLast = (incompletePacket->numOfFragments - 2) * LORA_MAX_FRAGMENT_PAYLOAD;
        int packetSize = firstFragmentSize + packetSizeWithoutFirstAndLast + payloadSize;

        if (incompletePacket->packetSize != packetSize)
            return true;
        return false;
    }

    return false;
}

void IncompletePacketList::updatePacketId(
    const uint8_t sourceId,
    const uint16_t newId)
{
    for (auto &entry : latestIdsFromSource_)
    {
        if (entry.first == sourceId)
        {
            entry.second = newId;
            return;
        }
    }

    latestIdsFromSource_.emplace_back(sourceId, newId);
}

bool IncompletePacketList::isNewIdSame(
    const uint8_t sourceId,
    const uint16_t newId) const
{
    for (const auto &entry : latestIdsFromSource_)
        if (entry.first == sourceId)
            return newId == entry.second;
    return false;
}

bool IncompletePacketList::isNewIdHigher(
    const uint8_t sourceId,
    const uint16_t newId) const
{
    for (const auto &entry : latestIdsFromSource_)
        if (entry.first == sourceId)
            return newId > entry.second;
    return true;
}