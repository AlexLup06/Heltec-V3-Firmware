#pragma once
#include <Arduino.h>
#include <RadioLib.h>
#include <LoRaDisplay.h>
#include <MacController.h>
#include <LoggerManager.h>
#include <MessageSimulator.h>
#include <ClickHandler.h>
#include <Configurator.h>
#include <MeshRouter.h>
#include <Aloha.h>
#include <CadAloha.h>
#include <Csma.h>
#include <RSMiTra.h>
#include <IRSMiTra.h>
#include <MiRS.h>

// --- Shared global objects ---
extern LoRaDisplay loraDisplay;
extern SX1262Public radio;
extern LoggerManager loggerManager;
extern MessageSimulator messageSimulator;
extern ClickHandler button;
extern Configurator configurator;
extern MacController macController;

extern int currentMac;
extern uint8_t nodeId;
extern MacContext macCtx;
extern MacBase *macProtocol;
extern Aloha aloha;
extern CadAloha cadAloha;
extern Csma csma;
extern MeshRouter meshRouter;
extern RSMiTra rsmitra;
extern IRSMiTra irsmitra;
extern MiRS mirs;

// --- Shared callbacks ---
void onDio1IR();
void onMacChanged(MacProtocol newMac);
void onMacFinished(MacProtocol finished);
void incrementCb();
