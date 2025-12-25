# RobotNetwork Firmware (student write-up)

This repo contains the firmware I used for my Master thesis project on LoRa MAC protocols. It runs on Heltec WiFi LoRa 32 V3 boards with SX1262 transceiver and lets me compare different MACs on real hardware. I also log the results locally with LittleFS so I can get them after a simulation run.

Some code comes from other projects (LGPL v3.0+ and OMNeT++ Public License). See the License Notes section at the end.

## What this firmware does
- Builds two roles from the same code: **master** (has extended button actions + time sync) and **node** (regular device).
- Runs multiple MAC protocols one after another.
- Generates traffic automatically (so it can run without external input).
- Shows status on the OLED and logs data to LittleFS.

## Hardware used
- Heltec WiFi LoRa 32 V3 (ESP32 + SX1262)
- Antenna connected (important for LoRa)
- USB cable for flashing

## Software needed
- PlatformIO (CLI or VS Code extension)
- Python 3.x
- USB drivers for the Heltec board

## How to build and flash
1. Install PlatformIO.
2. Clone this repo.
3. Plug in your Heltec board.
4. Build:
   ```bash
   # master firmware
   pio run -e master-heltec_wifi_lora_32_V3

   # node firmware
   pio run -e node-heltec_wifi_lora_32_V3
   ```
5. Upload and open serial:
   ```bash
   pio run -e master-heltec_wifi_lora_32_V3 -t upload
   pio device monitor -b 115200
   ```

## How to operate (basic workflow)
This is the part I actually use when running experiments.

1. **Flash one board as master**, and the rest as nodes.
2. **Power all nodes** (battery or USB).
3. **Power the master** and use the **button** to choose the network setup:
   - Single click: next topology
   - Double click: previous topology (master only)
   - Triple click: confirm setup / start time. This starts propagation of the network configuration and start time for the simulations
   - Quadruple click: dump LittleFS logs to serial
   - Long click: wipe LittleFS logs
4. After confirming, the master sends configuration packets.
5. All nodes run through the MAC protocols automatically, for the duration defined in `include/config.h`.
6. Logs are saved on each node (LittleFS). You can dump them later with the button or by using the scripts.

## Configuration
- `include/config.h`:
  - LoRa parameters (SF/BW/CR, pins)
  - Simulation length (`SIMULATION_DURATION_SEC`)
  - Number of runs (`NUMBER_OF_RUNS`)
  - Start delays and protocol switch delay
- `src/master_main.cpp`, `src/node_main.cpp`: entry points
- `lib/`: MAC implementations and helpers

## Folder overview
| Path | What it is |
| --- | --- |
| `src/` | Main application (master + node) |
| `src/shared/` | Shared setup and FreeRTOS tasks |
| `include/` | Global headers + config |
| `lib/` | MAC protocols and modules |
| `scripts/` | Log extraction helpers and data merging and aggregation|
| `platformio.ini` | PlatformIO environments |

## License Notes
Parts of this project use code under GNU LGPL v3.0+ and OMNeT++ Public License (OMNeT++ simulation framework, © 1992–2017 Andras Varga and OpenSim Ltd.). Check upstream licenses if you reuse this firmware.
