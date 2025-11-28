#pragma once

#include <cstdint>
#include <RadioLib.h>
#include <RadioHandler.h>
#include "functions.h"
#include "NodeContext.h"

class RadioBase : public NodeContext
{

private:
    bool isReceivingVar = false;
    bool isTransmittingVar = false;
    unsigned long lastPreambleTime = 0;

    SX1262Public *radio;
    std::function<void()> onSendCallback = nullptr;

protected:
    bool preambleTimedOutFlag = false;

    void handleIRQFlags();

    virtual void onReceiveIR() {};
    virtual void onPreambleDetectedIR() {};

public:

    void assignRadio(SX1262Public *_radio);
    void sendPacket(const uint8_t *data, const size_t len);

    void startReceive();
    bool isReceiving();
    bool isTransmitting();
    float getRSSI();
    float getSNR();
    void standby();
    size_t getPacketLength();
    int readData(uint8_t *data, size_t len);
    void setReceivingVar(bool s);
    void setOnSendCallback(std::function<void()> cb);
    void logSendStatistics(const uint8_t *data, const size_t len);
    bool hasPreambleTimedOut() const;

    void finishRadioBase();
};
