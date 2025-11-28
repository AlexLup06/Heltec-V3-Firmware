#include "shared/CommonGlobals.h"

void incrementCb()
{
    loraDisplay.incrementSent();
}

void onMacChanged(MacProtocol newMac)
{
    Serial.printf("Switched MAC protocol to %s\n", macController.macIdToString(newMac));
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
        macProtocol = &rsmitranr;
        macController.setMac(macProtocol);
        break;
    case MacProtocol::RS_MITRANR:
        macController.increaseRunCount();
        Serial.printf("\n\nFinished %d Runs!\n\n\n", macController.getRunCount());
        macProtocol = &aloha;
        macController.setMac(macProtocol);

        if (macController.finishedAllRuns())
        {
            delay(20000);
            macController.collectData();
            delay(3000);

            Serial.println("\n\nALL RUNS COMPLETED!\n\n");
            loraDisplay.renderFinish();
            return;
        }
        break;
    default:
        break;
    }
}
