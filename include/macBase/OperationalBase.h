#pragma once
#include <Arduino.h>

class OperationalBase
{
public:
    virtual ~OperationalBase() {}
    virtual void init() = 0;
    virtual void handle() = 0;
    virtual void finish() = 0;

    void receiveDio1Interrupt()
    {
        uint16_t irq = radio.getIrqFlags();
        radio.clearIrqFlags(irq);

        if (irq & RADIOLIB_SX126X_IRQ_PREAMBLE_DETECTED)
            onPreambleDetectedIR();

        if (irq & RADIOLIB_SX126X_IRQ_RX_DONE)
            onReceiveIR();

        if (irq & RADIOLIB_SX126X_IRQ_CRC_ERR)
            onCRCerrorIR();
    }

protected:
    virtual void onReceiveIR() = 0;
    virtual void onPreambleDetectedIR() = 0;
    virtual void onCRCerrorIR() = 0;
};