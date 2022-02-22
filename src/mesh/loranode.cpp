#include <mesh/loranode.h>
#include "config.h"

void LoraNode::initLora()
{
    SPI.begin(SCK, MISO, MOSI, SS);

    LoRa.setPins(SS, RST_LoRa, DIO0);
    if (!LoRa.begin(LORA_FREQUENCY, false))
    {
        Serial.println("LoRa init failed. Check your connections.");
        while (true)
            ;
    }
    // Default:
    // High Power:
    LoRa.setTxPower(20, RF_PACONFIG_PASELECT_PABOOST);
    LoRa.setSignalBandwidth(LORA_BANDWIDTH);
    LoRa.setSpreadingFactor(LORA_SPREADINGFACTOR);
    LoRa.setPreambleLength(LORA_PREAMBLE_LENGTH);


    // Eigenes Sync-Word setzen, damit Pakete anderer LoRa Netzwerke ignoriert werden
    LoRa.setSyncWord(0x12);
}