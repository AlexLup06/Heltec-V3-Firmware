// RadioHandler.h
#pragma once
#include <RadioLib.h>
#include "functions.h"

// --- Pin definitions (adjust for your board) ---
#define LORA_CS 8    // Chip select
#define LORA_RST 12  // Reset
#define LORA_DIO1 14 // Interrupt pin
#define LORA_BUSY 13 // Busy pin

// Subclass to expose the protected method setDioIrqParams()
class SX1262Public : public SX1262
{
    using SX1262::SX1262; // inherit constructor
public:
    int16_t startReceive() override;
    void init(float frequency, uint8_t sf, uint8_t txPower, uint32_t bw);
    void reInitRadio(float frequency, uint8_t sf, uint8_t txPower, uint32_t bw);
    void sendRaw(const uint8_t *data, const size_t len);
};