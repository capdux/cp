// Source: cp/cap/test/test_mqtt_production_deep.cp
// Deep Production Verification Suite for cap:mqtt & cap.mqtt
// Covers:
//   1. RemainingLength boundary vectors (0, 127, 128, 16383, 16384, 2097151, 268435455) & 4th-byte 0x80 rejection
//   2. CONNECT (CleanSession/Username/Password/Will) & CONNACK round-trip codec
//   3. All control packets round-trip (PUBLISH QoS 0/1/2 + DUP/RETAIN, PUBACK/PUBREC/PUBREL/PUBCOMP, SUBSCRIBE/SUBACK, UNSUBSCRIBE/UNSUBACK, PINGREQ/PINGRESP, DISCONNECT)
//   4. MqttStreamDecoder byte-by-byte fragmented feeds, coalesced multi-packet frames, PacketTooLarge guard & zero-leak check via std.testing.allocator
//   5. `import cap from "cap"; cap.mqtt` automatic dynamic mounting

const std = @import("std");
import { test, expect_eq } from "cap:test";
import cap from "cap";
import {
    MAX_REMAINING_LENGTH,
    MqttError,
    MqttPacketType,
    MqttQos,
    ConnAckCode,
    MqttWill,
    ConnectPacket,
    ConnAckPacket,
    PublishPacket,
    PacketIdOnly,
    TopicFilter,
    SubscribePacket,
    SubAckPacket,
    UnsubscribePacket,
    MqttPacket,
    MqttCodec,
    MqttStreamDecoder,
} from "cap:mqtt";

fn testRemainingLengthBoundaries(): !void {
    const values: [7]usize = [0, 127, 128, 16383, 16384, 2097151, 268435455];
    const expected_lens: [7]usize = [1, 1, 2, 2, 3, 3, 4];

    for (idx of 0..values.len) {
        let buf: [4]u8 = [0; 4];
        const written = MqttCodec.encodeRemainingLength(values[idx], buf[0..])?;
        expect_eq(written, expected_lens[idx]);

        const decoded_rl = MqttCodec.decodeRemainingLength(buf[0..written])?;
        expect_eq(decoded_rl.value, values[idx]);
        expect_eq(decoded_rl.consumed, expected_lens[idx]);
    }

    // Verify specific canonical encodings
    let b128: [4]u8 = [0; 4];
    * = MqttCodec.encodeRemainingLength(128, b128[0..])?;
    expect_eq(b128[0], 0x80 as u8);
    expect_eq(b128[1], 0x01 as u8);

    let bmax: [4]u8 = [0; 4];
    * = MqttCodec.encodeRemainingLength(MAX_REMAINING_LENGTH, bmax[0..])?;
    expect_eq(bmax[0], 0xFF as u8);
    expect_eq(bmax[1], 0xFF as u8);
    expect_eq(bmax[2], 0xFF as u8);
    expect_eq(bmax[3], 0x7F as u8);

    // 4th byte has continuation bit 0x80 set -> MalformedRemainingLength
    const malformed: [4]u8 = [0xFF, 0xFF, 0xFF, 0x80];
    let hit_malformed = false;
    * = MqttCodec.decodeRemainingLength(malformed[0..]) catch e => {
        if (e == MqttError.MalformedRemainingLength) hit_malformed = true;
    };
    expect_eq(hit_malformed, true);

    // > MAX_REMAINING_LENGTH -> PacketTooLarge
    let out_buf: [4]u8 = [0; 4];
    let hit_too_large = false;
    * = MqttCodec.encodeRemainingLength(MAX_REMAINING_LENGTH + 1, out_buf[0..]) catch e => {
        if (e == MqttError.PacketTooLarge) hit_too_large = true;
    };
    expect_eq(hit_too_large, true);
}

