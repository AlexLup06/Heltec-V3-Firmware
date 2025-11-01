#pragma once
#include <Arduino.h>
#include <LittleFS.h>
#include <cstdint>
#include <algorithm>
#include <cmath>
#include <cstdlib>
#include "config.h"
#include "definitions.h"

// Simple CRC-8 implementation (polynomial 0x07)
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
    constexpr double PREAMBLE_SYMB = 12.0;

    long bw = LORA_BANDWIDTH * 1000;

    // Derived parameters
    const bool lowDataRateOptimize = (LORA_SPREADINGFACTOR >= 11 && bw == 125000);
    const double DE = lowDataRateOptimize ? 1.0 : 0.0;
    const double H = IMPLICIT_HEADER ? 1.0 : 0.0;
    const double CRC = CRC_ON ? 1.0 : 0.0;

    // Symbol time in microseconds
    const double Tsym_us = (std::pow(2.0, LORA_SPREADINGFACTOR) * 1e6) / bw;

    // Payload symbol calculation (from SX1262 datasheet)
    const double numerator = 8.0 * packetBytes - 4.0 * LORA_SPREADINGFACTOR + 28.0 + 16.0 * CRC - 20.0 * H;
    const double denominator = 4.0 * (LORA_SPREADINGFACTOR - 2.0 * DE);
    const double payloadSymbNb = 8.0 + std::max(0.0, std::ceil(numerator / denominator) * (LORA_CR + 4.0));

    // Total symbols
    const double totalSymbols = PREAMBLE_SYMB + 4.25 + payloadSymbNb;

    // Airtime in µs
    const double duration_us = totalSymbols * Tsym_us;

    const uint64_t result = static_cast<uint64_t>(std::round(duration_us));
    return static_cast<uint32_t>(result > 0xFFFFFFFF ? 0xFFFFFFFF : result);
}

template <typename T>
inline T *tryCastMessage(uint8_t *buffer, size_t bufferSize = sizeof(T))
{
    if (!buffer)
        return nullptr;
    if (bufferSize < sizeof(T))
        return nullptr; // safety check

    const uint8_t *typePtr = buffer; // first byte in your structs
    const uint8_t expectedType = T{}.messageType;

    if (*typePtr != expectedType)
        return nullptr; // not the same message type

    return reinterpret_cast<T *>(buffer);
}

inline void dumpFilesOverSerial()
{
    File root = LittleFS.open("/");
    if (!root)
    {
        Serial.println("Failed to open LittleFS root");
        return;
    }

    File file = root.openNextFile();
    while (file)
    {
        String name = file.name();
        size_t size = file.size();

        // --- Send file header over Serial ---
        Serial.printf("BEGIN_FILE:%s:%u\n", name.c_str(), (unsigned)size);
        delay(50); // small delay to help PC sync

        // --- Send file contents ---
        uint8_t buffer[128];
        while (file.available())
        {
            size_t bytesRead = file.read(buffer, sizeof(buffer));
            Serial.write(buffer, bytesRead);
        }

        Serial.println(); // flush line break
        Serial.printf("END_FILE:%s\n", name.c_str());
        file = root.openNextFile();
        delay(100);
    }

    Serial.println("ALL_DONE");
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

#ifdef DEBUG_LORA_SERIAL
#define DEBUG_PRINTF(...) Serial.printf(__VA_ARGS__)
#else
#define DEBUG_PRINT(...)
#endif

#ifdef DEBUG_LORA_SERIAL
#define DEBUG_PRINTLN(...) Serial.println(__VA_ARGS__)
#else
#define DEBUG_PRINT(...)
#endif
