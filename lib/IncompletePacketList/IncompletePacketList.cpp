#include "IncompletePacketList.h"

FragmentedPacket_t *IncompletePacketList::getPacketBySource(const uint16_t source)
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
                        [source](FragmentedPacket_t *packet)
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

void IncompletePacketList::createIncompletePacket(
    const uint16_t id,
    const uint16_t messageSize,
    const uint8_t source,
    const uint8_t messageType,
    const uint8_t checksum)
{
    auto *pkt = getPacketBySource(source);
    if (pkt)
        assert(pkt->id < id);
    removePacketBySource(source);

    bool isLeaderFragment = messageType == MESSAGE_TYPE_BROADCAST_LEADER_FRAGMENT;
    int firstFragmentPayload = isLeaderFragment ? 247 : 251;
    int restMessageSize = messageSize - firstFragmentPayload;
    bool isOnlyOnePacket = restMessageSize <= 0;

    int numOfFragments = 1 + restMessageSize % 251;

    auto *packet = (FragmentedPacket_t *)malloc(sizeof(FragmentedPacket_t));
    packet->id = id;
    packet->checksum = checksum;
    packet->packetSize = messageSize;
    packet->source = source;
    packet->withLeaderFrag = messageType == MESSAGE_TYPE_BROADCAST_LEADER_FRAGMENT;
    packet->numOfFragments = numOfFragments;
    packet->corrupted = false;
    packet->received = 0;
    packet->payload = nullptr;

    if (xPortGetFreeHeapSize() - packet->packetSize > packet->packetSize + 10000)
    {
        packet->payload = (uint8_t *)malloc(packet->packetSize);
        packets_.push_back(packet);
    }
    else
    {
        free(packet);
    }
}

// TODO
Result IncompletePacketList::addToIncompletePacket(
    const uint16_t id,
    const uint16_t source,
    const uint8_t fragment,
    const uint16_t payloadSize,
    const uint8_t *payload)
{
    Result result;

    FragmentedPacket_t *incompletePacket = nullptr;
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

    // already received this packet
    if (incompletePacket->receivedFragments[fragment])
    {
        return result;
    }
    incompletePacket->receivedFragments[fragment] = true;

    if (isCorrupted(incompletePacket, fragment, payloadSize))
    {
        incompletePacket->corrupted = true;
        return result;
    }

    int offset = calcOffset(incompletePacket, fragment);
    memcpy(incompletePacket->payload + offset, payload, payloadSize);
    incompletePacket->received += payloadSize;

    assert(incompletePacket->received <= incompletePacket->packetSize);
    if (incompletePacket->received == incompletePacket->packetSize)
    {
        uint8_t calculatedChecksum = crc8(incompletePacket->payload, incompletePacket->packetSize);
        if (calculatedChecksum != incompletePacket->checksum)
            incompletePacket->corrupted = true;

        result.isComplete = true;
        result.sendUp = !incompletePacket->corrupted;
        result.completePacket = incompletePacket;
    }

    return result;
}

int IncompletePacketList::calcOffset(const FragmentedPacket_t *incompletePacket, const uint8_t fragment)
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

bool IncompletePacketList::isCorrupted(const FragmentedPacket_t *incompletePacket, const uint8_t fragment, const uint16_t payloadSize)
{
    // fragment payload has a max size of 251 Bytes
    if (payloadSize > 251)
        return true;

    // there is only one fragment
    if (incompletePacket->numOfFragments == 1)
    {
        if (incompletePacket->packetSize != payloadSize)
            return true;
        return false;
    }

    // There is two or more fragments

    // check if first fragment is corrupted
    if (fragment == 0)
    {
        int firstFragmentSize = incompletePacket->withLeaderFrag ? 247 : 251;
        if (firstFragmentSize != payloadSize)
            return true;
        return false;
    }

    // check middle fragment is corrupted if it exists
    if (incompletePacket->numOfFragments > 2 && fragment > 0 && fragment < incompletePacket->numOfFragments - 1)
    {
        if (payloadSize != 251)
            return true;
        return false;
    }

    // last fragment is corrupted
    if (incompletePacket->numOfFragments - 1 == fragment)
    {
        int firstFragmentSize = incompletePacket->withLeaderFrag ? 247 : 251;
        int packetSizeWithoutFirstAndLast = (incompletePacket->numOfFragments - 2) * 251;
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

bool IncompletePacketList::isNewIdLower(
    const uint8_t sourceId,
    const uint16_t newId) const
{
    for (const auto &entry : latestIdsFromSource_)
        if (entry.first == sourceId)
            return newId < entry.second;
    return false;
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