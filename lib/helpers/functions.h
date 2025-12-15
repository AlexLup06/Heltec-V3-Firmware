#pragma once
#include <Arduino.h>
#include <cstdint>
#include <algorithm>
#include <cmath>
#include <cstdlib>
#include "config.h"
#include "definitions.h"

inline bool nodeIdArrayContains(uint8_t value)
{
    for (uint8_t id : allNodeIds)
    {
        if (id == value)
            return true;
    }
    return false;
}

inline void debugTimestamp(char *buf, size_t len)
{
    uint64_t us = esp_timer_get_time();
    uint32_t ms = us / 1000;
    uint32_t sec = ms / 1000;

    snprintf(buf, len, "%u.%03u", sec, ms % 1000);
}

inline uint64_t unixTimeMs()
{
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);

    return (uint64_t)ts.tv_sec * 1000ULL +
           (uint64_t)ts.tv_nsec / 1000000ULL;
}

#ifdef DEBUG_LORA_SERIAL

#define DEBUG_PRINTF(...)                 \
    do                                    \
    {                                     \
        char _ts[16];                     \
        debugTimestamp(_ts, sizeof(_ts)); \
        Serial.print("[");                \
        Serial.print(_ts);                \
        Serial.print("] ");               \
        Serial.printf(__VA_ARGS__);       \
    } while (0)

#define DEBUG_PRINTLN(msg)                \
    do                                    \
    {                                     \
        char _ts[16];                     \
        debugTimestamp(_ts, sizeof(_ts)); \
        Serial.print("[");                \
        Serial.print(_ts);                \
        Serial.print("] ");               \
        Serial.println(msg);              \
    } while (0)

#define DEBUG_PRINTF_NO_TS(msg) Serial.printf(msg);
#define DEBUG_PRINTLN_NO_TS(msg) Serial.println(msg);

#else
#define DEBUG_PRINTF(...)
#define DEBUG_PRINTLN(...)
#define DEBUG_PRINTF_NO_TS(...)
#define DEBUG_PRINTLN_NO_TS(...)
#endif

inline uint8_t crc8(const uint8_t *data, size_t len)
{
    uint8_t crc = 0x00;
    for (size_t i = 0; i < len; i++)
    {
        crc ^= data[i];
        for (uint8_t j = 0; j < 8; j++)
        {
            if (crc & 0x80)
                crc = (crc << 1) ^ 0x07;
            else
                crc <<= 1;
        }
    }
    return crc;
}

inline uint32_t getToAByPacketSizeInUS(int packetBytes)
{
    constexpr bool CRC_ON = true;
    constexpr bool IMPLICIT_HEADER = false;
    constexpr double PREAMBLE_SYMB = LORA_PREAMBLE_LENGTH;

    long bw = LORA_BANDWIDTH * 1000;

    const bool lowDataRateOptimize = (LORA_SPREADINGFACTOR >= 11 && bw == 125000);
    const double DE = lowDataRateOptimize ? 1.0 : 0.0;
    const double H = IMPLICIT_HEADER ? 1.0 : 0.0;
    const double CRC = CRC_ON ? 1.0 : 0.0;

    const double Tsym_us = (std::pow(2.0, LORA_SPREADINGFACTOR) * 1e6) / bw;

    const double numerator = 8.0 * packetBytes - 4.0 * LORA_SPREADINGFACTOR + 28.0 + 16.0 * CRC - 20.0 * H;
    const double denominator = 4.0 * (LORA_SPREADINGFACTOR - 2.0 * DE);
    const double payloadSymbNb = 8.0 + std::max(0.0, std::ceil(numerator / denominator) * (LORA_CR + 4.0));

    const double totalSymbols = PREAMBLE_SYMB + 4.25 + payloadSymbNb;

    const double duration_us = totalSymbols * Tsym_us;

    const uint64_t result = static_cast<uint64_t>(std::round(duration_us));
    return static_cast<uint32_t>(result > 0xFFFFFFFF ? 0xFFFFFFFF : result);
}

template <typename T>
inline T *tryCastMessage(const uint8_t *buffer, size_t bufferSize = sizeof(T))
{
    if (!buffer)
        return nullptr;
    if (bufferSize < sizeof(T))
        return nullptr; // safety check

    const uint8_t *typePtr = buffer; // first byte in your structs
    const uint8_t expectedType = T{}.messageType;

    if (*typePtr != expectedType)
        return nullptr; // not the same message type

    return reinterpret_cast<const T *>(buffer);
}

inline const char *networkIdToString(uint8_t networkId)
{
    switch (networkId)
    {
    case TOPOLOGY_FULLY_MESHED:
        return "TOPOLOGY_FULLY_MESHED";
    case TOPOLOGY_LINE:
        return "TOPOLOGY_LINE";
    case TOPOLOGY_COMPLEX:
        return "TOPOLOGY_COMPLEX";
    default:
        return "UNKNOWN_TOPOLOGY";
    }
}

inline const char *msgIdToString(uint8_t msgId)
{
    msgId = msgId & 0x7F;
    switch (msgId)
    {
    case MESSAGE_TYPE_BROADCAST_CONFIG:
        return "BROADCAST_CONFIG";
    case MESSAGE_TYPE_BROADCAST_NODE_ANNOUNCE:
        return "BROADCAST_NODE_ANNOUNCE";
    case MESSAGE_TYPE_BROADCAST_RTS:
        return "BROADCAST_RTS";
    case MESSAGE_TYPE_BROADCAST_CONTINUOUS_RTS:
        return "BROADCAST_CONTINUOUS_RTS";
    case MESSAGE_TYPE_BROADCAST_CTS:
        return "BROADCAST_CTS";
    case MESSAGE_TYPE_BROADCAST_LEADER_FRAGMENT:
        return "BROADCAST_LEADER_FRAGMENT";
    case MESSAGE_TYPE_BROADCAST_FRAGMENT:
        return "BROADCAST_FRAGMENT";
    default:
        return "UNKNOWN_MESSAGE_TYPE";
    }
}
