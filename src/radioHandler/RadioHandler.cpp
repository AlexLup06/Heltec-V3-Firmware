// RadioHandler.cpp
#include "radioHandler/RadioHandler.h"

SX1262Public radio = SX1262Public(new Module(LORA_CS, LORA_DIO1, LORA_RST, LORA_BUSY));
// SX1262 radio = SX1262(new Module(LORA_CS, LORA_DIO1, LORA_RST, LORA_BUSY));

void initRadio(float frequency, uint8_t sf, uint8_t txPower, uint32_t bw) {
    SPI.begin();

    int state = radio.begin(frequency);
    if (state != RADIOLIB_ERR_NONE) {
        Serial.printf("Radio init failed: %d\n", state);
        while (true);
    }

    radio.setSpreadingFactor(sf);
    radio.setOutputPower(txPower);
    radio.setBandwidth(bw);
    radio.setCodingRate(5);
    radio.setSyncWord(0x12);
    radio.setPreambleLength(8);
}

void reInitRadio(float m_frequency, uint8_t m_sf, uint8_t m_txPower, uint32_t m_bw){
    // Put SX1262 into standby and small delay
    radio.standby();
    vTaskDelay(pdMS_TO_TICKS(50));

    // Reinitialize the modem with new frequency
    int state = radio.begin(m_frequency);
    if (state != RADIOLIB_ERR_NONE) {
        Serial.printf("SX1262 re-init failed! Code: %d\n", state);
        while (true);
    }

    // Apply modulation parameters
    radio.setOutputPower(m_txPower);   // 0–22 dBm
    radio.setBandwidth(m_bw);              // Hz or MHz (RadioLib auto-handles)
    radio.setSpreadingFactor(m_sf); // SF7–SF12
    radio.setCodingRate(5);                       // CR 4/5 (default)
    radio.setPreambleLength(12);
    radio.setSyncWord(0x12);

    // Optionally restart continuous receive mode
    radio.startReceive();

    Serial.println("SX1262 re-initialized successfully!");
}


void startReceive() {
    radio.startReceive();
}

void sendPacket(uint8_t *data, size_t len) {
    int state = radio.transmit(data, len);
    if (state == RADIOLIB_ERR_NONE) {
        Serial.println("Packet sent!");
    } else {
        Serial.printf("Send failed: %d\n", state);
    }
    radio.startReceive();
}

float getRSSI() {
    return radio.getRSSI();
}

float getSNR() {
    return radio.getSNR();
}