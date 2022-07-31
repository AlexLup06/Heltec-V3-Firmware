#include <Arduino.h>
#include <SPI.h>
#include <lib/LoRa.h>

class LoraNode
{
public:
    int spreadfactor;
    int8_t power;

    LoraNode()
    {
    }

    void send();

    void initLora();
};