#include "shared/CommonGlobals.h"

LoRaDisplay loraDisplay;
SX1262Public radio = SX1262Public(new Module(LORA_CS, LORA_DIO1, LORA_RST, LORA_BUSY));
LoggerManager loggerManager;
MessageSimulator messageSimulator;
ClickHandler button(0, true);
Configurator configurator;
MacController macController;

uint8_t nodeId;

MacContext macCtx;
MacBase *macProtocol = nullptr;
MeshRouter meshRouter;
Aloha aloha;
Csma csma;
RSMiTra rsmitra;
IRSMiTra irsmitra;
MiRS mirs;
RSMiTraNR rsmitranr;
RSMiTraNAV rsmitranav;
