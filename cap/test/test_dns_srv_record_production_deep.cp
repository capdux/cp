import std from "std";
import { test, expect_eq } from "cap:test";
import {
    SrvEntry as SrvRecord,
    SrvServiceName,
    compareSrvOrder,
    selectSrvByRoll,
} from "cap:dns";

test("1. SrvRecord.parseText and isServiceUnavailable handle standard and '.' unavailable records", () => {
    const r1 = SrvRecord.parseText("10 60 5060 sipserver.example.com.") ?? unreachable;
    expect_eq(r1.priority, u16(10));
    expect_eq(r1.weight, u16(60));
    expect_eq(r1.port, u16(5060));
    expect_eq(std.mem.eql(u8, r1.target, "sipserver.example.com."), true);
    expect_eq(r1.isServiceUnavailable(), false);

    const unavail = SrvRecord.parseText("  0\t0  0  .  ") ?? unreachable;
    expect_eq(unavail.isServiceUnavailable(), true);

    // Malformed text records rejected
    expect_eq(SrvRecord.parseText("") == null, true);
    expect_eq(SrvRecord.parseText("10 60 5060") == null, true);
    expect_eq(SrvRecord.parseText("10 60 65536 host.example.com.") == null, true);
    expect_eq(SrvRecord.parseText("10 60 5060 host.example.com. trailing") == null, true);
    expect_eq(SrvRecord.parseText("10 60 5060 bad..host.") == null, true);
});

test("2. SrvRecord.parseWireHeader decodes 6-byte big-endian header and target wire slice", () => {
    // priority=10 (0x000A), weight=500 (0x01F4), port=8443 (0x20FB), target_wire="\x03api\x00"
    const wire: [11]u8 = [0x00, 0x0A, 0x01, 0xF4, 0x20, 0xFB, 0x03, 'a', 'p', 'i', 0x00];
    const hdr = SrvRecord.parseWireHeader(wire[0..11]) ?? unreachable;
    expect_eq(hdr.priority, u16(10));
    expect_eq(hdr.weight, u16(500));
    expect_eq(hdr.port, u16(8443));
    expect_eq(hdr.target_wire.len, usize(5));
    expect_eq(hdr.target_wire[0], u8(0x03));

    const short_wire: [5]u8 = [0, 1, 0, 2, 0];
    expect_eq(SrvRecord.parseWireHeader(short_wire[0..5]) == null, true);
});

test("3. SrvServiceName.parse extracts _service._proto.domain and rejects malformed names", () => {
    const s1 = SrvServiceName.parse("_http._tcp.example.com.") ?? unreachable;
    expect_eq(std.mem.eql(u8, s1.service, "http"), true);
    expect_eq(std.mem.eql(u8, s1.proto, "tcp"), true);
    expect_eq(std.mem.eql(u8, s1.domain, "example.com."), true);

    const s2 = SrvServiceName.parse("_sip-tls._udp.corp.local") ?? unreachable;
    expect_eq(std.mem.eql(u8, s2.service, "sip-tls"), true);
    expect_eq(std.mem.eql(u8, s2.proto, "udp"), true);
    expect_eq(std.mem.eql(u8, s2.domain, "corp.local"), true);

    // Invalid SRV service names
    expect_eq(SrvServiceName.parse("http._tcp.example.com.") == null, true);
    expect_eq(SrvServiceName.parse("_http.tcp.example.com.") == null, true);
    expect_eq(SrvServiceName.parse("_.tcp.example.com.") == null, true);
    expect_eq(SrvServiceName.parse("_http._.example.com.") == null, true);
    expect_eq(SrvServiceName.parse("_http._tcp.") == null, true);
    expect_eq(SrvServiceName.parse("_http._tcp..") == null, true);
});

test("4. compareSrvOrder orders by priority ascending then weight descending", () => {
    const a = SrvRecord.parseText("10 20 443 a.example.com.") ?? unreachable;
    const b = SrvRecord.parseText("10 80 443 b.example.com.") ?? unreachable;
    const c = SrvRecord.parseText("20 100 443 c.example.com.") ?? unreachable;

    expect_eq(compareSrvOrder(a, c), i32(-1));
    expect_eq(compareSrvOrder(c, a), i32(1));
    expect_eq(compareSrvOrder(b, a), i32(-1));
    expect_eq(compareSrvOrder(a, b), i32(1));
    expect_eq(compareSrvOrder(a, a), i32(0));
});

test("5. selectSrvByRoll deterministically selects weighted targets and handles zero-weight / unavailable", () => {
    const r0 = SrvRecord.parseText("10 70 443 a.example.com.") ?? unreachable;
    const r1 = SrvRecord.parseText("20 100 443 backup.example.com.") ?? unreachable;
    const r2 = SrvRecord.parseText("10 30 443 b.example.com.") ?? unreachable;
    const r3 = SrvRecord.parseText("10 50 443 .") ?? unreachable; // unavailable, must be ignored

    const list: [4]SrvRecord = [r0, r1, r2, r3];

    // Priority 10 total valid weight = 70 + 30 = 100
    expect_eq(selectSrvByRoll(list[0..4], 10, 0) ?? unreachable, usize(0));
    expect_eq(selectSrvByRoll(list[0..4], 10, 69) ?? unreachable, usize(0));
    expect_eq(selectSrvByRoll(list[0..4], 10, 70) ?? unreachable, usize(2));
    expect_eq(selectSrvByRoll(list[0..4], 10, 99) ?? unreachable, usize(2));
    expect_eq(selectSrvByRoll(list[0..4], 10, 175) ?? unreachable, usize(2)); // 175 % 100 = 75

    // Priority 20 hits index 1; Priority 30 has no candidates
    expect_eq(selectSrvByRoll(list[0..4], 20, 0) ?? unreachable, usize(1));
    expect_eq(selectSrvByRoll(list[0..4], 30, 0) == null, true);

    // All zero-weight candidates distribute deterministically by roll % count
    const z0 = SrvRecord.parseText("5 0 80 z0.example.com.") ?? unreachable;
    const z1 = SrvRecord.parseText("5 0 80 z1.example.com.") ?? unreachable;
    const zlist: [2]SrvRecord = [z0, z1];
    expect_eq(selectSrvByRoll(zlist[0..2], 5, 0) ?? unreachable, usize(0));
    expect_eq(selectSrvByRoll(zlist[0..2], 5, 1) ?? unreachable, usize(1));
    expect_eq(selectSrvByRoll(zlist[0..2], 5, 2) ?? unreachable, usize(0));
});
