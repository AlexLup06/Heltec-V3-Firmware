#include "RadioHandler.h"

SX1262Public *SX1262Public::instance = nullptr;

SX1262Public::SX1262Public(Module *module) : SX1262(module)
{
    instance = this;
}

int16_t SX1262Public::startReceive()
{
    int16_t s = SX1262::startReceive();
    if (s == RADIOLIB_ERR_NONE)
    {
        this->setDioIrqParams(
            RADIOLIB_SX126X_IRQ_RX_DONE |
                RADIOLIB_SX126X_IRQ_PREAMBLE_DETECTED |
                RADIOLIB_SX126X_IRQ_HEADER_VALID |
                RADIOLIB_SX126X_IRQ_HEADER_ERR |
                RADIOLIB_SX126X_IRQ_CRC_ERR,
            RADIOLIB_SX126X_IRQ_RX_DONE |
                RADIOLIB_SX126X_IRQ_PREAMBLE_DETECTED |
                RADIOLIB_SX126X_IRQ_HEADER_ERR |
                RADIOLIB_SX126X_IRQ_CRC_ERR);
    }
    return s;
}

void SX1262Public::init(float frequency, uint8_t sf, uint8_t txPower, uint32_t bw)
{
    int state = this->begin(frequency);
    if (state != RADIOLIB_ERR_NONE)
    {
        DEBUG_PRINTF("Radio init failed: %d\n", state);
        while (true)
            ;
    }

    this->setSpreadingFactor(sf);
    this->setOutputPower(txPower);
    this->setBandwidth(bw);
    this->setCodingRate(5);
    this->setSyncWord(0x12);
    this->setPreambleLength(8);

    this->setDio1Action(receiveDio1Interrupt);

    DEBUG_PRINTLN("Radio initialized successfully!");
}

void SX1262Public::reInitRadio(float frequency, uint8_t sf, uint8_t txPower, uint32_t bw)
{
    this->standby();
    vTaskDelay(pdMS_TO_TICKS(50));

    int state = this->begin(frequency);
    if (state != RADIOLIB_ERR_NONE)
    {
        DEBUG_PRINTF("SX1262 re-init failed! Code: %d\n", state);
        while (true)
            ;
    }

    this->setSpreadingFactor(sf);
    this->setOutputPower(txPower);
    this->setBandwidth(bw);
    this->setCodingRate(5);
    this->setSyncWord(0x12);
    this->setPreambleLength(8);

    this->setDio1Action(receiveDio1Interrupt);

    DEBUG_PRINTLN("SX1262 re-initialized successfully!");
}

void SX1262Public::receiveDio1Interrupt()
{
    if (instance)
    {
        instance->handleDio1Interrupt();
    }
}

void SX1262Public::handleDio1Interrupt()
{
    uint16_t irq = this->getIrqFlags();

    if (irq & RADIOLIB_SX126X_IRQ_TX_DONE)
        irqFlag |= RADIOLIB_SX126X_IRQ_TX_DONE;

    if (irq & RADIOLIB_SX126X_IRQ_PREAMBLE_DETECTED)
        irqFlag |= RADIOLIB_SX126X_IRQ_PREAMBLE_DETECTED;

    if (irq & RADIOLIB_SX126X_IRQ_RX_DONE)
        irqFlag |= RADIOLIB_SX126X_IRQ_RX_DONE;

    if (irq & RADIOLIB_SX126X_IRQ_CRC_ERR)
        irqFlag |= RADIOLIB_SX126X_IRQ_CRC_ERR;

    if (irq & RADIOLIB_SX126X_IRQ_HEADER_ERR)
        irqFlag |= RADIOLIB_SX126X_IRQ_HEADER_ERR;

    const uint16_t RX_IRQ_MASK =
        RADIOLIB_SX126X_IRQ_TX_DONE |
        RADIOLIB_SX126X_IRQ_RX_DONE |
        RADIOLIB_SX126X_IRQ_PREAMBLE_DETECTED |
        RADIOLIB_SX126X_IRQ_HEADER_ERR |
        RADIOLIB_SX126X_IRQ_CRC_ERR;

    uint16_t relevant = irq & RX_IRQ_MASK;
    if (relevant)
        this->clearIrqFlags(relevant);
}

int SX1262Public::sendRaw(const uint8_t *data, const size_t len)
{
    int state = this->startTransmit(data, len);
    if (state == RADIOLIB_ERR_NONE)
    {
        DEBUG_PRINTF("Started TX %s\n", msgIdToString(data[0] & 0x7F));
    }
    else
    {
        DEBUG_PRINTF("Send failed: %d\n", state);
    }

    return state;
}