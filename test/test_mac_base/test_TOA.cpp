#include <unity.h>

#include <cstdint>
#include <stdlib.h>
#include <cmath>
#include <algorithm> 

#include "config.h"
#include "functions.h"


using std::max;

void test_calculate_time_on_air(){
    TEST_ASSERT_EQUAL(20096,getSendTimeByPacketSizeInUS(8));
    TEST_ASSERT_EQUAL(22656,getSendTimeByPacketSizeInUS(9));
    TEST_ASSERT_EQUAL(22656,getSendTimeByPacketSizeInUS(12));
    TEST_ASSERT_EQUAL(25216,getSendTimeByPacketSizeInUS(13));
}