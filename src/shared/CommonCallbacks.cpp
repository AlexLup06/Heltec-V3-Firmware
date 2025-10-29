#include "shared/CommonGlobals.h"

void onDio1IR()
{
    if (configurator.isInConfigMode())
    {
        configurator.receiveDio1Interrupt();
    }
    else
    {
        macProtocol->receiveDio1Interrupt();
    }
}

void onMacChanged(MacProtocol newMac)
{
    DEBUG_PRINTF("Switched MAC protocol to %d\n", newMac);
}

void onMacFinished(MacProtocol finished)
{
    DEBUG_PRINTF("Finished MAC %d — cooling down for 2 minutes\n", finished);
    macProtocol->finish();

    switch (finished)
    {
    case MacProtocol::MESH_ROUTER:
        // macProtocol = &cadAloha;
        break;
    case MacProtocol::CAD_ALOHA:
        macProtocol = &aloha;
        break;
    case MacProtocol::ALOHA:
        // macProtocol = &csma;
        break;
    case MacProtocol::CSMA:
        // macProtocol = &rsimitra;
        break;
    case MacProtocol::RS_IMITRA:
        // macProtocol = &mirs;
        break;
    case MacProtocol::MIRS:
        // topology complete
        break;
    default:
        break;
    }
}
