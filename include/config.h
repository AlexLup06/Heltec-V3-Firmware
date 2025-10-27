//
// Configuration of the Lora firmware
// Konfiguration der Lora Firmware
#ifndef ESP32_LORA_TEST_CONFIG_H
#define ESP32_LORA_TEST_CONFIG_H
#endif //ESP32_LORA_TEST_CONFIG_H


// Enables serial debug outputs to the Lora network
// Aktiviert Serielle Debug Ausgaben zum Lora-Netzwerk
#define DEBUG_LORA_SERIAL

// OLED screen refresh rate
// Aktualisierungsrate des OLED Bildschirms
#define OLED_SCREEN_UPDATE_INTERVAL_MS 100

// LoRa parameters of the node
// LoRa Parameter des Knotens
#define LORA_OUTPUT_POWER 20 // Allows transmission power of 20dBm and duty cycle of 10%
#define LORA_FREQUENCY 869400000 // Allows transmission power of 20dBm and duty cycle of 10%
#define LORA_BANDWIDTH 250E3     // Bandwidth
#define LORA_SPREADINGFACTOR 7   // Spreading factor
#define LORA_PREAMBLE_LENGTH 12  // Length of Preamble, Default 12
#define LORA_TRANSMISSION_POWER 20	// Allows transmission power of 20dBm
#define LORA_CR 1	// Allows transmission power of 20dBm


// DutyCycle. Currently not used. 
#define LORA_DUTY_CYCLE = 0.1;

// Sends a NodeAnnouncement every 3 seconds (for range measurement)
// Sendet alle 3 Sekunden ein NodeAnnouncement (zur Reichweitenmessung)
#define ANNOUNCE_IN_INTERVAL

// Enable forwarding of packets to network participants.
// By activating this option, the performance of the network is severely restricted, as each node resends the packet.
// Aktiviere das Weiterleiten von Paketen an Netzwerk Teilnehmer
// Durch aktivierern dieser Option, wird die Leistung des Netzwerks Massiv eingeschränkt, da jeder Knoten das Paket erneut versendet.
#undef RETRANSMIT_PACKETS