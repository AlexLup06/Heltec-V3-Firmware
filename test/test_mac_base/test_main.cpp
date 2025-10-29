#include <Arduino.h>
#include <unity.h>

extern void test_calculate_time_on_air();

// -----------------------------------------------------------------------------
// UNITY ENTRY POINT (cross-platform)
// -----------------------------------------------------------------------------

void setup() {
    delay(2000); // give serial time to initialize
    UNITY_BEGIN();

    RUN_TEST(test_calculate_time_on_air);

    UNITY_END();
}

void loop() {}