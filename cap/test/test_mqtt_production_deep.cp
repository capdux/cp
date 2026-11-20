// Source: cp/cap/test/test_mqtt_production_deep.cp (MQTT v3.1.1 / v5.0 Production Deep Test Suite)
const std = @import("std");
import { test, expect_eq } from "cap:test";
import cap from "cap";
import {
    MAX_VAR_INT,
    MqttError,
    MqttVersion,
    QoS,
    WillMessage,
    ConnectPacket,
    ConnackPacket,
    PublishPacket,
    TopicSubscription,
    SubscribePacket,
    SubackPacket,
    UnsubscribePacket,
    DisconnectPacket,
    MqttPacket,
    encodeVarByteInt,
    decodeVarByteInt,
    encodePacket,
    decodePacket,
    MqttStreamDecoder,
    validateTopicName,
    validateTopicFilter,
    topicMatches,
} from "cap:mqtt";

struct VarIntCase {
    val: usize,
    expected_len: usize,
}

test("mqtt varint boundaries and 5-byte malformed interception") {
    const cases = [
        VarIntCase{ .val = 0, .expected_len = 1 },
        VarIntCase{ .val = 127, .expected_len = 1 },
        VarIntCase{ .val = 128, .expected_len = 2 },
        VarIntCase{ .val = 16383, .expected_len = 2 },
        VarIntCase{ .val = 16384, .expected_len = 3 },
        VarIntCase{ .val = MAX_VAR_INT, .expected_len = 4 },
    ];

    for (c of cases) {
        let buf: [4]u8 = undefined;
        const n = try encodeVarByteInt(c.val, &buf);
        expect_eq(n, c.expected_len);
        const res = try decodeVarByteInt(buf[0..n]);
        expect_eq(res.value, c.val);
        expect_eq(res.bytes_read, c.expected_len);
    }

    // Overflow > MAX_VAR_INT
    let over_buf: [4]u8 = undefined;
    try std.testing.expectError(MqttError.MalformedVarInt, encodeVarByteInt(MAX_VAR_INT + 1, &over_buf));

    // 5-byte malformed varint (4th byte has continuation bit 0x80 set)
    const malformed = [ 0xFF, 0xFF, 0xFF, 0x80, 0x01 ];
    try std.testing.expectError(MqttError.MalformedVarInt, decodeVarByteInt(&malformed));

    // Incomplete varint
    const incomplete = [ 0x80, 0x80 ];
    try std.testing.expectError(MqttError.IncompletePacket, decodeVarByteInt(&incomplete));
}