fn testConnectAndConnAckRoundTrip(): !void {
    let gpa: std.heap.DebugAllocator(.{}) = .init;
    defer { if (gpa.deinit() == .leak) @panic("memory leak detected"); }
    const alloc = gpa.allocator();

    const connect_pkt = MqttPacket{
        Connect: ConnectPacket{
            protocol_name: "MQTT",
            protocol_level: 4,
            clean_session: true,
            keep_alive: 45,
            client_id: "cp-edge-node-01",
            will: MqttWill{
                topic: "devices/node-01/status",
                message: "offline",
                qos: MqttQos.AtLeastOnce,
                retain: true,
            },
            username: "iot_user",
            password: "s3cr3t_password",
        },
    };

    const wire = MqttCodec.encode(alloc, connect_pkt)?;
    defer alloc.free(wire);

    const res = MqttCodec.decodeOne(alloc, wire)?;
    let decoded = res.packet;
    defer decoded.deinit(alloc);
    expect_eq(res.consumed, wire.len);

    switch (decoded) {
        .Connect c => {
            expect_eq(std.mem.eql(u8, c.protocol_name, "MQTT"), true);
            expect_eq(c.protocol_level, 4 as u8);
            expect_eq(c.clean_session, true);
            expect_eq(c.keep_alive, 45 as u16);
            expect_eq(std.mem.eql(u8, c.client_id, "cp-edge-node-01"), true);
            if (w := c.will) {
                expect_eq(std.mem.eql(u8, w.topic, "devices/node-01/status"), true);
                expect_eq(std.mem.eql(u8, w.message, "offline"), true);
                expect_eq(w.qos == .AtLeastOnce, true);
                expect_eq(w.retain, true);
            } else {
                expect_eq(true, false);
            }
            if (u := c.username) {
                expect_eq(std.mem.eql(u8, u, "iot_user"), true);
            } else {
                expect_eq(true, false);
            }
            if (pw := c.password) {
                expect_eq(std.mem.eql(u8, pw, "s3cr3t_password"), true);
            } else {
                expect_eq(true, false);
            }
        },
        else => expect_eq(true, false),
    }

    // CONNACK round-trip
    const connack_wire = MqttCodec.encode(alloc, MqttPacket{
        ConnAck: ConnAckPacket{
            session_present: true,
            return_code: ConnAckCode.Accepted,
        },
    })?;
    defer alloc.free(connack_wire);
    expect_eq(connack_wire.len, 4 as usize);

    const ca_res = MqttCodec.decodeOne(alloc, connack_wire)?;
    let ca_pkt = ca_res.packet;
    defer ca_pkt.deinit(alloc);
    switch (ca_pkt) {
        .ConnAck ca => {
            expect_eq(ca.session_present, true);
            expect_eq(ca.return_code == .Accepted, true);
        },
        else => expect_eq(true, false),
    }
}

