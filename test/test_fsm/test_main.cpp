//
// Copyright (C) 2025
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//

#include <Arduino.h>
#include <unity.h>
#include <stdexcept>
#include <iostream>
#include "FSMA.h"

// -----------------------------------------------------------------------------
// Globals for test actions and conditions
// -----------------------------------------------------------------------------
static bool didFoo = false;
static bool didBar = false;
static bool didBaz = false;

static bool isFoo = false;
static bool isBar = false;
static bool isBaz = false;

static void doFoo() { didFoo = true; }
static void doBar() { didBar = true; }
static void doBaz() { didBaz = true; }

// -----------------------------------------------------------------------------
// Setup / teardown
// -----------------------------------------------------------------------------
void setUp() {
    didFoo = didBar = didBaz = false;
    isFoo = isBar = isBaz = false;
}

void tearDown() {}

// -----------------------------------------------------------------------------
// Helper FSM runner
// -----------------------------------------------------------------------------
enum State { STATE_X = 0, STATE_Y, STATE_Z };

void runSimpleFSM(cFSM &fsm) {
    FSMA_Switch(fsm)
    {
        FSMA_State(STATE_X)
        {
            FSMA_Event_Transition("XY", isFoo, STATE_Y, doFoo());
            FSMA_No_Event_Transition("XZ", isBaz, STATE_Z, doBaz());
        }
        FSMA_State(STATE_Y)
        {
            FSMA_Event_Transition("YX", isBar, STATE_X, doBar());
        }
        FSMA_State(STATE_Z)
        {
            FSMA_Event_Transition("ZX", isFoo, STATE_X, doFoo());
        }
    }
}

// -----------------------------------------------------------------------------
// TESTS
// -----------------------------------------------------------------------------

void test_initial_state_and_name() {
    cFSM fsm("TestFSM");
    TEST_ASSERT_EQUAL(0, fsm.getState());
    TEST_ASSERT_EQUAL_STRING("INIT", fsm.getStateName());
    TEST_ASSERT_EQUAL_STRING("TestFSM", fsm.getName());
}

void test_event_transition_XY() {
    cFSM fsm("fsmXY");
    fsm.setState(STATE_X, "X");
    isFoo = true;

    runSimpleFSM(fsm);

    TEST_ASSERT_TRUE(didFoo);
    TEST_ASSERT_EQUAL(STATE_Y, fsm.getState());
}

void test_event_transition_YX() {
    cFSM fsm("fsmYX");
    fsm.setState(STATE_Y, "Y");
    isBar = true;

    runSimpleFSM(fsm);

    TEST_ASSERT_TRUE(didBar);
    TEST_ASSERT_EQUAL(STATE_X, fsm.getState());
}

void test_no_event_transition_XZ() {
    cFSM fsm("fsmXZ");
    fsm.setState(STATE_X, "X");
    isBaz = true;
    didBaz = false;

    bool ___is_event = false;
    bool ___exit = false;
    bool ___transition_seen = false;
    int ___c = 0;
    cFSM& ___fsm = fsm;
    bool ___logging = true;

    while (!___exit && (___c++ < FSM_MAXT)) {
        FSMA_State(STATE_X)
        {
            FSMA_No_Event_Transition("XZ", isBaz, STATE_Z, doBaz());
        }
        ___exit = true;
    }

    TEST_ASSERT_TRUE_MESSAGE(didBaz, "No-event transition did not fire");
    TEST_ASSERT_EQUAL(STATE_Z, fsm.getState());
}

void test_ignore_event_does_not_transition() {
    cFSM fsm("fsmIgnore");
    fsm.setState(STATE_X, "X");
    isFoo = true;

    FSMA_Switch(fsm)
    {
        FSMA_State(STATE_X)
        {
            FSMA_Ignore_Event(isFoo);
        }
    }

    TEST_ASSERT_EQUAL(STATE_X, fsm.getState());
    TEST_ASSERT_FALSE(didFoo);
}

void test_fail_on_unhandled_event_throws() {
    cFSM fsm("fsmFail");
    fsm.setState(STATE_X, "X");

    bool threw = false;
    try {
        FSMA_Switch(fsm)
        {
            FSMA_State(STATE_X)
            {
                FSMA_Fail_On_Unhandled_Event();
            }
        }
    } catch (...) {
        threw = true;
    }

    TEST_ASSERT_TRUE(threw);
}

void test_enter_action_executes_once() {
    cFSM fsm("fsmEnter");
    fsm.setState(STATE_X, "X");

    int counter = 0;

    {
        bool ___is_event = false;
        bool ___exit = false;
        bool ___transition_seen = false;
        int ___c = 0;
        cFSM& ___fsm = fsm;
        bool ___logging = true;

        while (!___exit && (___c++ < FSM_MAXT)) {
            FSMA_State(STATE_X)
            {
                FSMA_Enter(counter++);
            }
            ___exit = true;
        }
    }

    TEST_ASSERT_EQUAL(1, counter);
}

void test_infinite_loop_protection() {
    cFSM fsm("fsmLoop");
    fsm.setState(STATE_X, "X");
    bool threw = false;

    try {
        for (int i = 0; i < FSM_MAXT + 2; ++i) {
            FSMA_Switch(fsm)
            {
                FSMA_State(STATE_X)
                {
                    FSMA_Event_Transition("XX", true, STATE_X, ;);
                }
            }
        }
        throw std::runtime_error("FSM_MAXT did not trigger");
    } catch (...) {
        threw = true;
    }

    TEST_ASSERT_TRUE_MESSAGE(threw, "FSM_MAXT protection did not trigger");
}

// -----------------------------------------------------------------------------
// UNITY ENTRY POINT (ESP32 only)
// -----------------------------------------------------------------------------

void setup() {
    delay(2000);  // allow serial to attach
    UNITY_BEGIN();

    RUN_TEST(test_initial_state_and_name);
    RUN_TEST(test_event_transition_XY);
    RUN_TEST(test_event_transition_YX);
    RUN_TEST(test_no_event_transition_XZ);
    RUN_TEST(test_ignore_event_does_not_transition);
    RUN_TEST(test_fail_on_unhandled_event_throws);
    RUN_TEST(test_enter_action_executes_once);
    RUN_TEST(test_infinite_loop_protection);

    UNITY_END();
}

void loop() {}
