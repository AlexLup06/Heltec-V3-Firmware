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

// LoRa Parameter des Knotens
#define LORA_FREQUENCY 869400000 // Erlaubt Sendeleitung von 20dBm und Duty Cycle von 10%
#define LORA_BANDWIDTH 250E3     // Bandbreite
#define LORA_SPREADINGFACTOR 7   // Spreizfaktor
#define LORA_PREAMBLE_LENGTH 12  // Länge der Preamble, Default 12
#define LORA_TRANSMISSION_POWER 20//Erlaubt Sendeleitung von 20dBm

// DutyCycle. Wird aktuell nicht verwendet.
#define LORA_DUTY_CYCLE = 0.1;

// Lasttest Modus. Wenn dieser aktiviert ist, werden unendlich viele Pakete generiert und der Durchsatz gemessen.
#undef TEST_MODE

// Sendet alle 3 Sekunden ein NodeAnnouncement (zur Reichweitenmessung)
#define ANNOUNCE_IN_INTERVAL

// Aktiviere das Weiterleiten von Paketen an Netzwerk Teilnehmer
// Durch aktivierern dieser Option, wird die Leistung des Netzwerks Massiv eingeschränkt, da jeder Knoten das Paket erneut versendet.
#undef RETRANSMIT_PAKETS