test("mqtt full packet round-trip and validation rules") {
    const alloc = std.testing.allocator;

    // 1. CONNECT with Will, Username, Password (v3.1.1 & v5.0)
    const conn_pkt = MqttPacket{
        .connect = ConnectPacket{
            .version = MqttVersion.v3_1_1,
            .client_id = "cp-agent-01",
            .clean_session = true,
            .keep_alive = 60,
            .will = WillMessage{
                .topic = "agents/01/status",
                .payload = "offline",
                .qos = QoS.at_least_once,
                .retain = true,
            },
            .username = "alice",
            .password = "secret123",
        },
    };
    const conn_wire = try encodePacket(alloc, conn_pkt);
    defer alloc.free(conn_wire);
    const conn_dec = try decodePacket(alloc, conn_wire);
    defer conn_dec.deinit(alloc);
    expect_eq(conn_dec.connect.version, MqttVersion.v3_1_1);
    expect_eq(std.mem.eql(u8, conn_dec.connect.client_id, "cp-agent-01"), true);
    expect_eq(conn_dec.connect.keep_alive, 60);
    expect_eq(std.mem.eql(u8, conn_dec.connect.will.?.topic, "agents/01/status"), true);
    expect_eq(std.mem.eql(u8, conn_dec.connect.username.?, "alice"), true);
    expect_eq(std.mem.eql(u8, conn_dec.connect.password.?, "secret123"), true);

    // 2. CONNACK
    const ca_wire = try encodePacket(alloc, MqttPacket{
        .connack = ConnackPacket{ .session_present = true, .return_code = 0 },
    });
    defer alloc.free(ca_wire);
    const ca_dec = try decodePacket(alloc, ca_wire);
    expect_eq(ca_dec.connack.session_present, true);
    expect_eq(ca_dec.connack.return_code, 0);

    // 3. PUBLISH QoS 0 / 1 / 2 + validation
    const pub0_wire = try encodePacket(alloc, MqttPacket{
        .publish = PublishPacket{
            .dup = false,
            .qos = QoS.at_most_once,
            .retain = false,
            .topic = "telemetry/cpu",
            .packet_id = null,
            .payload = "12%",
        },
    });
    defer alloc.free(pub0_wire);
    const pub0_dec = try decodePacket(alloc, pub0_wire);
    expect_eq(pub0_dec.publish.qos, QoS.at_most_once);
    expect_eq(pub0_dec.publish.packet_id == null, true);

    const pub2_wire = try encodePacket(alloc, MqttPacket{
        .publish = PublishPacket{
            .dup = true,
            .qos = QoS.exactly_once,
            .retain = true,
            .topic = "telemetry/mem",
            .packet_id = 1024,
            .payload = "64MB",
        },
    });
    defer alloc.free(pub2_wire);
    const pub2_dec = try decodePacket(alloc, pub2_wire);
    expect_eq(pub2_dec.publish.dup, true);
    expect_eq(pub2_dec.publish.retain, true);
    expect_eq(pub2_dec.publish.qos, QoS.exactly_once);
    expect_eq(pub2_dec.publish.packet_id.?, 1024);

    // QoS 0 with packet_id -> UnexpectedPacketId
    try std.testing.expectError(MqttError.UnexpectedPacketId, encodePacket(alloc, MqttPacket{
        .publish = PublishPacket{
            .dup = false,
            .qos = QoS.at_most_once,
            .retain = false,
            .topic = "a/b",
            .packet_id = 1,
            .payload = "",
        },
    }));

    // QoS 1 without packet_id -> MissingPacketId
    try std.testing.expectError(MqttError.MissingPacketId, encodePacket(alloc, MqttPacket{
        .publish = PublishPacket{
            .dup = false,
            .qos = QoS.at_least_once,
            .retain = false,
            .topic = "a/b",
            .packet_id = null,
            .payload = "",
        },
    }));

    // 4. SUBSCRIBE / SUBACK
    const subs = [
        TopicSubscription{ .filter = "sensors/+/temp", .qos = QoS.at_least_once },
        TopicSubscription{ .filter = "alerts/#", .qos = QoS.exactly_once },
    ];
    const sub_wire = try encodePacket(alloc, MqttPacket{
        .subscribe = SubscribePacket{ .packet_id = 77, .topics = &subs },
    });
    defer alloc.free(sub_wire);
    const sub_dec = try decodePacket(alloc, sub_wire);
    defer sub_dec.deinit(alloc);
    expect_eq(sub_dec.subscribe.packet_id, 77);
    expect_eq(sub_dec.subscribe.topics.len, 2);
    expect_eq(std.mem.eql(u8, sub_dec.subscribe.topics[1].filter, "alerts/#"), true);

    const codes = [ 1, 2 ];
    const sa_wire = try encodePacket(alloc, MqttPacket{
        .suback = SubackPacket{ .packet_id = 77, .return_codes = &codes },
    });
    defer alloc.free(sa_wire);
    const sa_dec = try decodePacket(alloc, sa_wire);
    defer sa_dec.deinit(alloc);
    expect_eq(sa_dec.suback.return_codes.len, 2);

    // 5. PINGREQ / DISCONNECT
    const ping_wire = try encodePacket(alloc, MqttPacket.pingreq);
    defer alloc.free(ping_wire);
    const ping_dec = try decodePacket(alloc, ping_wire);
    expect_eq(ping_dec == .pingreq, true);

    const disc_wire = try encodePacket(alloc, MqttPacket{
        .disconnect = DisconnectPacket{ .reason_code = 0 },
    });
    defer alloc.free(disc_wire);
    const disc_dec = try decodePacket(alloc, disc_wire);
    expect_eq(disc_dec.disconnect.reason_code, 0);

    // 6. UTF-8 null char rejection
    try std.testing.expectError(MqttError.InvalidUtf8, encodePacket(alloc, MqttPacket{
        .connect = ConnectPacket{
            .version = MqttVersion.v5_0,
            .client_id = "bad\x00id",
            .clean_session = true,
            .keep_alive = 30,
            .will = null,
            .username = null,
            .password = null,
        },
    }));
}

