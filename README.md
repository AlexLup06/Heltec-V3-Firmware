# RobotNetwork Firmware

RobotNetwork is a PlatformIO/Arduino firmware for Heltec WiFi LoRa 32 V3 nodes that evaluates multiple MAC protocols on real LoRa hardware. It provides a reproducible way to configure networks in the field, stream synthetic or live traffic, and record mission metrics directly on each node via LittleFS.

Parts of this project use code licensed under the GNU LGPL v3.0 or later. The repository also bundles components from the OMNeT++ simulation framework (© 1992–2017 Andras Varga and OpenSim Ltd.) under the OMNeT++ Public License (https://omnetpp.org/license).

## Highlights
- **Dual firmware roles** – Build either a master (time-sync + UI) or node image from the same codebase (`src/master_main.cpp`, `src/node_main.cpp`).
- **Protocol matrix** – Aloha, CAD-Aloha, CSMA, MeshRouter, RSMiTra, IRS-MiTra, MiRS, and RSMiTraNR share the same `MacBase`/`RtsCtsBase` infrastructure for apples-to-apples comparisons.
- **Deterministic scheduling** – FreeRTOS tasks (`macTask`, `radioIrqTask`, `buttonTask`) isolate MAC FSM work, SX1262 interrupts, and button gestures.
- **Synthetic traffic** – `MessageSimulator` injects mission/neighbour flows with configurable rates so experiments can run headless.
- **On-node UX** – `LoRaDisplay` renders per-node RSSI, send/receive counters, and run metadata for up to 32 peers without a serial console. Buttons trigger configuration steps or log management features (dump/delete LittleFS files).
- **Centralized parameterization** – `include/config.h` captures LoRa modulation settings, GPIO pin mappings, simulation durations, and start delays to keep firmware, tools, and experiments aligned.

## Repository Layout

| Path | Description |
| --- | --- |
| `src/master_main.cpp`, `src/node_main.cpp` | Entry points for master vs regular node builds. |
| `src/shared/` | Common setup/loop logic, shared globals, callbacks, and FreeRTOS tasks. |
| `include/` | Global headers (`config.h`, `definitions.h`) plus shared declarations. |
| `lib/` | Feature modules: MAC implementations, packet queue, incomplete packet tracking, configurator, display, simulators, helpers, etc. |
| `data*` | Example LittleFS payloads / datasets used by the logger. |
| `scripts/` | Helper scripts (log extraction, analysis, etc.). |
| `platformio.ini` | PlatformIO environments for the Heltec board (master vs node). |

## Requirements

### Hardware
- Heltec WiFi LoRa 32 V3 (ESP32 + SX1262) modules for each robot or gateway.
- Soldered antenna and access to the onboard user button.
- USB-to-serial cable for flashing and monitoring.

### Software
- [PlatformIO CLI](https://platformio.org/) (or VS Code + PlatformIO extension).
- Python 3.x (required by PlatformIO).
- USB drivers for the Heltec board on your OS.

## Building & Flashing

1. Install PlatformIO (`pip install platformio` or via VS Code extension).
2. Clone or download this repository.
3. Connect a Heltec WiFi LoRa 32 V3 via USB.
4. Build the desired environment:
   ```bash
   # Master firmware (adds Wi-Fi + configurator buttons)
   pio run -e master-heltec_wifi_lora_32_V3

   # Node firmware (headless node, shares shared/ code)
   pio run -e node-heltec_wifi_lora_32_V3
   ```
5. Upload and open the serial monitor:
   ```bash
   pio run -e master-heltec_wifi_lora_32_V3 -t upload
   pio device monitor -b 115200
   ```

The PlatformIO config already selects LittleFS partitions, C++17 flags, and required libraries (`RadioLib`, `Adafruit_SSD1306`, `Adafruit_BusIO`).

## Configuration & Experiment Control

- **`include/config.h`** – Update LoRa modem parameters, GPIO pins, OLED behavior, simulation length (`SIMULATION_DURATION_SEC`), inter-protocol wait (`MAC_PROTOCOL_SWITCH_DELAY_SEC`), start delays, and `NUMBER_OF_RUNS`.
- **Configurator (`lib/Configurator`)** – Handles configuration mode packets, network topology selection, start-time negotiation, and provides button gestures:
  - Single click: scroll forward through topologies.
  - Double click: go backward (master firmware).
  - Triple click: confirm setup/start time.
  - Quadruple click: dump LittleFS logs over serial.
  - Long click: delete all cached binaries/logs.
- **MAC Controller** – Located in `lib/MacController`, automatically cycles through all enabled protocols, enforces per-run timing, and notifies when experiments complete.

## Runtime Architecture

- **FreeRTOS tasks**: 
  - `macTask`: runs MAC FSMs, drives `MessageSimulator`, dispatches upper packets.
  - `radioIrqTask`: drains SX1262 DIO1 interrupts and forwards frames to `MacBase`.
  - `buttonTask`: debounces and interprets complex button gestures.
- **`MacBase` / `RtsCtsBase`**: Provide shared implementations for packet queuing, fragmentation, radio access, self-message scheduling, and RTS/CTS timing.
- **`MessageSimulator`**: Generates mission vs neighbour messages at configurable intervals to ensure steady traffic even without external payloads.
- **`LoRaDisplay`**: Maintains per-node RSSI, sent/received counters, run ID, network ID, and node count, paging automatically across 4 rows at a time.
- **LittleFS logging**: Mounted during `commonSetup()`. Logs can be dumped via button gesture or over serial/SPIFFS tools.

## Data & Logging

- All nodes mount LittleFS at boot; if mounting fails, an error prints on serial.
- Button gestures can dump logs or wipe the filesystem without reflashing.
- Scripts in `scripts/` (if present) help collect `.bin` logs via serial.

## Extending the Firmware

1. **Add a MAC**: Derive from `MacBase` or `RtsCtsBase`, override `handleWithFSM`, `handleUpperPacket`, `handleProtocolPacket`, and register it in `src/shared/CommonSetup.cpp` plus the switch statement in `CommonCallbacks.cpp`.
2. **Adjust radio behavior**: Modify `RadioBase` or LoRa parameters in `config.h`.
3. **Custom telemetry**: Use `LoggerManager` or extend `LoRaDisplay` to surface new metrics.

Remember to preserve memory usage—Heltec V3 devices have limited RAM/flash.

## License Notes

This project reuses code licensed under the GNU LGPL v3.0 (or later). Portions originate from the OMNeT++ simulation framework and remain under the OMNeT++ Public License. Review upstream licenses before redistributing firmware or derived works.
