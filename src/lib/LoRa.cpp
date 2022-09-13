#include "LoRa.h"

// Registers.
#define REG_IRQ_FLAGS 0x12
#define REG_RX_NB_BYTES 0x13
#define REG_PKT_RSSI_VALUE 0x1a
#define REG_PKT_SNR_VALUE 0x1b
#define REG_MODEM_CONFIG_1 0x1d
#define REG_MODEM_CONFIG_2 0x1e
#define REG_PREAMBLE_MSB 0x20

#define REG_PREAMBLE_LSB 0x21
#define REG_MODEM_CONFIG_3 0x26
#define REG_RSSI_WIDEBAND 0x2c
#define REG_DETECTION_OPTIMIZE 0x31
#define REG_DETECTION_THRESHOLD 0x37
#define REG_SYNC_WORD 0x39
#define REG_DIO_MAPPING_1 0x40
#define REG_VERSION 0x42
#define REG_PaDac 0x4d //add REG_PaDac

// Modes.
#define MODE_LONG_RANGE_MODE 0x80
#define MODE_SLEEP 0x00
#define MODE_STDBY 0x01
#define MODE_TX 0x03
#define MODE_RX_CONTINUOUS 0x05
#define MODE_RX_SINGLE 0x06

// PA config
//#define PA_BOOST                 0x80
//#define RFO                      0x70
// IRQ masks
#define IRQ_TX_DONE_MASK 0x08

// Incoming LoRa message, header received, see datasheet page 90. Bit 0 Mask: 00010000 -> 0x10
#define IRQ_HEADER_RECEIVED 0x10

// LoRa signal received, see data sheet page 90. Bit 0 Mask: 00000001 -> 0x01
#define IRQ_CAD_DETECTED 0x01
#define IRQ_PAYLOAD_CRC_ERROR_MASK 0x20
#define IRQ_RX_DONE_MASK 0x40

/*!
 * RegInvertIQ
 */
#define RFLR_INVERTIQ_RX_MASK 0xBF
#define RFLR_INVERTIQ_RX_OFF 0x00
#define RFLR_INVERTIQ_RX_ON 0x40
#define RFLR_INVERTIQ_TX_MASK 0xFE
#define RFLR_INVERTIQ_TX_OFF 0x01
#define RFLR_INVERTIQ_TX_ON 0x00

#define REG_LR_INVERTIQ 0x33

/*!
 * RegInvertIQ2
 */
#define RFLR_INVERTIQ2_ON 0x19
#define RFLR_INVERTIQ2_OFF 0x1D

#define REG_LR_INVERTIQ2 0x3B
#define MAX_PKT_LENGTH 255

// Constructor to initialize LoRa default parameters.
LoRaClass::LoRaClass() : _spiSettings(8E6, MSBFIRST, SPI_MODE0),
                         _ss(LORA_DEFAULT_SS_PIN), _reset(LORA_DEFAULT_RESET_PIN), _dio0(LORA_DEFAULT_DIO0_PIN),
                         _frequency(0),
                         _packetIndex(0),
                         _implicitHeaderMode(0),
                         _onReceive(NULL),
                         _onCad(NULL){
    // overide Stream timeout value
    setTimeout(0);
}

/*  This function is used to initialize the library with the specified frequency and start the SPI communication. 
* Input parameters: transmission frequency, a boolean for transmission power
*/
int LoRaClass::begin(long frequency, bool PABOOST) {
    // setup pins
    pinMode(_ss, OUTPUT);
    pinMode(_reset, OUTPUT);
    pinMode(_dio0, INPUT);
    // perform reset
    digitalWrite(_reset, LOW);
    delay(20);
    digitalWrite(_reset, HIGH);
    delay(50);
    // set SS high
    digitalWrite(_ss, HIGH);
    // start SPI
    SPI.begin();
    // check version
    uint8_t version = readRegister(REG_VERSION);
    if (version != 0x12) {
        return 0;
    }
    // put in sleep mode
    sleep();
    // set frequency
    setFrequency(frequency);
    // set base addresses
    writeRegister(REG_FIFO_TX_BASE_ADDR, 0);
    writeRegister(REG_FIFO_RX_BASE_ADDR, 0);
    // set LNA boost
    writeRegister(REG_LNA, readRegister(REG_LNA) | 0x03);
    // set auto AGC
    writeRegister(REG_MODEM_CONFIG_3, 0x04);
    // set output power to 14 dBm
    if (PABOOST == true)
        setTxPower(14, RF_PACONFIG_PASELECT_PABOOST);
    else
        setTxPower(14, RF_PACONFIG_PASELECT_RFO);
    // set spreading factor
    setSpreadingFactor(11);
    // put in standby mode
    setSignalBandwidth(125E3);
    // set coding rate
    setCodingRate4(5);
    // set syncword
    setSyncWord(0x34);
    // disable Cyclic redundancy check
    disableCrc();
    // enable Cyclic redundancy check to detect errors
    crc();
    // put in standby mode
    idle();
    return 1;
}

