//
// Konfiguration der Lora Firmware
//


#ifndef ESP32_LORA_TEST_CONFIG_H
#define ESP32_LORA_TEST_CONFIG_H
#endif //ESP32_LORA_TEST_CONFIG_H


// Aktiviert Serielle Debug Ausgaben zum Lora-Netzwerk
#undef DEBUG_LORA_SERIAL
#define USE_OTA_UPDATE_CHECKING

// Aktualisierungsrate des OLED Bildschirms
#define OLED_SCREEN_UPDATE_INTERVAL_MS 10

#define LORA_FREQUENCY 869400000
#define LORA_BANDWIDTH 250E3
#define LORA_SPREADINGFACTOR 7
#define LORA_PREAMBLE_LENGTH 12 // Default Value