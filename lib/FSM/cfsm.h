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

class cFSM {
private:
    int state = 0;
    const char *stateName = "INIT";
    std::string name;

public:
    explicit cFSM(const char *n = "fsm") : state(0), stateName("INIT"), name(n ? n : "") {}

    void setState(int s, const char *n = nullptr) {
        state = s;
        stateName = n ? n : "";
    }

    int getState() const { return state; }
    const char *getStateName() const { return stateName; }
    const char *getName() const { return name.c_str(); }
    bool isInTransientState() const { return state < 0; }
};


#define FSMA_SetLogging(enabled) \
    ___logging = enabled;

#define FSMA_State(s) if (___transition_seen = false, ___exit = true, ___fsm.getState() == (s))

#define FSMA_Event_Transition(transition, condition, target, action) \
    ___transition_seen = true;                                       \
    if ((condition) && ___is_event)                                  \
    {                                                                \
        ___is_event = false;                                         \
        FSMA_Transition(transition, (condition), target, action)

#define FSMA_No_Event_Transition(transition, condition, target, action) \
    ___transition_seen = true;                                          \
    if ((condition) && !___is_event)                                    \
    {                                                                   \
        FSMA_Transition(transition, (condition), target, action)

#define FSMA_Transition(transition, condition, target, action) \
    action;                                                    \
    ___fsm.setState(target, #target);                          \
    ___exit = false;                                           \
    continue;                                                  \
    }

#define FSMA_Ignore_Event(condition) \
    ___transition_seen = true;       \
    if ((condition) && ___is_event)  \
    {                                \
        ___is_event = false;         \
    }
