#ifdef ARDUINO
#include <Arduino.h>
#endif

#include <unity.h>
#include "test_TOA.cpp"

// -----------------------------------------------------------------------------
// UNITY ENTRY POINT (cross-platform)
// -----------------------------------------------------------------------------

#ifdef ARDUINO
void setup() {
    delay(2000); // give serial time to initialize
    UNITY_BEGIN();

    RUN_TEST(test_calculate_time_on_air);

    UNITY_END();
}

void loop() {}
#else
int main(int argc, char **argv) {
    UNITY_BEGIN();

    RUN_TEST(test_calculate_time_on_air);

    UNITY_END();
    return 0;
}
#endif