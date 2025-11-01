#include <unity.h>

#include <cstdint>
#include <stdlib.h>
#include <cmath>
#include <algorithm> 

#include "config.h"
#include "functions.h"


using std::max;

void test_calculate_time_on_air(){
    TEST_ASSERT_EQUAL(20096,getToAByPacketSizeInUS(8));
    TEST_ASSERT_EQUAL(22656,getToAByPacketSizeInUS(9));
    TEST_ASSERT_EQUAL(22656,getToAByPacketSizeInUS(12));
    TEST_ASSERT_EQUAL(25216,getToAByPacketSizeInUS(13));
}