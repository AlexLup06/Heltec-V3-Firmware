#pragma once 

#include <config.h>
#include <lib/LoRa.h>
#include "mesh/loranode.h"
#include "mesh/loradisplay.h"
#include "HostHandler.h"
#include <Arduino.h>
#include "mesh/MeshRouter.h"

//getter for global router
MeshRouter* getMeshRouter();
