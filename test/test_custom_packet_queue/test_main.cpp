#ifdef ARDUINO
#include <Arduino.h>
#else
#define delay(ms) ((void)0)
#endif

#include <unity.h>
#include "helpers/CustomPacketQueue.h"
#include "helpers/CustomPacketQueue.cpp"

static QueuedPacket *makePacket(uint8_t size,
                                bool header = false,
                                bool isMission = false,
                                bool nodeAnnounce = false)
{
    auto pkt = new QueuedPacket();
    pkt->data = (uint8_t *)malloc(size);
    pkt->packetSize = size;
    pkt->isHeader = header;
    pkt->isMission = isMission;
    pkt->isNodeAnnounce = nodeAnnounce;
    return pkt;
}

// ---------- TEST CASES ----------

void test_initial_state()
{
    CustomPacketQueue q;
    TEST_ASSERT_TRUE(q.isEmpty());
    TEST_ASSERT_EQUAL(0, q.size());
}

void test_enqueue_and_dequeue()
{
    CustomPacketQueue q;
    auto pkt = makePacket(5);
    TEST_ASSERT_TRUE(q.enqueuePacket(pkt));
    TEST_ASSERT_EQUAL(1, q.size());
    auto out = q.dequeuePacket();
    TEST_ASSERT_EQUAL_PTR(pkt, out);
    free(out->data);
    delete out;
}

void test_remove_at_position()
{
    CustomPacketQueue q;
    auto p1 = makePacket(1);
    auto p2 = makePacket(2);
    q.enqueuePacket(p1);
    q.enqueuePacket(p2);
    q.removePacketAtPosition(0);
    TEST_ASSERT_EQUAL(1, q.size());
    q.removePacketAtPosition(0);
    TEST_ASSERT_TRUE(q.isEmpty());
}

void test_node_announce_goes_front()
{
    CustomPacketQueue q;
    auto normal = makePacket(1, true, true, false);
    auto announce = makePacket(2, false, false, true);
    q.enqueuePacket(normal);
    q.enqueuePacket(announce);
    auto first = q.dequeuePacket();
    TEST_ASSERT_TRUE(first->isNodeAnnounce);
    free(first->data);
    delete first;
    auto second = q.dequeuePacket();
    free(second->data);
    delete second;
}

void test_node_announce_goes_after_fragment_if_header_gone()
{
    CustomPacketQueue q;
    auto normal = makePacket(1, false, false, false);
    auto announce = makePacket(2, false, false, true);
    q.enqueuePacket(normal);
    q.enqueuePacket(announce);
    auto first = q.dequeuePacket();
    free(first->data);
    delete first;
    auto second = q.dequeuePacket();
    TEST_ASSERT_TRUE(second->isNodeAnnounce);
    free(second->data);
    delete second;
}

void test_only_one_node_announce()
{
    CustomPacketQueue q;
    auto first = makePacket(1, false, false, true);
    auto second = makePacket(2, false, false, true);
    q.enqueuePacket(first);
    q.enqueuePacket(second);
    TEST_ASSERT_EQUAL(1, q.size());
    auto pkt = q.dequeuePacket();
    free(pkt->data);
    delete pkt;
}

void test_new_neighbour_replaces_old_neighbour()
{
    CustomPacketQueue q;
    auto n1 = makePacket(1, true, false, false);
    auto n2 = makePacket(2, false, false, false);
    auto n3 = makePacket(3, false, false, false);
    q.enqueuePacket(n1);
    q.enqueuePacket(n2);
    q.enqueuePacket(n3);
    auto header = makePacket(4, true, false, false);
    auto frag = makePacket(5, false, false, false);
    q.enqueuePacket(header);
    q.enqueuePacket(frag);
    TEST_ASSERT_EQUAL(2, q.size());
    auto out = q.dequeuePacket();
    TEST_ASSERT_TRUE(out->isHeader);
    free(out->data);
    delete out;
}

void test_new_neighbour_does_not_replace_old_neighbour_when_header_gone()
{
    CustomPacketQueue q;
    auto n1 = makePacket(1, false, false, false);
    auto n4 = makePacket(4, true, true, false);
    auto n5 = makePacket(5, false, true, false);
    auto n2 = makePacket(2, true, false, false);
    auto n3 = makePacket(3, false, false, false);
    q.enqueuePacket(n1);
    q.enqueuePacket(n4);
    q.enqueuePacket(n5);
    q.enqueuePacket(n2);
    q.enqueuePacket(n3);
    TEST_ASSERT_EQUAL(5, q.size());
    auto out1 = q.dequeuePacket();
    TEST_ASSERT_TRUE(!out1->isHeader);
    free(out1->data);
    delete out1;

    auto out2 = q.dequeuePacket();
    TEST_ASSERT_TRUE(out2->isHeader && out2->isMission);
    free(out2->data);
    delete out2;
}

void test_enqueue_at_position()
{
    CustomPacketQueue q;
    auto a = makePacket(1);
    auto b = makePacket(2);
    auto c = makePacket(3);
    q.enqueuePacket(a);
    q.enqueuePacket(b);
    q.enqueuePacketAtPosition(c, 1);
    TEST_ASSERT_EQUAL(3, q.size());
    auto first = q.dequeuePacket();
    auto second = q.dequeuePacket();
    TEST_ASSERT_EQUAL(3, second->packetSize);
    free(first->data);
    delete first;
    free(second->data);
    delete second;
    auto last = q.dequeuePacket();
    free(last->data);
    delete last;
}

void test_remove_specific_packet()
{
    CustomPacketQueue q;
    auto p1 = makePacket(1);
    auto p2 = makePacket(2);
    q.enqueuePacket(p1);
    q.enqueuePacket(p2);
    q.removePacket(p1);
    TEST_ASSERT_EQUAL(1, q.size());
    q.removePacket(p2);
    TEST_ASSERT_TRUE(q.isEmpty());
}

// ---------- UNITY ENTRY POINT ----------

// ---------- UNITY ENTRY POINT ----------

void setUp() {}
void tearDown() {}

#ifdef ARDUINO
void setup()
{
    delay(2000); // give serial time to initialize
    UNITY_BEGIN();

    RUN_TEST(test_initial_state);
    RUN_TEST(test_enqueue_and_dequeue);
    RUN_TEST(test_remove_at_position);
    RUN_TEST(test_node_announce_goes_front);
    RUN_TEST(test_only_one_node_announce);
    RUN_TEST(test_new_neighbour_replaces_old_neighbour);
    RUN_TEST(test_new_neighbour_does_not_replace_old_neighbour_when_header_gone);
    RUN_TEST(test_enqueue_at_position);
    RUN_TEST(test_remove_specific_packet);
    RUN_TEST(test_node_announce_goes_after_fragment_if_header_gone);

    UNITY_END();
}

void loop() {}
#else
int main(int argc, char **argv)
{
    UNITY_BEGIN();

    RUN_TEST(test_initial_state);
    RUN_TEST(test_enqueue_and_dequeue);
    RUN_TEST(test_remove_at_position);
    RUN_TEST(test_node_announce_goes_front);
    RUN_TEST(test_only_one_node_announce);
    RUN_TEST(test_new_neighbour_replaces_old_neighbour);
    RUN_TEST(test_new_neighbour_does_not_replace_old_neighbour_when_header_gone);
    RUN_TEST(test_enqueue_at_position);
    RUN_TEST(test_remove_specific_packet);
    RUN_TEST(test_node_announce_goes_after_fragment_if_header_gone);

    UNITY_END();
    return 0;
}
#endif