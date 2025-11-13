#include "shared/CommonGlobals.h"

void incrementCb()
{
    loraDisplay.incrementSent();
}

void onMacChanged(MacProtocol newMac)
{
    if (macController.finishedAllRuns())
    {
        DEBUG_PRINTLN("\n\nALL RUNS COMPLETED!\n\n");
        return;
    }
    DEBUG_PRINTF("Switched MAC protocol to %s\n", macController.macIdToString(newMac));
    macProtocol->initRun();
    messageSimulator.init();
}

void onMacFinished(MacProtocol finished)
{
    macProtocol->finish();
    messageSimulator.finish();

    switch (finished)
    {
    case MacProtocol::ALOHA:
        macProtocol = &cadAloha;
        macController.setMac(macProtocol);
        break;
    case MacProtocol::CAD_ALOHA:
        macProtocol = &csma;
        macController.setMac(macProtocol);
        break;
    case MacProtocol::CSMA:
        macProtocol = &meshRouter;
        macController.setMac(macProtocol);
        break;
    case MacProtocol::MESH_ROUTER:
        macProtocol = &rsmitra;
        macController.setMac(macProtocol);
        break;
    case MacProtocol::RS_MITRA:
        macProtocol = &irsmitra;
        macController.setMac(macProtocol);
        break;
    case MacProtocol::IRS_MITRA:
        macProtocol = &mirs;
        macController.setMac(macProtocol);
        break;
    case MacProtocol::MIRS:
        macController.increaseRunCount();
        DEBUG_PRINTF("\n\nFinished %d Runs!\n\n\n", macController.getRunCount());
        macProtocol = &aloha;
        macController.setMac(macProtocol);
        break;
    default:
        break;
    }
}
