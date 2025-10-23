#include "PacketBase.h"

PacketBase::PacketBase()
{
}

PacketBase::~PacketBase()
{
}

void PacketBase::createRTS()
{
}
void PacketBase::createCTS()
{
}
void PacketBase::createLeaderFragment()
{
}
void PacketBase::createFragment()
{
}

void PacketBase::clearQueue()
{
}

void PacketBase::decapsulate(const bool isMission, const uint8_t *packet)
{
}

void PacketBase::encapsulate(const bool isMission, const uint8_t *packet)
{
}

void PacketBase::createNodeAnnouncePacket()
{
}

void PacketBase::enqueuePacket(const uint8_t *packet, const uint8_t packetSize, const bool isHeader, const bool isNeighbour, const bool isNodeAnnounce)
{
}

QueuedPacket PacketBase::dequeuePacket()
{
    return QueuedPacket();
}
