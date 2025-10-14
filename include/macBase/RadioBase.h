#pragma once

enum RadioMode{
    RECEIVER,
    TRANSMITTER,
    IDLE,
};

enum TransmitterState {
    IDLE,
    TRANSMITTING,
    UNDEFINED,
};

enum ReceiverState {
    IDLE,
    RECEIVING,
    UNDEFINED,
};

class RadioBase {

    private: 
        bool isReceiving = false;
        RadioMode radioMode;
        TransmitterState transmitterState = TransmitterState::IDLE;
        ReceiverState receiverState = ReceiverState::IDLE;


    public:
        void setRadioMode(RadioMode newRadioMode);
        bool isReceiving();
        bool isFreeToSend();
};
