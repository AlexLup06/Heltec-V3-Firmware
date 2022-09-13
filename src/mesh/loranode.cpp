#include "mesh\loranode.h"


// Initialize LoRa communication. This function starts a SPI communication and sets LoRa module pins. It sets the Tx power, signal bandwidth, spreading factor and preamble length to their default values. It also sets a sync word to avoid interference from other Lora networks.
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
    LoRa.setTxPower(LORA_TRANSMISSION_POWER, RF_PACONFIG_PASELECT_PABOOST);
    LoRa.setSignalBandwidth(LORA_BANDWIDTH);
    LoRa.setSpreadingFactor(LORA_SPREADINGFACTOR);
    LoRa.setPreambleLength(LORA_PREAMBLE_LENGTH);


    // Set your own sync word so that packets from other LoRa networks are ignored.
    // Eigenes Sync - Word setzen, damit Pakete anderer LoRa Netzwerke ignoriert werden.
    LoRa.setSyncWord(0x12);
}