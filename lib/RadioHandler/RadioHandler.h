// RadioHandler.h
#pragma once
#include <RadioLib.h>
#include "functions.h"

// Subclass to expose the protected method setDioIrqParams()
class SX1262Public : public SX1262
{
    using SX1262::SX1262; // inherit constructor
public:
    int16_t startReceive() override;
    void init(float frequency, uint8_t sf, uint8_t txPower, uint32_t bw);
    void reInitRadio(float frequency, uint8_t sf, uint8_t txPower, uint32_t bw);
    void sendRaw(const uint8_t *data, const size_t len);
    void setOnCallback(std::function<void()> cb);

private:
    std::function<void()> onSendCallback = nullptr;
};