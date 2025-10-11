#ifndef RADIOBASE_h
#define RADIOBASE_H

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
    IDEL,
    RECEIVING,
    UNDEFINED,
};

class RadioBase {

    private: 
        bool isReceiving = false;
        RadioMode radioMode;
        TransmitterState transmitterState = IDLE;
        ReceiverState receiverState = IDLE;


    public:
        void setRadioMode(RadioMode newRadioMode);
        bool isReceiving();
        bool isFreeToSend();
};

#endif 
