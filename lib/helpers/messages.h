#pragma once
#include <cstdint>
#include "definitions.h"

struct MessageTypeBase
{
protected:
    static constexpr uint8_t TYPE_MASK = 0x80; // 1000 0000
    static constexpr uint8_t MSG_MASK = 0x7F;  // 0111 1111

public:
    inline void setIsMission()
    {
        uint8_t &t = reinterpret_cast<uint8_t &>(*this);
        t = t | TYPE_MASK;
    }

    inline bool isMission() const
    {
        const uint8_t &t = reinterpret_cast<const uint8_t &>(*this);
        return (t & TYPE_MASK);
    }

    inline void clearIsMission()
    {
        uint8_t &t = reinterpret_cast<uint8_t &>(*this);
        t &= MSG_MASK;
    }
};

#pragma pack(push, 1)
struct BroadcastConfig_t
{
    uint8_t messageType = MESSAGE_TYPE_BROADCAST_CONFIG;
    uint8_t source;
    uint32_t startTime;
    uint32_t currentTime;
};
#pragma pack(pop)

#pragma pack(push, 1)
struct BroadcastNodeIdAnnounce_t : public MessageTypeBase
{
    uint8_t messageType = MESSAGE_TYPE_BROADCAST_NODE_ANNOUNCE;
    uint8_t nodeId;
    uint8_t respond;
};
#pragma pack(pop)

#pragma pack(push, 1)
struct BroadcastRTSPacket_t : public MessageTypeBase
{
    uint8_t messageType = MESSAGE_TYPE_BROADCAST_RTS;
    uint8_t source;
    uint8_t hopId;
    uint16_t id;
    uint16_t size;
    uint8_t checksum;
};
#pragma pack(pop)

#pragma pack(push, 1)
struct BroadcastContinuousRTSPacket_t : public MessageTypeBase
{
    uint8_t messageType = MESSAGE_TYPE_BROADCAST_CONTINUOUS_RTS;
    uint8_t source;
    uint8_t hopId;
    uint16_t id;
    uint8_t fragmentSize;
};
#pragma pack(pop)

#pragma pack(push, 1)
struct BroadcastCTS_t : public MessageTypeBase
{
    uint8_t messageType = MESSAGE_TYPE_BROADCAST_CTS;
    uint8_t rtsSource;
    uint8_t fragmentSize;
};
#pragma pack(pop)

#pragma pack(push, 1)
struct BroadcastLeaderFragmentPacket_t : public MessageTypeBase
{
    uint8_t messageType = MESSAGE_TYPE_BROADCAST_LEADER_FRAGMENT;
    uint8_t source;
    uint16_t id;
    uint16_t size;
    uint8_t checksum;
    uint8_t payload[LORA_MAX_PACKET_SIZE - BROADCAST_LEADER_FRAGMENT_METADATA_SIZE] = {0};
};
#pragma pack(pop)

#pragma pack(push, 1)
struct BroadcastFragmentPacket_t : public MessageTypeBase
{
    uint8_t messageType = MESSAGE_TYPE_BROADCAST_FRAGMENT;
    uint8_t source;
    uint16_t id;
    uint8_t fragment;
    uint8_t payload[LORA_MAX_PACKET_SIZE - BROADCAST_FRAGMENT_METADATA_SIZE] = {0};
};
#pragma pack(pop)

// This struct contains information on the serially transmitted payload packet.
#pragma pack(1)
struct MessageToSend_t
{
    bool isMission;
    uint16_t size;
    uint8_t *payload;
};
#pragma pack()
