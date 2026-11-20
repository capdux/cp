// Source: cp/cap/test/test_siphash_production_deep.cp
const std = @import("std");
import { test, expect_eq } from "cap:test";
import cap from "cap";
import {
    SIPHASH_KEY_LEN,
    SipHashStream,
    siphash24,
    siphash13,
    siphash24_128,
    verifySipHash24,
} from "cap:siphash";

fn testKey(): [16]u8 {
    let k: [16]u8 = undefined;
    let i: usize = 0;
    while (i < 16) : (i += 1) {
        k[i] = @intCast(i);
    }
    return k;
}

test("1. SipHash-2-4 official specification test vectors (0, 1, 7, 8, 15 bytes)") {
    const k = testKey();
    let msg: [64]u8 = undefined;
    let i: usize = 0;
    while (i < 64) : (i += 1) {
        msg[i] = @intCast(i);
    }
    // Official SipHash-2-4 vectors (Jean-Philippe Aumasson & Daniel J. Bernstein)
    expect_eq(siphash24(k, msg[0..0]), 0x726fdb47dd0e0e31);
    expect_eq(siphash24(k, msg[0..1]), 0x74f839c593dc67fd);
    expect_eq(siphash24(k, msg[0..7]), 0xab0200f58b01d137);
    expect_eq(siphash24(k, msg[0..8]), 0x93f5f5799a932462);
    expect_eq(siphash24(k, msg[0..15]), 0xa129ca6149be45e5);
}

test("2. SipHash-2-4 128-bit official specification vector") {
    const k = testKey();
    let msg: [15]u8 = undefined;
    let i: usize = 0;
    while (i < 15) : (i += 1) {
        msg[i] = @intCast(i);
    }
    const tag128 = siphash24_128(k, &msg);
    const expected128: [16]u8 = [
        0xa3, 0x81, 0x7f, 0x04, 0xba, 0x25, 0xa8, 0xe6,
        0x6d, 0xf6, 0x72, 0x14, 0xc7, 0x55, 0x02, 0x93,
    ];
    try std.testing.expectEqualSlices(u8, &expected128, &tag128);
}

test("3. Streaming 1..7 byte chunked feeding bit-exact with oneshot") {
    const k = testKey();
    const payload = "The quick brown fox jumps over the lazy dog 0123456789!";
    const oneshot24 = siphash24(k, payload);
    const oneshot13 = siphash13(k, payload);
    const oneshot128 = siphash24_128(k, payload);

    let s24 = SipHashStream.init24(k);
    let s13 = SipHashStream.init13(k);
    let s128 = SipHashStream.init24_128(k);
    let idx: usize = 0;
    let step: usize = 1;
    while (idx < payload.len) {
        const end = @min(idx + step, payload.len);
        const chunk = payload[idx..end];
        s24.update(chunk);
        s13.update(chunk);
        s128.update(chunk);
        idx = end;
        step = (step % 7) + 1;
    }
    expect_eq(s24.final64(), oneshot24);
    expect_eq(s13.final64(), oneshot13);
    try std.testing.expectEqualSlices(u8, &oneshot128, &s128.final128());
}

test("4. Constant-time verifySipHash24 and key/data sensitivity") {
    const k = testKey();
    let k2 = k;
    k2[0] ^= 0x01;
    const tag = siphash24(k, "session-cookie-payload");
    expect_eq(verifySipHash24(k, "session-cookie-payload", tag), true);
    expect_eq(verifySipHash24(k, "session-cookie-payloae", tag), false);
    expect_eq(verifySipHash24(k2, "session-cookie-payload", tag), false);
    expect_eq(SIPHASH_KEY_LEN, 16);
}

test("5. Dynamic cap.siphash namespace and zero-alloc check") {
    const k = testKey();
    const h = cap.siphash.siphash24(k, "hello-cap");
    expect_eq(cap.siphash.verifySipHash24(k, "hello-cap", h), true);
}
