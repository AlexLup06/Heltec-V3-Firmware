// RadioHandler.h
#pragma once
#include <RadioLib.h>

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
    int16_t startReceive() override
    {
        int16_t s = SX1262::startReceive();
        if (s == RADIOLIB_ERR_NONE)
        {
            // RADIOLIB_IRQ_CRC_ERR already enabled by default
            this->setDioIrqParams(
                getIrqMapped(RADIOLIB_IRQ_RX_DEFAULT_FLAGS | RADIOLIB_IRQ_PREAMBLE_DETECTED),
                getIrqMapped(RADIOLIB_IRQ_RX_DEFAULT_MASK | RADIOLIB_IRQ_PREAMBLE_DETECTED));
        }
        return s;
    }
};

extern SX1262Public radio;

void initRadio(float frequency, uint8_t sf, uint8_t txPower, uint32_t bw);
void reInitRadio(float frequency, uint8_t sf, uint8_t txPower, uint32_t bw);

void startReceive();
void sendPacket(uint8_t *data, size_t len);
float getRSSI();
float getSNR();