// This function puts the LoRa module in sleep mode and disables the SPI bus.
void LoRaClass::end() {
    sleep();
    SPI.end();
}

/* Start the sequence of sending a packet. Returns 1 if radio is ready to transmit, 0 if busy or on failure.
* Input parameters: implicitHeader(optional)- true enables implicit header mode, false enables explicit header mode (default)
*/ 
int LoRaClass::beginPacket(int implicitHeader) {
    // put in standby mode
    idle();
    if (implicitHeader) {
        implicitHeaderMode();
    } else {
        explicitHeaderMode();
    }
    // reset FIFO address and payload length
    writeRegister(REG_FIFO_ADDR_PTR, 0);
    writeRegister(REG_PAYLOAD_LENGTH, 0);
    return 1;
}
/* End the sequence of sending a packet. Returns 1 on success, 0 on failure.
* Input parameters: boolean variable async - true enables non-blocking mode, false waits for transmission to be completed(default)
*/ 
int LoRaClass::endPacket(bool async) {
    // put in TX mode
    writeRegister(REG_OP_MODE, MODE_LONG_RANGE_MODE | MODE_TX);

    if (async) {
        // grace time is required for the radio
        delayMicroseconds(150);
    } else {
        // wait for TX done
        while ((readRegister(REG_IRQ_FLAGS) & IRQ_TX_DONE_MASK) == 0) {
            yield();
        }
        // clear IRQ's
        writeRegister(REG_IRQ_FLAGS, IRQ_TX_DONE_MASK);
    }

    return 1;
}

/* This function checks if a packet has been received. Returns the packet size in bytes or 0 if no packet was received.
* Input parameters: size - if > 0 implicit header mode is enabled with the expected packet of size bytes, default mode is explicit header mode
*/ 
int LoRaClass::parsePacket(int size) {
    int packetLength = 0;
    int irqFlags = readRegister(REG_IRQ_FLAGS);

    if (size > 0) {
        implicitHeaderMode();
        writeRegister(REG_PAYLOAD_LENGTH, size & 0xff);
    } else {
        explicitHeaderMode();
    }

    // clear IRQ's
    writeRegister(REG_IRQ_FLAGS, irqFlags);

    if ((irqFlags & IRQ_RX_DONE_MASK) && (irqFlags & IRQ_PAYLOAD_CRC_ERROR_MASK) == 0) {
        // received a packet
        _packetIndex = 0;
        // Read packet length. If no packet received, set packet length to 0.           
        if (_implicitHeaderMode) {
            packetLength = readRegister(REG_PAYLOAD_LENGTH);
        } else {
            packetLength = readRegister(REG_RX_NB_BYTES);
        }
        // Store received value to FIFO address
        writeRegister(REG_FIFO_ADDR_PTR, readRegister(REG_FIFO_RX_CURRENT_ADDR));
        // put in standby mode
        idle();
    } else if (readRegister(REG_OP_MODE) != (MODE_LONG_RANGE_MODE | MODE_RX_SINGLE)) {
        // not currently in RX mode
        // reset FIFO address
        writeRegister(REG_FIFO_ADDR_PTR, 0);
        // put in single RX mode
        writeRegister(REG_OP_MODE, MODE_LONG_RANGE_MODE | MODE_RX_SINGLE);
    }
    return packetLength;
}

/* Returns signal - to - noise ratio (SNR) of the latest received packet.
*/
int8_t LoRaClass::getSNR()
{
	int8_t snr = 0;
	int8_t SnrValue = readRegister(0x19);
	if (SnrValue & 0x80) // The SNR sign bit is 1
	{
		// Invert and divide by 4
		snr = ((~SnrValue + 1) & 0xFF) >> 2;
		snr = -snr;
	}
	else {
		// Divide by 4
		snr = (SnrValue & 0xFF) >> 2;
	}
    return snr;
}

