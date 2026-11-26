import { test, expect_eq } from "cap:test";
import {
    isValidTopicName,
    hasMetricCollisionRisk,
    murmur2,
    toPositive,
    partitionForKey,
    parseTopicPartition,
} from "../../packages/@cap/kafka-topic/src/index.cp";

fn testEq(a: []const u8, b: []const u8) -> bool {
    if (a.len != b.len) {
        return false;
    }
    let i: usize = 0;
    while (i < a.len) {
        if (a[i] != b[i]) {
            return false;
        }
        i += 1;
    }
    return true;
}

fn main() {
    test("1. @cap/kafka-topic — Topic name validation & boundary rejection", () => {
        expect_eq(isValidTopicName("orders.events"), true);
        expect_eq(isValidTopicName("user_clicks-v2"), true);
        expect_eq(isValidTopicName("a"), true);

        expect_eq(isValidTopicName(""), false);
        expect_eq(isValidTopicName("."), false);
        expect_eq(isValidTopicName(".."), false);
        expect_eq(isValidTopicName("bad/topic"), false);
        expect_eq(isValidTopicName("bad topic"), false);

        // 250-char name exceeds Kafka's 249-byte limit
        let long_name: [250]u8 = undefined;
        let i: usize = 0;
        while (i < 250) {
            long_name[i] = 97; // 'a'
            i += 1;
        }
        expect_eq(isValidTopicName(long_name[0..249]), true);
        expect_eq(isValidTopicName(long_name[0..250]), false);
    });

    test("2. @cap/kafka-topic — JMX metric collision risk detection ('.' + '_')", () => {
        expect_eq(hasMetricCollisionRisk("orders.events_v1"), true);
        expect_eq(hasMetricCollisionRisk("orders.events"), false);
        expect_eq(hasMetricCollisionRisk("orders_events"), false);
        expect_eq(hasMetricCollisionRisk("orders-events"), false);
    });

    test("3. @cap/kafka-topic — Official Apache Kafka Utils.murmur2 test vectors", () => {
        // Canonical vectors from Apache Kafka org.apache.kafka.common.utils.UtilsTest:
        // "21" -> -973932308 (0xC5F2F8EC)
        expect_eq(murmur2("21"), 0xC5F2F8EC as u32);
        // "foobar" -> -790332482 (0xD0E47BBE)
        expect_eq(murmur2("foobar"), 0xD0E47BBE as u32);
        // "abc" -> 479470107 (0x1C94221B)
        expect_eq(murmur2("abc"), 0x1C94221B as u32);
        // "a-little-bit-long-string" -> -985981536 (0xC53B1DA0)
        expect_eq(murmur2("a-little-bit-long-string"), 0xC53B1DA0 as u32);
        // "a-little-bit-longer-string" -> 0xA768C9C3
        expect_eq(murmur2("a-little-bit-longer-string"), 0xA768C9C3 as u32);
        // "lkjh234lh9fiuh90y23oiuhsafujhadof229phr9h19h89h8" -> -58897971 (0xFC7D49CD)
        expect_eq(murmur2("lkjh234lh9fiuh90y23oiuhsafujhadof229phr9h19h89h8"), 0xFC7D49CD as u32);
        // "" -> 0x106E08D9
        expect_eq(murmur2(""), 0x106E08D9 as u32);
    });

    test("4. @cap/kafka-topic — toPositive & partitionForKey deterministic routing", () => {
        expect_eq(toPositive(0x80000005), 5 as u32);
        expect_eq(toPositive(0x7FFFFFFF), 0x7FFFFFFF as u32);

        const p1 = partitionForKey("customer-42", 12);
        const p2 = partitionForKey("customer-42", 12);
        expect_eq(p1, p2);
        expect_eq(p1 < 12, true);

        // Zero partitions safely returns 0
        expect_eq(partitionForKey("customer-42", 0), 0 as u32);
        expect_eq(partitionForKey("", 16) < 16, true);
    });

    test("5. @cap/kafka-topic — parseTopicPartition colon & hyphen formats", () => {
        const tp1 = parseTopicPartition("orders.events:12");
        expect_eq(tp1.valid, true);
        expect_eq(testEq(tp1.topic, "orders.events"), true);
        expect_eq(tp1.partition, 12 as i32);

        const tp2 = parseTopicPartition("user-clicks-v2-7");
        expect_eq(tp2.valid, true);
        expect_eq(testEq(tp2.topic, "user-clicks-v2"), true);
        expect_eq(tp2.partition, 7 as i32);

        const tp3 = parseTopicPartition("logs:0");
        expect_eq(tp3.valid, true);
        expect_eq(tp3.partition, 0 as i32);
    });

    test("6. @cap/kafka-topic — Invalid topic-partition specs rejected", () => {
        expect_eq(parseTopicPartition("").valid, false);
        expect_eq(parseTopicPartition("orders").valid, false);
        expect_eq(parseTopicPartition("orders:").valid, false);
        expect_eq(parseTopicPartition(":5").valid, false);
        expect_eq(parseTopicPartition("orders:-1").valid, false);
        expect_eq(parseTopicPartition("orders:abc").valid, false);
        expect_eq(parseTopicPartition("bad/topic:1").valid, false);
    });
}
