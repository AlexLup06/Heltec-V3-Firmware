#include "CustomPacketQueue.h"

CustomPacketQueue::CustomPacketQueue() {}

void CustomPacketQueue::printQueue()
{
    if (packetQueue.size() == 0)
    {
        DEBUG_PRINTLN("[Queue empty]");
        return;
    }

    DEBUG_PRINTF("Queue: ");
    for (int i = 0; i < packetQueue.size(); ++i)
    {
        const QueuedPacket *pkt = packetQueue.at(i);
        if (!pkt)
            continue;

        char buf[100];
        snprintf(buf, sizeof(buf), "%s(id=%u)", msgIdToString(pkt->data[0]), pkt->id);
        DEBUG_PRINTF_NO_TS(buf);
        if (i < packetQueue.size() - 1)
            DEBUG_PRINTF(", ");
    }
    DEBUG_PRINTLN_NO_TS();
}

CustomPacketQueue::~CustomPacketQueue()
{
    while (!packetQueue.empty())
    {
        auto pkt = packetQueue.front();
        if (pkt)
        {
            free(pkt->data);
        }
        packetQueue.pop_front();
    }
}

const QueuedPacket *CustomPacketQueue::getFirstPacket() const
{
    if (packetQueue.empty())
    {
        return nullptr;
    }
    return packetQueue.front();
}

bool CustomPacketQueue::enqueuePacket(QueuedPacket *pkt)
{
    if (!pkt)
        return false;

    // Example logic: NodeAnnounce always goes to front
    if (pkt->isNodeAnnounce)
    {
        enqueueNodeAnnounce(pkt);
        return true;
    }

    // Mission packets (non-neighbour) go to the back
    if (pkt->isMission)
    {
        packetQueue.push_back(pkt);
        return true;
    }

    if (!pkt->isMission && pkt->isHeader)
    {
        int insertPos = -1;
        int index = 0;

        for (auto it = packetQueue.begin(); it != packetQueue.end();)
        {
            auto existing = *it;
            if (!existing->isMission && existing->isHeader)
            {
                // Found the neighbour header — mark its position
                insertPos = index;

                // Delete this header and all following neighbour packets
                while (it != packetQueue.end() && !(*it)->isMission)
                {
                    auto old = *it;
                    free(old->data);
                    it = packetQueue.erase(it);
                }
                break; // stop once done
            }
            else
            {
                ++it;
                ++index;
            }
        }

        if (insertPos == -1)
        {
            // No neighbour header found — just append
            packetQueue.push_back(pkt);
        }
        else
        {
            enqueuePacketAtPosition(pkt, insertPos);
        }

        return true;
    }

    if (!pkt->isMission && !pkt->isHeader)
    {
        auto headerIt = packetQueue.end();
        auto lastNeighbourAfterHeader = packetQueue.end();

        // Find the first neighbour header and then the last neighbour after it
        for (auto it = packetQueue.begin(); it != packetQueue.end(); ++it)
        {
            if (!(*it)->isMission && (*it)->isHeader)
            {
                headerIt = it;
                lastNeighbourAfterHeader = it;
            }
            else if (headerIt != packetQueue.end() && !(*it)->isMission)
            {
                lastNeighbourAfterHeader = it;
            }
        }

        if (headerIt == packetQueue.end())
        {
            // No neighbour header exists → push front
            packetQueue.push_front(pkt);
        }
        else
        {
            // Insert right after the last neighbour fragment after the header
            ++lastNeighbourAfterHeader;
            packetQueue.insert(lastNeighbourAfterHeader, pkt);
        }

        return true;
    }

    return false;
}

void CustomPacketQueue::enqueuePacketAtPosition(QueuedPacket *pkt, int pos)
{
    if (pos < 0 || pos > packetQueue.size())
    {
        packetQueue.push_back(pkt);
        return;
    }

    auto it = packetQueue.begin();
    std::advance(it, pos);
    packetQueue.insert(it, pkt);
}

void CustomPacketQueue::enqueueNodeAnnounce(QueuedPacket *pkt)
{
    // Check if one already exists
    for (auto existing : packetQueue)
    {
        if (existing->isNodeAnnounce)
        {
            free(pkt->data);
            return;
        }
    }

    if (packetQueue.empty())
    {
        packetQueue.push_front(pkt);
        return;
    }

    // If first packet is a header -> insert at front
    if (packetQueue.front()->isHeader)
    {
        packetQueue.push_front(pkt);
        return;
    }

    // Otherwise insert before next header
    auto insertPos = packetQueue.end();
    for (auto it = packetQueue.begin(); it != packetQueue.end(); ++it)
    {
        if ((*it)->isHeader)
        {
            insertPos = it;
            break;
        }
    }

    if (insertPos != packetQueue.end())
    {
        packetQueue.insert(insertPos, pkt);
    }
    else
    {
        packetQueue.push_back(pkt);
    }
}

QueuedPacket *CustomPacketQueue::dequeuePacket()
{
    if (packetQueue.empty())
        return nullptr;
    auto pkt = packetQueue.front();
    packetQueue.pop_front();
    return pkt;
}

void CustomPacketQueue::removePacketAtPosition(int pos)
{
    if (pos < 0 || pos >= packetQueue.size())
        return;

    auto it = packetQueue.begin();
    std::advance(it, pos);

    auto pkt = *it;
    free(pkt->data);
    packetQueue.erase(it);
}

void CustomPacketQueue::removePacket(QueuedPacket *pkt)
{
    if (!pkt)
        return;
    packetQueue.erase(remove(packetQueue.begin(), packetQueue.end(), pkt), packetQueue.end());
    free(pkt->data);
}

bool CustomPacketQueue::isEmpty() const
{
    return packetQueue.empty();
}

int CustomPacketQueue::size() const
{
    return packetQueue.size();
}