// Returns the signal strength indicator (RSSI) of the radio (dBm).
int LoRaClass::packetRssi() {
    int8_t snr = 0;
    int8_t SnrValue = readRegister(0x19);
    int16_t rssi = readRegister(REG_PKT_RSSI_VALUE);

    if (SnrValue & 0x80) // The SNR sign bit is 1
    {
        // Invert and divide by 4
        snr = ((~SnrValue + 1) & 0xFF) >> 2;
        snr = -snr;
    } else {
        // Divide by 4
        snr = (SnrValue & 0xFF) >> 2;
    }
    if (snr < 0) {
        rssi = rssi - (_frequency < 525E6 ? 164 : 157) + (rssi >> 4) + snr;
    } else {
        rssi = rssi - (_frequency < 525E6 ? 164 : 157) + (rssi >> 4);
    }

    return (rssi);
}

// Returns the estimated SNR of the received packet in dB.
float LoRaClass::packetSnr() {
    return ((int8_t) readRegister(REG_PKT_SNR_VALUE)) * 0.25;
}

/*This function writes data to the packet. Each packet can contain up to 255 bytes. Returns the number of bytes written.
* Input parameters: buffer data
*/ 
size_t LoRaClass::write(uint8_t byte) {
    return write(&byte, sizeof(byte));
}

/* This function writes data to the packet. Each packet can contain up to 255 bytes. Returns the number of bytes written.
* Input parameters: buffer data, size of buffer data
*/ 
size_t LoRaClass::write(const uint8_t *buffer, size_t size) {
    // read current payload length
    int currentLength = readRegister(REG_PAYLOAD_LENGTH);
    // check remaining empty space in packet 
    if ((currentLength + size) > MAX_PKT_LENGTH) {
        size = MAX_PKT_LENGTH - currentLength;
    }
    // write as much data into the packet as can be accommodated
    for (size_t i = 0; i < size; i++) {
        writeRegister(REG_FIFO, buffer[i]);
    }
    // update length of payload
    writeRegister(REG_PAYLOAD_LENGTH, currentLength + size);
    return size;
}

// Returns number of bytes available for reading.
int LoRaClass::available() {
    return (readRegister(REG_RX_NB_BYTES) - _packetIndex);
}

// Returns the next byte in the packet or -1 if no bytes are available.
int LoRaClass::read() {
    if (!available()) {
        return -1;
    }
    _packetIndex++;
    return readRegister(REG_FIFO);
}

// Peek at the next byte in the packet. Returns -1 if no bytes are available.
int LoRaClass::peek() {
    if (!available()) {
        return -1;
    }
    // store current FIFO address
    int currentAddress = readRegister(REG_FIFO_ADDR_PTR);
    // read
    uint8_t b = readRegister(REG_FIFO);
    // restore FIFO address
    writeRegister(REG_FIFO_ADDR_PTR, currentAddress);
    return b;
}

void LoRaClass::flush() {
}

/* Register a callback function for when a packet is received.
* Input paramters: function to call when a packet is received.
*/ 
void LoRaClass::onReceive(void (*callback)(int)) {
    _onReceive = callback;

   // The dio0 pin is used for receiving callback. It handles external interrupts.
    if (callback) {
        writeRegister(REG_DIO_MAPPING_1, 0x00);
        attachInterrupt(digitalPinToInterrupt(_dio0), LoRaClass::onDio0Rise, RISING);
        //    attachInterrupt(digitalPinToInterrupt(_dio0), LoRaClass::onDio0Rise, RISING);
    } else {
        detachInterrupt(digitalPinToInterrupt(_dio0));
    }
}

//
/* Always called on detecting a LoRa packet preamble
*Input paramters : callback function.
*/ 
void LoRaClass::onCad(void (*callback)()) {
    _onCad = callback;
}

/* Puts the radio in continuous receive mode.
*Input paramters : size - (optional) if > 0 implicit header mode is enabled with the expected a packet of size bytes, default mode is explicit header mode.
*/
void LoRaClass::receive(int size) {
    if (size > 0) {
        implicitHeaderMode();
        writeRegister(REG_PAYLOAD_LENGTH, size & 0xff);
    } else {
        explicitHeaderMode();
    }
    //
    writeRegister(REG_OP_MODE, MODE_LONG_RANGE_MODE | MODE_RX_CONTINUOUS);
}

