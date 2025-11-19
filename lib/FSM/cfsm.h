//
// SimpleFSM.h — standalone finite state machine implementation
// Based on OMNeT++ LGPL code, stripped of dependencies
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//

#pragma once
#include <stdexcept>
#include <string>

#define FSM_MAXT 128

class cFSM
{
private:
    int state = 0;
    const char *stateName = "INIT";
    std::string name;

public:
    explicit cFSM(const char *n = "fsm") : state(0), stateName("INIT"), name(n ? n : "") {}

    void setState(int s, const char *n = nullptr)
    {
        state = s;
        stateName = n ? n : "";
    }

    int getState() const { return state; }
    const char *getStateName() const { return stateName; }
    const char *getName() const { return name.c_str(); }
    bool isInTransientState() const { return state < 0; }
};