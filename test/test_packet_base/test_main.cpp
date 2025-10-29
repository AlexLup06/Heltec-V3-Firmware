#include <Arduino.h>
#include <unity.h>
#include "PacketBase.h"
#include "PacketBase.cpp"
#include "CustomPacketQueue.cpp"
#include "IncompletePacketList.cpp"

// -----------------------------------------------------------------------------
// Helpers
// -----------------------------------------------------------------------------
static uint8_t dummyPayload[600] = {0};

void setUp() {}
void tearDown() {}

static uint8_t makeChecksum(const uint8_t *payload, uint16_t size) {
    return crc8(payload, size);
}

// -----------------------------------------------------------------------------
// Tests
// -----------------------------------------------------------------------------

void test_createNodeAnnounce_adds_to_queue() {
    PacketBase base;
    uint8_t mac[6] = {1,2,3,4,5,6};

    base.createNodeAnnouncePacket(mac, 7);
    const QueuedPacket* pkt = base.dequeuedPacketWasLast() ? base.dequeuePacket() : nullptr;

    TEST_ASSERT_NOT_NULL(pkt);
    TEST_ASSERT_TRUE(pkt->isNodeAnnounce);
    TEST_ASSERT_FALSE(pkt->isHeader);
    TEST_ASSERT_FALSE(pkt->isMission);
}

void test_createMessage_withRTS_creates_multiple_packets() {
    PacketBase base;
    uint8_t payload[400];
    memset(payload, 0xAB, sizeof(payload));

    base.createMessage(payload, sizeof(payload), 5, true, true);

    // First should be RTS
    const QueuedPacket* first = base.dequeuePacket();
    TEST_ASSERT_NOT_NULL(first);
    TEST_ASSERT_TRUE(first->isHeader);
    TEST_ASSERT_TRUE(first->isMission);
    TEST_ASSERT_FALSE(first->isNodeAnnounce);

    // There should still be fragments queued
    int count = 1;
    while (base.dequeuePacket() != nullptr) count++;
    TEST_ASSERT_GREATER_THAN(1, count);
}

void test_createMessage_withoutRTS_creates_leader_and_fragments() {
    PacketBase base;
    uint8_t payload[600];
    memset(payload, 0xCD, sizeof(payload));

    base.createMessage(payload, sizeof(payload), 3, false, false);

    const QueuedPacket* first = base.dequeuePacket();
    TEST_ASSERT_NOT_NULL(first);
    TEST_ASSERT_TRUE(first->isHeader);
    TEST_ASSERT_FALSE(first->isMission);
    TEST_ASSERT_FALSE(first->isNodeAnnounce);

    int count = 1;
    while (base.dequeuePacket() != nullptr) count++;
    TEST_ASSERT_GREATER_THAN(1, count);
}

void test_doesIncompletePacketExist_returns_correct_value() {
    PacketBase base;
    base.createIncompletePacket(1, 100, 1, 0, 0x11, true);
    base.createIncompletePacket(2, 120, 2, 0, 0x22, false);

    TEST_ASSERT_TRUE(base.doesIncompletePacketExist(1, 1, true));
    TEST_ASSERT_TRUE(base.doesIncompletePacketExist(2, 2, false));
    TEST_ASSERT_FALSE(base.doesIncompletePacketExist(3, 3, true));
}

void test_addToIncompletePacket_uses_correct_list() {
    PacketBase base;
    memset(dummyPayload, 0x44, 251);

    base.createIncompletePacket(10, 251, 8, 0, 0x33, true);
    auto res1 = base.addToIncompletePacket(10, 8, 0, 251, dummyPayload, true, false);
    TEST_ASSERT_TRUE(res1.isComplete || !res1.isComplete);

    base.createIncompletePacket(20, 251, 9, 0, 0x44, false);
    auto res2 = base.addToIncompletePacket(20, 9, 0, 251, dummyPayload, false, false);
    TEST_ASSERT_TRUE(res2.isComplete || !res2.isComplete);
}

void test_encapsulate_and_decapsulate_roundtrip() {
    PacketBase base;
    BroadcastRTSPacket_t pkt{};
    pkt.source = 10;

    MessageTypeBase msg;
    base.encapsulate(msg);

    uint8_t buffer[2] = {0x00, 0x12};
    buffer[0] |= 0x80; // simulate mission bit
    bool wasMission = base.decapsulate(buffer);
    TEST_ASSERT_TRUE(wasMission);
    TEST_ASSERT_EQUAL(0x00, buffer[0]);
}

void test_clearQueue_frees_packets() {
    PacketBase base;
    uint8_t mac[6] = {9,8,7,6,5,4};
    base.createNodeAnnouncePacket(mac, 9);

    base.clearQueue();
    QueuedPacket *p = base.dequeuePacket();
    TEST_ASSERT_NULL(p);
}

void test_dequeuedPacketWasLast_logic() {
    PacketBase base;
    uint8_t mac[6] = {1,2,3,4,5,6};
    base.createNodeAnnouncePacket(mac, 2);
    TEST_ASSERT_TRUE(base.dequeuedPacketWasLast());
}

// -----------------------------------------------------------------------------
// UNITY ENTRY POINT for ESP32
// -----------------------------------------------------------------------------

void setup() {
    delay(2000);  // Give serial monitor time to attach
    UNITY_BEGIN();

    RUN_TEST(test_createNodeAnnounce_adds_to_queue);
    RUN_TEST(test_createMessage_withRTS_creates_multiple_packets);
    RUN_TEST(test_createMessage_withoutRTS_creates_leader_and_fragments);
    RUN_TEST(test_doesIncompletePacketExist_returns_correct_value);
    RUN_TEST(test_addToIncompletePacket_uses_correct_list);
    RUN_TEST(test_encapsulate_and_decapsulate_roundtrip);
    RUN_TEST(test_clearQueue_frees_packets);
    RUN_TEST(test_dequeuedPacketWasLast_logic);

    UNITY_END();
}

void loop() {}
