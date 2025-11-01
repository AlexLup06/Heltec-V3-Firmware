// Enables serial debug outputs to the Lora network
#define DEBUG_LORA_SERIAL

// LoRa parameters of the node
#define LORA_OUTPUT_POWER 20
#define LORA_FREQUENCY 869.4
#define LORA_BANDWIDTH 250.0
#define LORA_SPREADINGFACTOR 7
#define LORA_PREAMBLE_LENGTH 12
#define LORA_TRANSMISSION_POWER 20
#define LORA_CR 1

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

// simulation runs configs
#define SIMULATION_DURATION_SEC 10ul
#define MAC_PROTOCOL_SWITCH_DELAY_SEC 5ul
#define START_DELAY_SEC 5
#define NUMBER_OF_RUNS 5