// Put the radio in idle (standby) mode.
void LoRaClass::idle() {
    writeRegister(REG_OP_MODE, MODE_LONG_RANGE_MODE | MODE_STDBY);
}

// Put the radio in sleep mode.
void LoRaClass::sleep() {
    writeRegister(REG_OP_MODE, MODE_LONG_RANGE_MODE | MODE_SLEEP);
}

/* This function changes the TX power of the radio.
* Input parameters: txPower - TX power in dB, defaults to 17, outputPin - (optional) PA output pin.
*/
void LoRaClass::setTxPower(int8_t power, int8_t outputPin) {
    uint8_t paConfig = 0;
    uint8_t paDac = 0;

    paConfig = readRegister(REG_PA_CONFIG);
    paDac = readRegister(REG_PaDac);

    paConfig = (paConfig & RF_PACONFIG_PASELECT_MASK) | outputPin;
    paConfig = (paConfig & RF_PACONFIG_MAX_POWER_MASK) | 0x70;

    if ((paConfig & RF_PACONFIG_PASELECT_PABOOST) == RF_PACONFIG_PASELECT_PABOOST) {
        if (power > 17) {
            paDac = (paDac & RF_PADAC_20DBM_MASK) | RF_PADAC_20DBM_ON;
        } else {
            paDac = (paDac & RF_PADAC_20DBM_MASK) | RF_PADAC_20DBM_OFF;
        }
        if ((paDac & RF_PADAC_20DBM_ON) == RF_PADAC_20DBM_ON) {
            if (power < 5) {
                power = 5;
            }
            if (power > 20) {
                power = 20;
            }
            paConfig = (paConfig & RF_PACONFIG_OUTPUTPOWER_MASK) | (uint8_t) ((uint16_t) (power - 5) & 0x0F);
        } else {
            if (power < 2) {
                power = 2;
            }
            if (power > 17) {
                power = 17;
            }
            paConfig = (paConfig & RF_PACONFIG_OUTPUTPOWER_MASK) | (uint8_t) ((uint16_t) (power - 2) & 0x0F);
        }
    } else {
        if (power < -1) {
            power = -1;
        }
        if (power > 14) {
            power = 14;
        }
        paConfig = (paConfig & RF_PACONFIG_OUTPUTPOWER_MASK) | (uint8_t) ((uint16_t) (power + 1) & 0x0F);
    }
    writeRegister(REG_PA_CONFIG, paConfig);
    writeRegister(REG_PaDac, paDac);
}

/* This function sets the power of the Lora radio to the maximum.
* Input parameters: level
*/
void LoRaClass::setTxPowerMax(int level) {
    if (level < 5) {
        level = 5;
    } else if (level > 20) {
        level = 20;
    }
    writeRegister(REG_LR_OCP, 0x3f);
    writeRegister(REG_PaDac, 0x87); //Open PA_BOOST
    writeRegister(REG_PA_CONFIG, RF_PACONFIG_PASELECT_PABOOST | (level - 5));
}

/* This function sets the frequency of the radio.
* Input parameters: frequency in Hz 
*/
void LoRaClass::setFrequency(long frequency) {
    _frequency = frequency;
    uint64_t frf = ((uint64_t) frequency << 19) / 32000000;
    writeRegister(REG_FRF_MSB, (uint8_t) (frf >> 16));
    writeRegister(REG_FRF_MID, (uint8_t) (frf >> 8));
    writeRegister(REG_FRF_LSB, (uint8_t) (frf >> 0));
}

/* This function sets the spreading factor of the radio.
* Input parameters:  spreading factor- ranges from 6 to 12
*/
void LoRaClass::setSpreadingFactor(int sf) {
    if (sf < 6) {
        sf = 6;
    } else if (sf > 12) {
        sf = 12;
    }
    if (sf == 6) {
        writeRegister(REG_DETECTION_OPTIMIZE, 0xc5);
        writeRegister(REG_DETECTION_THRESHOLD, 0x0c);
    } else {
        writeRegister(REG_DETECTION_OPTIMIZE, 0xc3);
        writeRegister(REG_DETECTION_THRESHOLD, 0x0a);
    }
    writeRegister(REG_MODEM_CONFIG_2, (readRegister(REG_MODEM_CONFIG_2) & 0x0f) | ((sf << 4) & 0xf0));
}

