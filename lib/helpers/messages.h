#pragma once
#include <cstdint>
#include "definitions.h"

struct MessageTypeBase
{
protected:
    static constexpr uint8_t TYPE_MASK = 0x80; // 1000 0000
    static constexpr uint8_t MSG_MASK = 0x7F;  // 0111 1111

public:
    inline void setIsMission(bool isMission)
    {
        uint8_t &t = reinterpret_cast<uint8_t &>(*this);
        t = isMission ? (t | TYPE_MASK) : (t & MSG_MASK);
    }

    inline bool isMission() const
    {
        const uint8_t &t = reinterpret_cast<const uint8_t &>(*this);
        return (t & TYPE_MASK) != 0;
    }

    inline void clearIsMission()
    {
        uint8_t &t = reinterpret_cast<uint8_t &>(*this);
        t &= MSG_MASK;
    }
};

#pragma pack(push, 1)
struct OperationConfig_t
{
    uint8_t messageType = MESSAGE_TYPE_OPERATION_MODE_CONFIG;
    uint32_t startTime;
};
#pragma pack(pop)

#pragma pack(push, 1)
struct TimeSync_t
{
    uint8_t messageType = MESSAGE_TYPE_TIME_SYNC;
    uint32_t currentTime;
};
#pragma pack(pop)

#pragma pack(push, 1)
struct Node_Indicator_t
{
    uint8_t messageType = MESSAGE_TYPE_NODE_INDICATOR;
    uint16_t source;
};
#pragma pack(pop)

#pragma pack(push, 1)
struct NodeIdAnnounce_t : public MessageTypeBase
{
    uint8_t messageType = MESSAGE_TYPE_BROADCAST_NODE_ANNOUNCE;
    uint8_t deviceMac[6];
    uint8_t nodeId;
    uint8_t respond;
};
#pragma pack(pop)

#pragma pack(push, 1)
struct BroadcastRTSPacket_t : public MessageTypeBase
{
    uint8_t messageType = MESSAGE_TYPE_BROADCAST_RTS;
    uint16_t id = 0;
    uint8_t source = 0;
    uint16_t size = 0;
    uint8_t checksum = 0;
};
#pragma pack(pop)

#pragma pack(push, 1)
struct BroadcastContinuousRTSPacket_t : public MessageTypeBase
{
    uint8_t messageType = MESSAGE_TYPE_BROADCAST_CONTINUOUS_RTS;
};
#pragma pack(pop)

#pragma pack(push, 1)
struct BroadcastCTS_t : public MessageTypeBase
{
    uint8_t messageType = MESSAGE_TYPE_BROADCAST_CTS;
    uint16_t sizeOfFragment;
};
#pragma pack(pop)

#pragma pack(push, 1)
struct BroadcastLeaderFragmentPacket_t : public MessageTypeBase
{
    uint8_t messageType = MESSAGE_TYPE_BROADCAST_LEADER_FRAGMENT;
    uint16_t id = 0;
    uint8_t source = 0;
    uint16_t size = 0;
    uint8_t checksum = 0;
    uint8_t payload[LORA_MAX_PACKET_SIZE - BROADCAST_LEADER_FRAGMENT_METADATA_SIZE] = {0};
};
#pragma pack(pop)

#pragma pack(push, 1)
struct BroadcastFragmentPacket_t : public MessageTypeBase
{
    uint8_t messageType = MESSAGE_TYPE_BROADCAST_FRAGMENT;
    uint16_t id = 0;
    uint8_t fragment = 0;
    uint8_t source = 0;
    uint8_t payload[LORA_MAX_PACKET_SIZE - BROADCAST_FRAGMENT_METADATA_SIZE] = {0};
};
#pragma pack(pop)

// This struct contains information on the serially transmitted payload packet.
#pragma pack(1)
typedef struct
{
    bool isMission;
    uint16_t size;
    uint8_t *payload;
} MessageToSend_t;
#pragma pack()
