#include <Arduino.h>
#include <SPI.h>
#include <C:\Users\Anushka Gulati\source\repos\RobotNetwork\src\lib\LoRa.h>
#include "C:\Users\Anushka Gulati\source\repos\RobotNetwork\include\config.h"

// This class defines the LoRa parameters for a node and functions to intialize LoRa node and transmit messages.
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