fn testAllControlPacketsRoundTrip(): !void {
    let gpa: std.heap.DebugAllocator(.{}) = .init;
    defer { if (gpa.deinit() == .leak) @panic("memory leak detected"); }
    const alloc = gpa.allocator();

    // 1. PUBLISH QoS 0 (no packet_id)
    const pub0_wire = MqttCodec.encode(alloc, MqttPacket{
        Publish: PublishPacket{
            dup: false,
            qos: MqttQos.AtMostOnce,
            retain: false,
            topic: "telemetry/cpu",
            packet_id: null,
            payload: "18%",
        },
    })?;
    defer alloc.free(pub0_wire);
    const pub0_res = MqttCodec.decodeOne(alloc, pub0_wire)?;
    let pub0 = pub0_res.packet;
    defer pub0.deinit(alloc);
    switch (pub0) {
        .Publish p => {
            expect_eq(p.qos == .AtMostOnce, true);
            expect_eq(p.packet_id == null, true);
            expect_eq(std.mem.eql(u8, p.topic, "telemetry/cpu"), true);
            expect_eq(std.mem.eql(u8, p.payload, "18%"), true);
        },
        else => expect_eq(true, false),
    }

    // 2. PUBLISH QoS 2 with DUP + RETAIN + packet_id
    const pub2_wire = MqttCodec.encode(alloc, MqttPacket{
        Publish: PublishPacket{
            dup: true,
            qos: MqttQos.ExactlyOnce,
            retain: true,
            topic: "commands/reboot",
            packet_id: 0x1234,
            payload: "now",
        },
    })?;
    defer alloc.free(pub2_wire);
    const pub2_res = MqttCodec.decodeOne(alloc, pub2_wire)?;
    let pub2 = pub2_res.packet;
    defer pub2.deinit(alloc);
    switch (pub2) {
        .Publish p => {
            expect_eq(p.dup, true);
            expect_eq(p.qos == .ExactlyOnce, true);
            expect_eq(p.retain, true);
            expect_eq(p.packet_id.?, 0x1234 as u16);
            expect_eq(std.mem.eql(u8, p.payload, "now"), true);
        },
        else => expect_eq(true, false),
    }

    // PUBLISH QoS 1 without packet_id -> MissingPacketId
    let hit_missing_pid = false;
    * = MqttCodec.encode(alloc, MqttPacket{
        Publish: PublishPacket{
            qos: MqttQos.AtLeastOnce,
            topic: "bad/pub",
            packet_id: null,
            payload: "x",
        },
    }) catch e => {
        if (e == MqttError.MissingPacketId) hit_missing_pid = true;
    };
    expect_eq(hit_missing_pid, true);

    // 3. PUBACK & PUBREL (verify 0b0010 fixed flags on PUBREL)
    const puback_wire = MqttCodec.encode(alloc, MqttPacket{ PubAck: PacketIdOnly{ packet_id: 77 } })?;
    defer alloc.free(puback_wire);
    expect_eq(puback_wire[0], 0x40 as u8);
    const puback_res = MqttCodec.decodeOne(alloc, puback_wire)?;
    let puback = puback_res.packet;
    defer puback.deinit(alloc);
    switch (puback) {
        .PubAck a => expect_eq(a.packet_id, 77 as u16),
        else => expect_eq(true, false),
    }

    const pubrel_wire = MqttCodec.encode(alloc, MqttPacket{ PubRel: PacketIdOnly{ packet_id: 88 } })?;
    defer alloc.free(pubrel_wire);
    expect_eq(pubrel_wire[0], 0x62 as u8); // type=6, flags=0b0010
    const pubrel_res = MqttCodec.decodeOne(alloc, pubrel_wire)?;
    let pubrel = pubrel_res.packet;
    defer pubrel.deinit(alloc);
    switch (pubrel) {
        .PubRel r => expect_eq(r.packet_id, 88 as u16),
        else => expect_eq(true, false),
    }

    // 4. SUBSCRIBE & SUBACK & UNSUBSCRIBE & UNSUBACK
    const filters: [2]TopicFilter = [
        TopicFilter{ topic: "sensors/+/temp", qos: MqttQos.AtLeastOnce },
        TopicFilter{ topic: "alerts/#", qos: MqttQos.ExactlyOnce },
    ];
    const sub_wire = MqttCodec.encode(alloc, MqttPacket{
        Subscribe: SubscribePacket{
            packet_id: 901,
            topics: filters[0..],
        },
    })?;
    defer alloc.free(sub_wire);
    expect_eq(sub_wire[0], 0x82 as u8); // type=8, flags=0b0010
    const sub_res = MqttCodec.decodeOne(alloc, sub_wire)?;
    let sub = sub_res.packet;
    defer sub.deinit(alloc);
    switch (sub) {
        .Subscribe s => {
            expect_eq(s.packet_id, 901 as u16);
            expect_eq(s.topics.len, 2 as usize);
            expect_eq(std.mem.eql(u8, s.topics[0].topic, "sensors/+/temp"), true);
            expect_eq(s.topics[0].qos == .AtLeastOnce, true);
            expect_eq(std.mem.eql(u8, s.topics[1].topic, "alerts/#"), true);
            expect_eq(s.topics[1].qos == .ExactlyOnce, true);
        },
        else => expect_eq(true, false),
    }

    const rc_codes: [2]u8 = [0x01, 0x02];
    const suback_wire = MqttCodec.encode(alloc, MqttPacket{
        SubAck: SubAckPacket{
            packet_id: 901,
            return_codes: rc_codes[0..],
        },
    })?;
    defer alloc.free(suback_wire);
    const suback_res = MqttCodec.decodeOne(alloc, suback_wire)?;
    let suback = suback_res.packet;
    defer suback.deinit(alloc);
    switch (suback) {
        .SubAck sa => {
            expect_eq(sa.packet_id, 901 as u16);
            expect_eq(sa.return_codes.len, 2 as usize);
            expect_eq(sa.return_codes[1], 0x02 as u8);
        },
        else => expect_eq(true, false),
    }

    const unsub_topics: [1]string = ["sensors/+/temp"];
    const unsub_wire = MqttCodec.encode(alloc, MqttPacket{
        Unsubscribe: UnsubscribePacket{
            packet_id: 902,
            topics: unsub_topics[0..],
        },
    })?;
    defer alloc.free(unsub_wire);
    expect_eq(unsub_wire[0], 0xA2 as u8); // type=10, flags=0b0010
    const unsub_res = MqttCodec.decodeOne(alloc, unsub_wire)?;
    let unsub = unsub_res.packet;
    defer unsub.deinit(alloc);
    switch (unsub) {
        .Unsubscribe u => {
            expect_eq(u.packet_id, 902 as u16);
            expect_eq(u.topics.len, 1 as usize);
            expect_eq(std.mem.eql(u8, u.topics[0], "sensors/+/temp"), true);
        },
        else => expect_eq(true, false),
    }

    // 5. PINGREQ / PINGRESP / DISCONNECT
    const ping_wire = MqttCodec.encode(alloc, MqttPacket.PingReq)?;
    defer alloc.free(ping_wire);
    expect_eq(ping_wire[0], 0xC0 as u8);
    expect_eq(ping_wire[1], 0x00 as u8);
    const ping_res = MqttCodec.decodeOne(alloc, ping_wire)?;
    let ping_pkt = ping_res.packet;
    defer ping_pkt.deinit(alloc);
    switch (ping_pkt) {
        .PingReq => {},
        else => expect_eq(true, false),
    }

    const disc_wire = MqttCodec.encode(alloc, MqttPacket.Disconnect)?;
    defer alloc.free(disc_wire);
    expect_eq(disc_wire[0], 0xE0 as u8);
    const disc_res = MqttCodec.decodeOne(alloc, disc_wire)?;
    let disc_pkt = disc_res.packet;
    defer disc_pkt.deinit(alloc);
    switch (disc_pkt) {
        .Disconnect => {},
        else => expect_eq(true, false),
    }
}

