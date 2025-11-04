// RadioHandler.h
#pragma once
#include <RadioLib.h>
#include "functions.h"

// Subclass to expose the protected method setDioIrqParams()
class SX1262Public : public SX1262
{
    using SX1262::SX1262; // inherit constructor
public:
    SX1262Public(Module* module);

    uint16_t irqFlag = 0b0000000000000000;
    static void receiveDio1Interrupt();
    void handleDio1Interrupt();

    int16_t startReceive() override;
    void init(float frequency, uint8_t sf, uint8_t txPower, uint32_t bw);
    void reInitRadio(float frequency, uint8_t sf, uint8_t txPower, uint32_t bw);
    int sendRaw(const uint8_t *data, const size_t len);

private:
    static SX1262Public *instance;
};