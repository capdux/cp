// cap/test/testCapMqttPacket.cp
// Comprehensive unit tests for @cap/mqtt-packet (OASIS MQTT v3.1.1 parity)

import { test, expect_eq } from "cap:test";
import {
    PKT_PUBLISH,
    PKT_PUBACK,
    PKT_PINGREQ,
    PKT_PINGRESP,
    PKT_DISCONNECT,
    MQTT_OK,
    MQTT_INCOMPLETE,
    MQTT_INVALID,
    MqttHeader,
    MqttPublish,
    encodeRemainingLen,
    decodeHeader,
    encodePublish,
    decodePublish,
    encodeSimple,
    encodePuback,
} from "../../packages/mqttbytes/src/index.cp";

fn sliceEq(a: []const u8, b: []const u8) -> bool {
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
    test("1. @cap/mqtt-packet — Remaining Length 1/2/3/4-byte boundaries", () => {
        let buf: [8]u8 = undefined;
        buf[0] = PKT_PUBLISH << 4;

        // 0 -> 1 byte [0x00]
        expect_eq(encodeRemainingLen(0, buf[1..8]), 1 as i64);
        expect_eq(decodeHeader(buf[0..2]).remaining_len, 0 as u32);

        // 127 -> 1 byte [0x7F]
        expect_eq(encodeRemainingLen(127, buf[1..8]), 1 as i64);
        expect_eq(decodeHeader(buf[0..2]).remaining_len, 127 as u32);

        // 128 -> 2 bytes [0x80, 0x01]
        expect_eq(encodeRemainingLen(128, buf[1..8]), 2 as i64);
        expect_eq(buf[1], 0x80 as u8);
        expect_eq(buf[2], 0x01 as u8);
        expect_eq(decodeHeader(buf[0..3]).remaining_len, 128 as u32);

        // 16383 -> 2 bytes, 16384 -> 3 bytes
        expect_eq(encodeRemainingLen(16383, buf[1..8]), 2 as i64);
        expect_eq(decodeHeader(buf[0..3]).remaining_len, 16383 as u32);
        expect_eq(encodeRemainingLen(16384, buf[1..8]), 3 as i64);
        expect_eq(decodeHeader(buf[0..4]).remaining_len, 16384 as u32);

        // 2097152 -> 4 bytes
        expect_eq(encodeRemainingLen(2097152, buf[1..8]), 4 as i64);
        expect_eq(decodeHeader(buf[0..5]).remaining_len, 2097152 as u32);
    });

    test("2. @cap/mqtt-packet — QoS 0 encodePublish -> decodePublish round-trip", () => {
        let buf: [128]u8 = undefined;
        const n: i64 = encodePublish("home/temp", "24.5", 0, false, 0, buf[0..128]);
        expect_eq(n > 0, true);
        const pkt: []const u8 = buf[0..(n as usize)];

        const p: MqttPublish = decodePublish(pkt);
        expect_eq(p.status, MQTT_OK);
        expect_eq(p.qos, 0 as u8);
        expect_eq(p.retain, false);
        expect_eq(p.packet_id, 0 as u16);
        expect_eq(p.consumed, pkt.len);
        expect_eq(sliceEq(pkt[p.topic_start..(p.topic_start + p.topic_len)], "home/temp"), true);
        expect_eq(sliceEq(pkt[p.payload_start..(p.payload_start + p.payload_len)], "24.5"), true);
    });

    test("3. @cap/mqtt-packet — QoS 1 with packet_id & retain=true round-trip", () => {
        let buf: [128]u8 = undefined;
        const n: i64 = encodePublish("iot/cmd", "OPEN", 1, true, 0x1234, buf[0..128]);
        expect_eq(n > 0, true);
        const pkt: []const u8 = buf[0..(n as usize)];

        const p: MqttPublish = decodePublish(pkt);
        expect_eq(p.status, MQTT_OK);
        expect_eq(p.qos, 1 as u8);
        expect_eq(p.retain, true);
        expect_eq(p.dup, false);
        expect_eq(p.packet_id, 0x1234 as u16);
        expect_eq(sliceEq(pkt[p.topic_start..(p.topic_start + p.topic_len)], "iot/cmd"), true);
        expect_eq(sliceEq(pkt[p.payload_start..(p.payload_start + p.payload_len)], "OPEN"), true);
    });

    test("4. @cap/mqtt-packet — encodeSimple (PINGREQ/PINGRESP/DISCONNECT) & encodePuback", () => {
        let buf: [8]u8 = undefined;

        expect_eq(encodeSimple(PKT_PINGREQ, buf[0..8]), 2 as i64);
        expect_eq(buf[0], 0xC0 as u8);
        expect_eq(buf[1], 0x00 as u8);
        const h_ping: MqttHeader = decodeHeader(buf[0..2]);
        expect_eq(h_ping.status, MQTT_OK);
        expect_eq(h_ping.pkt_type, PKT_PINGREQ);

        expect_eq(encodeSimple(PKT_PINGRESP, buf[0..8]), 2 as i64);
        expect_eq(buf[0], 0xD0 as u8);

        expect_eq(encodeSimple(PKT_DISCONNECT, buf[0..8]), 2 as i64);
        expect_eq(buf[0], 0xE0 as u8);

        expect_eq(encodePuback(0xABCD, buf[0..8]), 4 as i64);
        expect_eq(buf[0], (PKT_PUBACK << 4) as u8);
        expect_eq(buf[1], 2 as u8);
        expect_eq(buf[2], 0xAB as u8);
        expect_eq(buf[3], 0xCD as u8);
    });

    test("5. @cap/mqtt-packet — truncated / partial frames return MQTT_INCOMPLETE", () => {
        let buf: [64]u8 = undefined;
        const n: i64 = encodePublish("a/b", "hello", 0, false, 0, buf[0..64]);
        expect_eq(n > 4, true);

        // Only 1 byte
        expect_eq(decodeHeader(buf[0..1]).status, MQTT_INCOMPLETE);
        // Header complete, payload truncated
        expect_eq(decodePublish(buf[0..((n as usize) - 1)]).status, MQTT_INCOMPLETE);
        // Incomplete varint (continuation bit set, no next byte)
        const partial_varint: [2]u8 = [0x30, 0x80];
        expect_eq(decodeHeader(partial_varint[0..2]).status, MQTT_INCOMPLETE);
    });

    test("6. @cap/mqtt-packet — rejects >4-byte varint, QoS 3 & tiny buffers", () => {
        // 5-byte varint (all continuation bits set)
        const bad_varint: [5]u8 = [0x30, 0x80, 0x80, 0x80, 0x80];
        expect_eq(decodeHeader(bad_varint[0..5]).status, MQTT_INVALID);

        // QoS 3 (flags 0x06) is invalid in MQTT v3.1.1
        const bad_qos: [6]u8 = [0x36, 0x04, 0x00, 0x01, 'a' as u8, 'x' as u8];
        expect_eq(decodePublish(bad_qos[0..6]).status, MQTT_INVALID);

        // Reject QoS 3 and tiny buffers in encoders
        let tiny: [4]u8 = undefined;
        expect_eq(encodePublish("a", "b", 3, false, 0, tiny[0..4]), -1 as i64);
        expect_eq(encodePublish("long/topic", "payload", 0, false, 0, tiny[0..4]), -1 as i64);
        expect_eq(encodePuback(1, tiny[0..3]), -1 as i64);
        expect_eq(encodeRemainingLen(268435456, tiny[0..4]), -1 as i64);
    });
}
