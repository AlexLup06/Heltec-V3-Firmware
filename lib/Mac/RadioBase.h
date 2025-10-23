#pragma once

#include <cstdint>
#include <RadioLib.h>
#include <RadioHandler.h>

enum class RadioMode {
    RECEIVER,
    TRANSMITTER,
    IDLE
};

enum class TransmitterState {
    IDLE,
    TRANSMITTING,
    UNDEFINED
};

enum class ReceiverState {
    IDLE,
    RECEIVING,
    UNDEFINED
};


class RadioBase
{

private:
    bool isReceivingVar = false;
    bool isTransmittingVar = false;
    RadioMode radioMode;
    TransmitterState transmitterState = TransmitterState::IDLE;
    ReceiverState receiverState = ReceiverState::IDLE;

public:
    SX1262Public *radio;

    void setRadioMode(RadioMode newRadioMode);
    bool isReceiving();
    bool isTransmitting();
    bool isChannelFree();
    void receiveDio1Interrupt();
    void startReceive();
    float getRSSI();
    float getSNR();
    void sendPacket(const uint8_t *data, const size_t len);
    void reInitRadio(float m_frequency, uint8_t m_sf, uint8_t m_txPower, uint32_t m_bw);
    void initRadio(float frequency, uint8_t sf, uint8_t txPower, uint32_t bw);

protected:
    virtual void onReceiveIR() = 0;
    virtual void onPreambleDetectedIR() = 0;
    virtual void onCRCerrorIR() = 0;
};