/* This function sets the signal bandwidth of the radio.
* Input parameters: signal bandwidth in Hz
*/
void LoRaClass::setSignalBandwidth(long sbw) {
    int bw;

    if (sbw <= 7.8E3) {
        bw = 0;
    } else if (sbw <= 10.4E3) {
        bw = 1;
    } else if (sbw <= 15.6E3) {
        bw = 2;
    } else if (sbw <= 20.8E3) {
        bw = 3;
    } else if (sbw <= 31.25E3) {
        bw = 4;
    } else if (sbw <= 41.7E3) {
        bw = 5;
    } else if (sbw <= 62.5E3) {
        bw = 6;
    } else if (sbw <= 125E3) {
        bw = 7;
    } else if (sbw <= 250E3) {
        bw = 8;
    } else /*if (sbw <= 250E3)*/
    {
        bw = 9;
    }
    writeRegister(REG_MODEM_CONFIG_1, (readRegister(REG_MODEM_CONFIG_1) & 0x0f) | (bw << 4));
}

/* This function sets the coding rate of the radio.
* Input parameters: denominator of the coding rate- ranges 5 to 8
*/
void LoRaClass::setCodingRate4(int denominator) {
    if (denominator < 5) {
        denominator = 5;
    } else if (denominator > 8) {
        denominator = 8;
    }
    int cr = denominator - 4;
    writeRegister(REG_MODEM_CONFIG_1, (readRegister(REG_MODEM_CONFIG_1) & 0xf1) | (cr << 1));
}

/* This function sets preamble length of the radio.
* Input parameters: preamble length in symbols
*/
void LoRaClass::setPreambleLength(long length) {
    writeRegister(REG_PREAMBLE_MSB, (uint8_t) (length >> 8));
    writeRegister(REG_PREAMBLE_LSB, (uint8_t) (length >> 0));
}

/* This function sets  the sync word of the radio.
* Input parameters:  byte value to use as the sync word
*/
void LoRaClass::setSyncWord(int sw) {
    writeRegister(REG_SYNC_WORD, sw);
}

/* This function enables Cyclic Redundancy Check.*/
void LoRaClass::enableCrc() {
    writeRegister(REG_MODEM_CONFIG_2, readRegister(REG_MODEM_CONFIG_2) | 0x04);
}

/* This function disables Cyclic Redundancy Check.*/
void LoRaClass::disableCrc() {
    writeRegister(REG_MODEM_CONFIG_2, readRegister(REG_MODEM_CONFIG_2) & 0xfb);
}

/* This function enables TX Invert the LoRa I and Q signals. */
void LoRaClass::enableTxInvertIQ() {
    writeRegister(REG_LR_INVERTIQ, ((readRegister(REG_LR_INVERTIQ) & RFLR_INVERTIQ_TX_MASK & RFLR_INVERTIQ_RX_MASK) |
                                    RFLR_INVERTIQ_RX_OFF | RFLR_INVERTIQ_TX_ON));
    writeRegister(REG_LR_INVERTIQ2, RFLR_INVERTIQ2_ON);
}

/* This function enables RX Invert the LoRa I and Q signals. */
void LoRaClass::enableRxInvertIQ() {
    writeRegister(REG_LR_INVERTIQ, ((readRegister(REG_LR_INVERTIQ) & RFLR_INVERTIQ_TX_MASK & RFLR_INVERTIQ_RX_MASK) |
                                    RFLR_INVERTIQ_RX_ON | RFLR_INVERTIQ_TX_OFF));
    writeRegister(REG_LR_INVERTIQ2, RFLR_INVERTIQ2_ON);
}

/* This function disables Invert the LoRa I and Q signals. */
void LoRaClass::disableInvertIQ() {
    writeRegister(REG_LR_INVERTIQ, ((readRegister(REG_LR_INVERTIQ) & RFLR_INVERTIQ_TX_MASK & RFLR_INVERTIQ_RX_MASK) |
                                    RFLR_INVERTIQ_RX_OFF | RFLR_INVERTIQ_TX_OFF));
    writeRegister(REG_LR_INVERTIQ2, RFLR_INVERTIQ2_OFF);
}

