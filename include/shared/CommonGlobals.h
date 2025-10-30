#pragma once
#include <Arduino.h>
#include <RadioLib.h>
#include <LoRaDisplay.h>
#include <MeshRouter.h>
#include <Aloha.h>
#include <CadAloha.h>
#include <Csma.h>
#include <MacController.h>
#include <LoggerManager.h>
#include <MessageSimulator.h>
#include <ClickHandler.h>
#include <Configurator.h>

// --- Shared global objects ---
extern LoRaDisplay loraDisplay;
extern SX1262Public radio;
extern LoggerManager loggerManager;
extern MessageSimulator messageSimulator;
extern ClickHandler button;
extern Configurator configurator;

extern int currentMac;
extern MacContext macCtx;
extern MacBase *macProtocol;
extern MeshRouter meshRouter;
extern Aloha aloha;
extern CadAloha cadAloha;
extern Csma csma;

// --- Shared callbacks ---
void onDio1IR();
void onMacChanged(MacProtocol newMac);
void onMacFinished(MacProtocol finished);
void incrementCb();
