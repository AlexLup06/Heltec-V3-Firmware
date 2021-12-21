#include <mesh/loranode.h>

#define MESH_FREQUENCY 869400000

void LoraNode::initLora()
{
    SPI.begin(SCK, MISO, MOSI, SS);

    LoRa.setPins(SS, RST_LoRa, DIO0);
    if (!LoRa.begin(MESH_FREQUENCY, false))
    {
        Serial.println("LoRa init failed. Check your connections.");
        while (true)
            ;
    }
    // Default:
    LoRa.setTxPower(15, RF_PACONFIG_PASELECT_RFO);
    LoRa.setSignalBandwidth(250E3);
    LoRa.setSpreadingFactor(8);

    // Eigenes Sync-Word setzen, damit Pakete anderer LoRa Netzwerke ignoriert werden
    LoRa.setSyncWord(0x12);
}