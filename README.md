# RobotNetwork

This Project is an implementation of a simple ad hoc Network, based on LoRa Modules.
Target Platform is the [WiFi-LoRa-32 Board by Heltec](https://heltec.org/project/wifi-lora-32/), but it should be possible to use any alternative board, with an ESP32 and a LoRa-Modem of the SX1276/SX1277/SX1278 Series.

This ad hoc Network was developed, to research the possibility of using LoRa as Communication Technique between a Swarm of Robots.
As the Data is Transmitted in Broadcasts, there was no need for Unicast Packets, which is why there is no active Routing.

# Unrouted ad hoc Network

The Network does not have Routing. By Default, every Data Packet is Repeated by every Receiver once, flooding the Data through the Network.
Packet Repeating can be turned of, by Setting
`#define RETRANSMIT_PAKETS` to `#undef RETRANSMIT_PAKETS` (in config.h)

The Modem can only transmit 255 Byte large LoRa Packets, so larger ones have to be Fragmented.
A Transmission Starts, by sending an Anouncement Paket, which contains a Locally generated Message ID, Size, Source and Checksum.
The Fragmented Packets follow, including the current Fragment ID and the Message ID. The Receiver can then reconstruct the Packet by that data.

The Messages are unreliable. There is currently no Retransmition of Lost Fragment Packets. Losing one Fragment, will result in a complete loss of the Message.

# Getting Started

## Editor and Framework

The Development was done with the Visual Studio Code Editor, which can be freely downloaded from the Microsoft Website.
For the Microcontroller Support, you have to add the `PlatformIO` Plugin, from the Extensions Tab, after launching the Editor.

The Installation should start in the Console. When its done, Restart your IDE.

The PlatformIO Platform, implements the Arduino Framework and offers an extensive Support for Microcontrollers and external Librarys.
It is Documented very well and offers a beginner friendly way to start with Microcontroller Development.
The Official Documention can be found [Here](https://docs.platformio.org/en/latest).

This Project uses the following external Libraries:

adafruit/Adafruit SSD1306 -> Controlling of the Module's OLED Display
adafruit/Adafruit BusIO -> Helper Library for I2C & SPI transactions, used for LoRa Modem Communication
espressif/arduino-esp32 -> ESP32 Development Framework for Arduino
framework-arduinoespressif32 -> Framework with lots of Helper Libraries, for use with the ESP32
ivanseidel/LinkedList -> LinkedList, used for the SendQueue
arduino-libraries/ArduinoHttpClient -> HTTP Client (Optional), used for Over-The-Air Updates

These will be downloaded automaticly. You will need an active Internet Connection, when building for the First time.

## Open

To Open the Project, go to File -> Open Folder and select the Folder onto which you cloned the Project.
There should be a large Utility Bar at the Botton of VSCode, displaying Various Buttons. Click the Checkmark Button, to start Building the Project.
PlatformIO should now Download all Requirements and start building the Project.

# Build and Test

## Programming

To Programm a given Module, plug it onto your Computer and determine the COM Port.
Go to the platformio.ini File and adjust the Line `upload_port = COM10` according to your Port.
Press the Upload Button, to start building and uploading the Firmware to the Heltec Board.

## Over-The-Air Updates

It is also possible, to Programm multiple Boards at once, using the ESP32's build in WiFi Function.
This Behaviour can be turned on, by Setting `#define USE_OTA_UPDATE_CHECKING` in the config.h file.

This will Start a FreeROS Task, on the 2. Core of the ESP32 CPU. The Update Logic is further found in the ota/devota.cpp and ota/devota.h Files.
The Task will then Connect to the WiFi Network, specified at ota/devota.cpp, and Download the File `/esp32/revision.txt` from `192.168.0.1:80` over HTTP.
This Files contains a Version as Number. When the Version is Higher then the internal build version, the ESP32 will download `/esp32/firmware.bin` and Flash itself.

I used a Linux Microrouter as Server and Access-Point. The Python Script `upload_to_server.py` is called after a regular PlatformIO Build and will upload the binaries to the Router's Webserver. The Firmware Version is simply the current Linux Time, at the time of the build, see get-revision.py.
If you want to use this Feature, make sure Python is Installed and in the System's Path. Also make sure to Adjust the Server's Adress and possible Authentication.

### Disable

You can completly disable the execution of those Scripts, by removing the Following Lines from the platformio.ini

```
build_flags = !python get-revision.py
extra_scripts = post:upload_to_server.py
```

Make sure to also set a static BUILD_REVISION. Just add `#define BUILD_REVISION 0` in the config.h, or the compiler will throw out errors, for not finding the `BUILD_REVISION` Variable, which would normally be set by the get-revision.py script.

# More Information

For Technical Details, see my Thesis `Entwicklung eines realen mobilen ad hoc Netzwerks auf „LoRa“-Basis für Steuerungsdaten eines dezentralen Roboterschwarms unter Wissensbeschränkungen`.