test("mqtt stream decoder 1..7 byte chunk slicing and max_packet_bytes limit") {
    const alloc = std.testing.allocator;

    const p1 = try encodePacket(alloc, MqttPacket{
        .publish = PublishPacket{
            .dup = false,
            .qos = QoS.at_least_once,
            .retain = false,
            .topic = "stream/one",
            .packet_id = 11,
            .payload = "hello-chunked-mqtt",
        },
    });
    defer alloc.free(p1);

    const p2 = try encodePacket(alloc, MqttPacket.pingreq);
    defer alloc.free(p2);

    const p3 = try encodePacket(alloc, MqttPacket{
        .puback = 11,
    });
    defer alloc.free(p3);

    // Concatenate sticky packets into one stream
    let combined = std.ArrayList(u8).empty;
    defer combined.deinit(alloc);
    try combined.appendSlice(alloc, p1);
    try combined.appendSlice(alloc, p2);
    try combined.appendSlice(alloc, p3);

    let decoder = MqttStreamDecoder.init(alloc, 256);
    defer decoder.deinit();

    let decoded_count: usize = 0;
    let pos: usize = 0;
    let step: usize = 1;
    while (pos < combined.items.len) {
        const end = @min(pos + step, combined.items.len);
        try decoder.feed(combined.items[pos..end]);
        pos = end;
        step = (step % 7) + 1;

        while (pkt := try decoder.nextPacket()) {
            defer pkt.deinit(alloc);
            decoded_count += 1;
            if (decoded_count == 1) {
                expect_eq(std.mem.eql(u8, pkt.publish.payload, "hello-chunked-mqtt"), true);
            } else if (decoded_count == 2) {
                expect_eq(pkt == .pingreq, true);
            } else if (decoded_count == 3) {
                expect_eq(pkt.puback, 11);
            }
        }
    }
    expect_eq(decoded_count, 3);

    // max_packet_bytes interception
    let small_decoder = MqttStreamDecoder.init(alloc, 10);
    defer small_decoder.deinit();
    try small_decoder.feed(p1);
    try std.testing.expectError(MqttError.PacketTooLarge, small_decoder.nextPacket());
}

test("mqtt topic wildcard matching and validation rules") {
    // Valid & invalid topic names
    try validateTopicName("sensors/room1/temp");
    try std.testing.expectError(MqttError.InvalidTopicName, validateTopicName(""));
    try std.testing.expectError(MqttError.InvalidTopicName, validateTopicName("sensors/+/temp"));
    try std.testing.expectError(MqttError.InvalidTopicName, validateTopicName("sensors/#"));
    try std.testing.expectError(MqttError.InvalidTopicName, validateTopicName("bad\x00topic"));

    // Valid & invalid topic filters
    try validateTopicFilter("#");
    try validateTopicFilter("sport/tennis/player1/#");
    try validateTopicFilter("sport/+/player1");
    try std.testing.expectError(MqttError.InvalidTopicFilter, validateTopicFilter("sport/tennis#"));
    try std.testing.expectError(MqttError.InvalidTopicFilter, validateTopicFilter("sport/#/ranking"));
    try std.testing.expectError(MqttError.InvalidTopicFilter, validateTopicFilter("sport/+tennis"));

    // Standard wildcard matching + zero-segment '#'
    expect_eq(topicMatches("sport/tennis/player1/#", "sport/tennis/player1"), true);
    expect_eq(topicMatches("sport/tennis/player1/#", "sport/tennis/player1/ranking"), true);
    expect_eq(topicMatches("sport/tennis/player1/#", "sport/tennis/player1/score/wimbledon"), true);
    expect_eq(topicMatches("sport/tennis/+", "sport/tennis/player1"), true);
    expect_eq(topicMatches("sport/tennis/+", "sport/tennis/"), true);
    expect_eq(topicMatches("sport/tennis/+", "sport/tennis"), false);
    expect_eq(topicMatches("sport/tennis/+", "sport/tennis/player1/ranking"), false);

    // $SYS isolation per MQTT §4.7.2
    expect_eq(topicMatches("#", "$SYS/broker/load"), false);
    expect_eq(topicMatches("+/broker/load", "$SYS/broker/load"), false);
    expect_eq(topicMatches("$SYS/#", "$SYS/broker/load"), true);
    expect_eq(topicMatches("$SYS/+/load", "$SYS/broker/load"), true);
}

test("cap.mqtt dynamic mount and zero-leak verification") {
    const alloc = std.testing.allocator;
    expect_eq(cap.mqtt.topicMatches("devices/+/telemetry", "devices/edge-01/telemetry"), true);

    const wire = try cap.mqtt.encodePacket(alloc, cap.mqtt.MqttPacket{
        .unsuback = 55,
    });
    defer alloc.free(wire);

    const pkt = try cap.mqtt.decodePacket(alloc, wire);
    defer pkt.deinit(alloc);
    expect_eq(pkt.unsuback, 55);
}
