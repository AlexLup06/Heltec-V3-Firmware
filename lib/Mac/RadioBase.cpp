#include "RadioBase.h"

void RadioBase::receiveDio1Interrupt()
{
    uint16_t irq = radio->getIrqFlags();
    radio->clearIrqFlags(irq);

    if (irq & RADIOLIB_SX126X_IRQ_PREAMBLE_DETECTED)
        onPreambleDetectedIR();

    if (irq & RADIOLIB_SX126X_IRQ_RX_DONE)
        onReceiveIR();

    if (irq & RADIOLIB_SX126X_IRQ_CRC_ERR)
        onCRCerrorIR();
}

void RadioBase::initRadio(float frequency, uint8_t sf, uint8_t txPower, uint32_t bw)
{
    SPI.begin();

    int state = radio->begin(frequency);
    if (state != RADIOLIB_ERR_NONE)
    {
        Serial.printf("Radio init failed: %d\n", state);
        while (true)
            ;
    }

    radio->setSpreadingFactor(sf);
    radio->setOutputPower(txPower);
    radio->setBandwidth(bw);
    radio->setCodingRate(5);
    radio->setSyncWord(0x12);
    radio->setPreambleLength(8);
}

void RadioBase::reInitRadio(float m_frequency, uint8_t m_sf, uint8_t m_txPower, uint32_t m_bw)
{
    // Put SX1262 into standby and small delay
    radio->standby();
    vTaskDelay(pdMS_TO_TICKS(50));

    // Reinitialize the modem with new frequency
    int state = radio->begin(m_frequency);
    if (state != RADIOLIB_ERR_NONE)
    {
        Serial.printf("SX1262 re-init failed! Code: %d\n", state);
        while (true)
            ;
    }

    // Apply modulation parameters
    radio->setOutputPower(m_txPower); // 0–22 dBm
    radio->setBandwidth(m_bw);        // Hz or MHz (RadioLib auto-handles)
    radio->setSpreadingFactor(m_sf);  // SF7–SF12
    radio->setCodingRate(5);          // CR 4/5 (default)
    radio->setPreambleLength(12);
    radio->setSyncWord(0x12);

    // Optionally restart continuous receive mode
    radio->startReceive();

    Serial.println("SX1262 re-initialized successfully!");
}

void RadioBase::startReceive()
{
    radio->startReceive();
}

bool RadioBase::isChannelFree()
{
    // TODO: check if channel free
    return true;
}

void RadioBase::sendPacket(const uint8_t *data, const size_t len)
{
    isTransmittingVar = true;
    int state = radio->transmit(data, len);
    if (state == RADIOLIB_ERR_NONE)
    {
        Serial.println("Packet sent!");
    }
    else
    {
        Serial.printf("Send failed: %d\n", state);
    }
    isTransmittingVar = false;
    radio->startReceive();
}

bool RadioBase::isReceiving()
{
    return isReceivingVar;
}
bool RadioBase::isTransmitting()
{
    return isTransmittingVar;
}

float RadioBase::getRSSI()
{
    return radio->getRSSI();
}

float RadioBase::getSNR()
{
    return radio->getSNR();
}