fn testStreamDecoderFragmentedAndCoalesced(): !void {
    let gpa: std.heap.DebugAllocator(.{}) = .init;
    defer { if (gpa.deinit() == .leak) @panic("memory leak detected"); }
    const alloc = gpa.allocator();

    const p1 = MqttCodec.encode(alloc, MqttPacket{
        Publish: PublishPacket{
            qos: MqttQos.AtLeastOnce,
            topic: "edge/a",
            packet_id: 10,
            payload: "hello",
        },
    })?;
    defer alloc.free(p1);

    const p2 = MqttCodec.encode(alloc, MqttPacket.PingResp)?;
    defer alloc.free(p2);

    const p3 = MqttCodec.encode(alloc, MqttPacket{
        PubComp: PacketIdOnly{ packet_id: 99 },
    })?;
    defer alloc.free(p3);

    // Combine all 3 packets into a single coalesced stream
    let stream_bytes: std.ArrayList(u8) = .empty;
    defer stream_bytes.deinit(alloc);
    stream_bytes.appendSlice(alloc, p1)?;
    stream_bytes.appendSlice(alloc, p2)?;
    stream_bytes.appendSlice(alloc, p3)?;

    let decoder = MqttStreamDecoder.init(alloc, 4096);
    defer decoder.deinit();

    // Feed 1 byte at a time to exercise partial/fragmented decoding
    let decoded_count: usize = 0;
    let idx: usize = 0;
    while (idx < stream_bytes.items.len) : (idx += 1) {
        decoder.feed(stream_bytes.items[idx .. idx + 1])?;
        while (true) {
            const maybe_pkt = decoder.nextPacket(alloc) catch e => return e;
            if (pkt_opt := maybe_pkt) {
                let pkt = pkt_opt;
                defer pkt.deinit(alloc);
                if (decoded_count == 0) {
                    switch (pkt) {
                        .Publish p => {
                            expect_eq(p.packet_id.?, 10 as u16);
                            expect_eq(std.mem.eql(u8, p.payload, "hello"), true);
                        },
                        else => expect_eq(true, false),
                    }
                } else if (decoded_count == 1) {
                    switch (pkt) {
                        .PingResp => {},
                        else => expect_eq(true, false),
                    }
                } else if (decoded_count == 2) {
                    switch (pkt) {
                        .PubComp c => expect_eq(c.packet_id, 99 as u16),
                        else => expect_eq(true, false),
                    }
                }
                decoded_count += 1;
            } else {
                break;
            }
        }
    }
    expect_eq(decoded_count, 3 as usize);
    expect_eq(decoder.bufferedLen(), 0 as usize);

    // PacketTooLarge protection (max_packet_bytes = 16)
    let small_dec = MqttStreamDecoder.init(alloc, 16);
    defer small_dec.deinit();

    const big_pub = MqttCodec.encode(alloc, MqttPacket{
        Publish: PublishPacket{
            qos: MqttQos.AtMostOnce,
            topic: "oversize/topic",
            payload: "0123456789abcdef",
        },
    })?;
    defer alloc.free(big_pub);

    // Even feeding just the header triggers PacketTooLarge immediately
    small_dec.feed(big_pub[0..3])?;
    let hit_too_large = false;
    * = small_dec.nextPacket(alloc) catch e => {
        if (e == MqttError.PacketTooLarge) hit_too_large = true;
    };
    expect_eq(hit_too_large, true);
}

