#ifdef ARDUINO
#include <Arduino.h>
#else
#define delay(ms) ((void)0)
#endif

#include <unity.h>
#include "IncompletePacketList.h"
#include "IncompletePacketList.cpp"

// -----------------------------------------------------------------------------
// Helpers
// -----------------------------------------------------------------------------
static uint8_t dummyPayload[300] = {0};

void setUp() {}
void tearDown() {}

static uint8_t makeChecksum(uint16_t id, uint16_t size, uint8_t src) {
    return (id + size + src) & 0xFF;
}

// -----------------------------------------------------------------------------
// Tests
// -----------------------------------------------------------------------------

void test_create_and_get_packet() {
    IncompletePacketList list;

    list.createIncompletePacket(1, 300, 7, 0, makeChecksum(1, 300, 7));
    auto *pkt = list.getPacketBySource(7);
    TEST_ASSERT_NOT_NULL(pkt);
    TEST_ASSERT_EQUAL(1, pkt->id);
    TEST_ASSERT_EQUAL(300, pkt->packetSize);
    TEST_ASSERT_FALSE(pkt->corrupted);
}

void test_remove_packet_by_source() {
    IncompletePacketList list;
    list.createIncompletePacket(1, 100, 1, 0, 0xAA);
    list.createIncompletePacket(2, 100, 2, 0, 0xBB);
    list.createIncompletePacket(3, 100, 3, 0, 0xCC);

    list.removePacketBySource(2);
    auto *p1 = list.getPacketBySource(1);
    auto *p2 = list.getPacketBySource(2);
    auto *p3 = list.getPacketBySource(3);

    TEST_ASSERT_NOT_NULL(p1);
    TEST_ASSERT_NULL(p2);
    TEST_ASSERT_NOT_NULL(p3);
}

void test_create_replaces_existing() {
    IncompletePacketList list;
    list.createIncompletePacket(1, 100, 1, 0, 0xAA);
    list.createIncompletePacket(2, 120, 1, 0, 0xBB);
    auto *pkt = list.getPacketBySource(1);
    TEST_ASSERT_EQUAL(2, pkt->id);
}

void test_calc_offset_nonleader() {
    IncompletePacketList list;
    FragmentedPacket_t pkt{};
    pkt.withLeaderFrag = false;

    int off0 = list.calcOffset(&pkt, 0);
    int off1 = list.calcOffset(&pkt, 1);
    int off2 = list.calcOffset(&pkt, 2);

    TEST_ASSERT_EQUAL(0, off0);
    TEST_ASSERT_EQUAL(LORA_MAX_FRAGMENT_PAYLOAD, off1);
    TEST_ASSERT_EQUAL(2 * LORA_MAX_FRAGMENT_PAYLOAD, off2);
}

void test_calc_offset_leader() {
    IncompletePacketList list;
    FragmentedPacket_t pkt{};
    pkt.withLeaderFrag = true;

    int off0 = list.calcOffset(&pkt, 0);
    int off1 = list.calcOffset(&pkt, 1);
    int off2 = list.calcOffset(&pkt, 2);

    TEST_ASSERT_EQUAL(0, off0);
    TEST_ASSERT_EQUAL(LORA_MAX_FRAGMENT_LEADER_PAYLOAD, off1);
    TEST_ASSERT_EQUAL(LORA_MAX_FRAGMENT_LEADER_PAYLOAD + LORA_MAX_FRAGMENT_PAYLOAD, off2);
}

void test_is_corrupted_rules() {
    IncompletePacketList list;
    FragmentedPacket_t pkt{};

    // single fragment - must match
    pkt.numOfFragments = 1;
    pkt.packetSize = 100;
    TEST_ASSERT_TRUE(list.isCorrupted(&pkt, 0, 50));  // mismatch
    TEST_ASSERT_FALSE(list.isCorrupted(&pkt, 0, 100)); // correct

    // leader fragment
    pkt.numOfFragments = 2;
    pkt.withLeaderFrag = true;
    pkt.packetSize = 247 + 80; // 327 bytes total
    TEST_ASSERT_TRUE(list.isCorrupted(&pkt, 0, 200)); // leader frag wrong
    TEST_ASSERT_FALSE(list.isCorrupted(&pkt, 0, 247)); // leader frag ok
}

void test_add_to_incomplete_completes() {
    IncompletePacketList list;
    list.createIncompletePacket(1, 251, 10, 0, 0x11);

    auto *pkt = list.getPacketBySource(10);
    TEST_ASSERT_NOT_NULL(pkt);
    memset(dummyPayload, 0x55, 251);

    auto res = list.addToIncompletePacket(1, 10, 0, 251, dummyPayload);
    TEST_ASSERT_TRUE(res.isComplete);
    TEST_ASSERT_TRUE(res.sendUp);
    TEST_ASSERT_EQUAL(pkt, res.completePacket);
}

void test_add_duplicate_fragment_ignored() {
    IncompletePacketList list;
    list.createIncompletePacket(1, 500, 9, 0, 0x22);
    auto *pkt = list.getPacketBySource(9);
    memset(dummyPayload, 0x66, 251);

    // first insert
    list.addToIncompletePacket(1, 9, 0, 251, dummyPayload);
    TEST_ASSERT_EQUAL(251, pkt->received);

    // duplicate same fragment should not add more
    list.addToIncompletePacket(1, 9, 0, 251, dummyPayload);
    TEST_ASSERT_EQUAL(251, pkt->received); // unchanged
}

void test_update_packet_id_comparisons() {
    IncompletePacketList list;
    list.updatePacketId(1, 10);
    TEST_ASSERT_TRUE(list.isNewIdHigher(1, 11));
    TEST_ASSERT_TRUE(list.isNewIdSame(1, 10));
    TEST_ASSERT_TRUE(list.isNewIdLower(1, 9));
}

// -----------------------------------------------------------------------------
// UNITY ENTRY POINT (cross-platform)
// -----------------------------------------------------------------------------

#ifdef ARDUINO
void setup() {
    delay(2000); // give serial time to initialize
    UNITY_BEGIN();

    RUN_TEST(test_create_and_get_packet);
    RUN_TEST(test_remove_packet_by_source);
    RUN_TEST(test_create_replaces_existing);
    RUN_TEST(test_calc_offset_nonleader);
    RUN_TEST(test_calc_offset_leader);
    RUN_TEST(test_is_corrupted_rules);
    RUN_TEST(test_add_to_incomplete_completes);
    RUN_TEST(test_add_duplicate_fragment_ignored);
    RUN_TEST(test_update_packet_id_comparisons);

    UNITY_END();
}

void loop() {}
#else
int main(int argc, char **argv) {
    UNITY_BEGIN();

    RUN_TEST(test_create_and_get_packet);
    RUN_TEST(test_remove_packet_by_source);
    RUN_TEST(test_create_replaces_existing);
    RUN_TEST(test_calc_offset_nonleader);
    RUN_TEST(test_calc_offset_leader);
    RUN_TEST(test_is_corrupted_rules);
    RUN_TEST(test_add_to_incomplete_completes);
    RUN_TEST(test_add_duplicate_fragment_ignored);
    RUN_TEST(test_update_packet_id_comparisons);

    UNITY_END();
    return 0;
}
#endif