/* This function enables Invert the LoRa I and Q signals. */
void LoRaClass::enableInvertIQ() {
    writeRegister(REG_LR_INVERTIQ, ((readRegister(REG_LR_INVERTIQ) & RFLR_INVERTIQ_TX_MASK & RFLR_INVERTIQ_RX_MASK) |
                                    RFLR_INVERTIQ_RX_ON | RFLR_INVERTIQ_TX_ON));
    writeRegister(REG_LR_INVERTIQ2, RFLR_INVERTIQ2_ON);
}

/* This function generates and returns a random byte, based on the Wideband RSSI measurement.
*/
byte LoRaClass::random() {
    return readRegister(REG_RSSI_WIDEBAND);
}

/* Override the default NSS, NRESET, and DIO0 pins used by the library.
* Input parameters: new slave select pin, new reset pin, new DIO0 pin 
*/
void LoRaClass::setPins(int ss, int reset, int dio0) {
    _ss = ss;
    _reset = reset;
    _dio0 = dio0;
}

/* This function overrides the default SPI frequency of 10 MHz used by the library. 
* Input parameters:  new SPI frequency 
*/
void LoRaClass::setSPIFrequency(uint32_t frequency) {
    _spiSettings = SPISettings(frequency, MSBFIRST, SPI_MODE0);
}

/* This function will send in log a dump of registers: it can be a way to check if SPI is OK and if access to hard is OK
* Input parameters: serial dump data
*/
void LoRaClass::dumpRegisters(Stream &out) {
    for (int i = 0; i < 128; i++) {
        out.print("0x");
        out.print(i, HEX);
        out.print(": 0x");
        out.println(readRegister(i), HEX);
    }
}

// Sets LoRa explicit header mode
void LoRaClass::explicitHeaderMode() {
    _implicitHeaderMode = 0;
    writeRegister(REG_MODEM_CONFIG_1, readRegister(REG_MODEM_CONFIG_1) & 0xfe);
}

// Sets LoRa implicit header mode (fixed length transmission)
void LoRaClass::implicitHeaderMode() {
    _implicitHeaderMode = 1;
    writeRegister(REG_MODEM_CONFIG_1, readRegister(REG_MODEM_CONFIG_1) | 0x01);
}

/* Handles interrupts with the onReceive callback function.
*/
void LoRaClass::handleDio0Rise() {
    int irqFlags = readRegister(REG_IRQ_FLAGS);
    // clear IRQ's
    writeRegister(REG_IRQ_FLAGS, irqFlags);

    if((irqFlags & IRQ_CAD_DETECTED) != 0){
        _onCad();
    }

    if ((irqFlags & IRQ_PAYLOAD_CRC_ERROR_MASK) == 0) {
        // received a packet
        _packetIndex = 0;
        // read packet length. 0 if no packet received.
        int packetLength = _implicitHeaderMode ? readRegister(REG_PAYLOAD_LENGTH) : readRegister(REG_RX_NB_BYTES);

        if (_onReceive) {
            _onReceive(packetLength);
        }
    }
}

/* Reads the indicated internal register.
 Input parameters: address of register
 Output parameters: returns value stored in given address
*/
uint8_t LoRaClass::readRegister(uint8_t address) {
    return singleTransfer(address & 0x7f, 0x00);
}

/* Writes to indicated internal register
 Input parameters: address of register, value written on register
*/ 
void LoRaClass::writeRegister(uint8_t address, uint8_t value) {
    singleTransfer(address | 0x80, value);
}

/* Uses SPI transmission to send a single byte, simultaneously sends and receives the register address and the data that will be placed there. Returns the received data.
 * Input parameters: address of register, the byte to send out over the bus
*/
uint8_t LoRaClass::singleTransfer(uint8_t address, uint8_t value) {
    uint8_t response;
    digitalWrite(_ss, LOW);
    SPI.beginTransaction(_spiSettings);
    SPI.transfer(address);
    response = SPI.transfer(value);
    SPI.endTransaction();
    digitalWrite(_ss, HIGH);
    return response;
}

// Indicates RX and TX done 
void LoRaClass::onDio0Rise() {
    LoRa.DIO_RISE = true;
}

LoRaClass LoRa;