fn testCapMqttDynamicMount(): !void {
    let gpa: std.heap.DebugAllocator(.{}) = .init;
    defer { if (gpa.deinit() == .leak) @panic("memory leak detected"); }
    const alloc = gpa.allocator();

    const wire = cap.mqtt.MqttCodec.encode(alloc, cap.mqtt.MqttPacket{
        Publish: cap.mqtt.PublishPacket{
            qos: cap.mqtt.MqttQos.AtLeastOnce,
            topic: "cap/mount/check",
            packet_id: 55,
            payload: "ok",
        },
    })?;
    defer alloc.free(wire);

    let decoder = cap.mqtt.MqttStreamDecoder.init(alloc, 1024);
    defer decoder.deinit();
    decoder.feed(wire)?;

    const maybe_pkt = decoder.nextPacket(alloc) catch e => return e;
    if (pkt := maybe_pkt) {
        let owned: cap.mqtt.MqttPacket = pkt;
        defer owned.deinit(alloc);
        switch (owned) {
            .Publish p => {
                expect_eq(p.packet_id.?, 55 as u16);
                expect_eq(std.mem.eql(u8, p.topic, "cap/mount/check"), true);
                expect_eq(std.mem.eql(u8, p.payload, "ok"), true);
            },
            else => expect_eq(true, false),
        }
    } else {
        expect_eq(true, false);
    }
}

fn main() {
    test("1. RemainingLength boundary vectors and 4th-byte 0x80 malformed rejection", () => {
        testRemainingLengthBoundaries()?;
    });
    test("2. CONNECT (CleanSession/User/Pass/Will) & CONNACK round-trip", () => {
        testConnectAndConnAckRoundTrip()?;
    });
    test("3. Full MQTT 3.1.1 control packets round-trip & protocol flag enforcement", () => {
        testAllControlPacketsRoundTrip()?;
    });
    test("4. MqttStreamDecoder byte-by-byte half-packet, coalesced frames & PacketTooLarge", () => {
        testStreamDecoderFragmentedAndCoalesced()?;
    });
    test("5. import cap from \"cap\"; cap.mqtt Automatic Dynamic Mounting", () => {
        testCapMqttDynamicMount()?;
    });
}
