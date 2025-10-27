#pragma once
#include <cstdint>
#include <algorithm>
#include <cmath>
#include <cstdlib> 
#include "config.h"

// Simple CRC-8 implementation (polynomial 0x07)
inline uint8_t crc8(const uint8_t *data, size_t len) {
    uint8_t crc = 0x00;
    for (size_t i = 0; i < len; i++) {
        crc ^= data[i];
        for (uint8_t j = 0; j < 8; j++) {
            if (crc & 0x80)
                crc = (crc << 1) ^ 0x07;
            else
                crc <<= 1;
        }
    }
    return crc;
}

inline uint32_t getSendTimeByPacketSizeInUS(int packetBytes)
{
    constexpr bool CRC_ON = true;
    constexpr bool IMPLICIT_HEADER = false;
    constexpr double PREAMBLE_SYMB = 12.0;

    // Derived parameters
    const bool lowDataRateOptimize = (LORA_SPREADINGFACTOR >= 11 && LORA_BANDWIDTH == 125000);
    const double DE = lowDataRateOptimize ? 1.0 : 0.0;
    const double H = IMPLICIT_HEADER ? 1.0 : 0.0;
    const double CRC = CRC_ON ? 1.0 : 0.0;

    // Symbol time in microseconds
    const double Tsym_us = (std::pow(2.0, LORA_SPREADINGFACTOR) * 1e6) / LORA_BANDWIDTH;

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

#ifdef DEBUG_LORA_SERIAL
  #define DEBUG_PRINT(...)  DEBUG_LORA_SERIAL(__VA_ARGS__)
#else
  #define DEBUG_PRINT(...)
#endif
