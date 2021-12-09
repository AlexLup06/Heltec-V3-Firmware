#include <Arduino.h>

class LoraNode
{
public:
    int spreadfactor;
    long frequency;
    int8_t power;

    LoraNode(long frequency1, int spreadfactor1)
    {
        spreadfactor = spreadfactor1;
        frequency = frequency1;
    }

    void send();
};