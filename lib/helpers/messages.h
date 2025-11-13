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
struct BroadcastConfig
{
    uint8_t messageType = MESSAGE_TYPE_BROADCAST_CONFIG;
    uint8_t source;
    uint32_t startTime;
    uint8_t networkId;
    uint8_t numberOfNodes;
    uint32_t currentTime;
};

struct BroadcastNodeIdAnnounce : public MessageTypeBase
{
    uint8_t messageType = MESSAGE_TYPE_BROADCAST_NODE_ANNOUNCE;
    uint8_t nodeId;
    uint8_t respond;
};

struct BroadcastRTSPacket : public MessageTypeBase
{
    uint8_t messageType = MESSAGE_TYPE_BROADCAST_RTS;
    uint16_t id;
    uint8_t source;
    uint8_t hopId;
    uint16_t size;
    uint8_t checksum;
};

struct BroadcastContinuousRTSPacket : public MessageTypeBase
{
    uint8_t messageType = MESSAGE_TYPE_BROADCAST_CONTINUOUS_RTS;
    uint8_t source;
    uint8_t hopId;
    uint16_t id;
    uint8_t fragmentSize;
};

struct BroadcastCTS : public MessageTypeBase
{
    uint8_t messageType = MESSAGE_TYPE_BROADCAST_CTS;
    uint8_t rtsSource;
    uint8_t fragmentSize;
};

struct BroadcastLeaderFragmentPacket : public MessageTypeBase
{
    uint8_t messageType = MESSAGE_TYPE_BROADCAST_LEADER_FRAGMENT;
    uint16_t id;
    uint8_t source;
    uint16_t size;
    uint8_t checksum;
    uint8_t payload[LORA_MAX_PACKET_SIZE - BROADCAST_LEADER_FRAGMENT_METADATA_SIZE] = {0};
};

struct BroadcastFragmentPacket : public MessageTypeBase
{
    uint8_t messageType = MESSAGE_TYPE_BROADCAST_FRAGMENT;
    uint16_t id;
    uint8_t source;
    uint8_t fragment;
    uint8_t payload[LORA_MAX_PACKET_SIZE - BROADCAST_FRAGMENT_METADATA_SIZE] = {0};
};

struct MessageToSend
{
    bool isMission;
    uint16_t size;
    uint8_t *payload;
};
#pragma pack()
