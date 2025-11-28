#include "definitions.h"

// Enables serial debug outputs to the Lora network
#undef DEBUG_LORA_SERIAL

// LoRa parameters of the node
#define LORA_OUTPUT_POWER 20
#define LORA_FREQUENCY 869.4
#define LORA_BANDWIDTH 250.0
#define LORA_SPREADINGFACTOR 7
#define LORA_PREAMBLE_LENGTH 8
#define LORA_TRANSMISSION_POWER 20
#define LORA_CR 1
#define LORA_SYNCWORD 0x84
#define LORA_CRC 2

// LoRa radio Pins
#define LORA_CS 8
#define LORA_RST 12
#define LORA_DIO1 14
#define LORA_BUSY 13

// OLED Display configs
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
#define SCREEN_ADDRESS 0x3C
#define PAGE_INTERVAL_MS 5000
#define RENDER_INTERVAL_MS 1000

// simulation runs configs
#define SIMULATION_DURATION_SEC 300ul
#define MAC_PROTOCOL_SWITCH_DELAY_SEC 60ul
#define START_DELAY_SEC 60
#define NUMBER_OF_RUNS 6