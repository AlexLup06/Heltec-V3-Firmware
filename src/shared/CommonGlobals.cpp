#include "shared/CommonGlobals.h"

LoRaDisplay loraDisplay;
SX1262Public radio = SX1262Public(new Module(LORA_CS, LORA_DIO1, LORA_RST, LORA_BUSY));
LoggerManager loggerManager("fullmesh");
MessageSimulator messageSimulator;
ClickHandler button(0, true);
Configurator configurator;

int currentMac = MacProtocol::MESH_ROUTER;
MacContext macCtx;
MacBase *macProtocol = nullptr;
MeshRouter meshRouter;
CadAloha cadAloha;
Aloha aloha;
Csma csma;
