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

uint32_t getSendTimeByPacketSizeInUS(int packetBytes)
{
    // Symbol duration in microseconds
    // Tsym = 2^SF / BW  → multiply by 1e6 for µs
    const double Tsym_us = (static_cast<double>(1u << LORA_SPREADINGFACTOR) * 1e6) / LORA_BANDWIDTH;

    constexpr double preambleSymbNb = 12.0;
    constexpr double headerSymbNb = 8.0;

    // Compute payload symbol number (LoRa formula)
    const double numerator = 8.0 * packetBytes - 4.0 * LORA_SPREADINGFACTOR + 28.0 + 16.0;
    const double denominator = 4.0 * (LORA_SPREADINGFACTOR - 2.0);
    double payloadSymbNb = std::ceil(std::max(0.0, numerator / denominator)) * (LORA_CR + 4.0);

    // Total symbols: preamble + header + payload
    const double totalSymbols = preambleSymbNb + 4.25 + headerSymbNb + payloadSymbNb;

    // Airtime in µs
    const double duration_us = totalSymbols * Tsym_us;

    // Clamp and round
    const uint64_t result = static_cast<uint64_t>(std::round(duration_us));
    return static_cast<uint32_t>(result > 0xFFFFFFFF ? 0xFFFFFFFF : result);
}