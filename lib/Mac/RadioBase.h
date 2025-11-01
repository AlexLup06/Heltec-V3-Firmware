#pragma once

#include <cstdint>
#include <RadioLib.h>
#include <RadioHandler.h>
#include "functions.h"

class RadioBase
{

private:
    bool isReceivingVar = false;
    bool isTransmittingVar = false;
    SX1262Public *radio;

    volatile uint16_t irqFlag = 0b0000000000000000;

public:
    void assignRadio(SX1262Public *_radio);
    void sendPacket(const uint8_t *data, const size_t len);
    void receiveDio1Interrupt();

    void startReceive();
    bool isReceiving();
    bool isTransmitting();
    float getRSSI();
    float getSNR();
    void standby();
    size_t getPacketLength();
    int readData(uint8_t *data, size_t len);
    void setReceivingVar(bool s);

    void finishRadioBase();

protected:
    virtual void onReceiveIR() {};
    virtual void onPreambleDetectedIR() {};

    void handleDio1Interrupt();
};
