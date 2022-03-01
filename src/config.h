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
#define OLED_SCREEN_UPDATE_INTERVAL_MS 100

#define LORA_FREQUENCY 869400000
#define LORA_BANDWIDTH 500E3
#define LORA_SPREADINGFACTOR 7
#define LORA_DUTY_CYCLE = 0.1;
#define LORA_PREAMBLE_LENGTH 12 // Default Value

// Last-Test Modus
#undef TEST_MODE

// Aktiviere das Weiterleiten von Paketen an Netzwerk Teilnehmer
#undef RETRANSMIT_PAKETS