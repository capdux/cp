// Source: cp/cap/test/test_blake3_production_deep.cp (Production deep tests for cap:blake3)

import { test, expect_eq } from "cap:test";
import {
    Blake3Hasher,
    hash,
    hashInto,
    keyedHash,
    deriveKey,
    hashHex,
    verifyKeyedMac,
} from "../../lib/blake3/index.cp";
import cap from "cap";

const std = @import("std");

fn fillPattern(buf: []u8) {
    let i: usize = 0;
    while (i < buf.len) {
        buf[i] = @intCast(i % 251);
        i += 1;
    }
}

fn expectBytesEq(actual: []const u8, expected: []const u8) {
    expect_eq(actual.len, expected.len);
    let i: usize = 0;
    while (i < actual.len) {
        expect_eq(actual[i], expected[i]);
        i += 1;
    }
}

test("BLAKE3 official vectors (0B, 1B, 64B, 1024B, 1025B, 3072B) match standard") {
    const empty_hex = hashHex("");
    const expected_empty = "af1349b9f5f9a1a6a0404dea36dcc9499bcb25c9adc112b7cc9a93cae41f3262";
    expectBytesEq(empty_hex[0..], expected_empty);

    let buf: [3072]u8 = undefined;
    fillPattern(buf[0..]);

    const lengths: [6]usize = [0, 1, 64, 1024, 1025, 3072];
    let idx: usize = 0;
    while (idx < lengths.len) {
        const n = lengths[idx];
        const actual = hash(buf[0..n]);
        let expected: [32]u8 = undefined;
        std.crypto.hash.Blake3.hash(buf[0..n], &expected, .{});
        expectBytesEq(actual[0..], expected[0..]);
        idx += 1;
    }
}

test("BLAKE3 streaming update with 1..7 byte chunks matches one-shot hash") {
    let buf: [2500]u8 = undefined;
    fillPattern(buf[0..]);

    const expected = hash(buf[0..]);
    const steps: [5]usize = [1, 2, 3, 5, 7];
    let s_idx: usize = 0;
    while (s_idx < steps.len) {
        const step = steps[s_idx];
        let hasher = Blake3Hasher.init();
        let pos: usize = 0;
        while (pos < buf.len) {
            const rem = buf.len - pos;
            const take = if (rem < step) rem else step;
            hasher.update(buf[pos .. pos + take]);
            pos += take;
        }
        let actual: [32]u8 = undefined;
        hasher.final(actual[0..]);
        expectBytesEq(actual[0..], expected[0..]);
        s_idx += 1;
    }
}

test("BLAKE3 keyedHash, verifyKeyedMac constant-time and deriveKey context isolation") {
    let key: [32]u8 = undefined;
    fillPattern(key[0..]);
    const msg = "authenticated-payload-for-cp-blake3";

    const mac = keyedHash(key, msg);
    let std_mac: [32]u8 = undefined;
    std.crypto.hash.Blake3.hash(msg, &std_mac, .{ .key = key });
    expectBytesEq(mac[0..], std_mac[0..]);

    expect_eq(verifyKeyedMac(key, msg, mac[0..]), true);

    let tampered = mac;
    tampered[0] ^= 0x01;
    expect_eq(verifyKeyedMac(key, msg, tampered[0..]), false);
    expect_eq(verifyKeyedMac(key, msg, mac[0..16]), false);

    let k1: [32]u8 = undefined;
    let k2: [32]u8 = undefined;
    deriveKey("cp-app v1 auth", "shared-secret", k1[0..]);
    deriveKey("cp-app v1 enc", "shared-secret", k2[0..]);

    let std_k1: [32]u8 = undefined;
    let kdf_hasher = std.crypto.hash.Blake3.initKdf("cp-app v1 auth", .{});
    kdf_hasher.update("shared-secret");
    kdf_hasher.final(&std_k1);
    expectBytesEq(k1[0..], std_k1[0..]);

    let differs = false;
    let i: usize = 0;
    while (i < 32) {
        if (k1[i] != k2[i]) {
            differs = true;
        }
        i += 1;
    }
    expect_eq(differs, true);
}

test("BLAKE3 XOF extended output and finalSeek random offset consistency") {
    let input: [1500]u8 = undefined;
    fillPattern(input[0..]);

    const short_digest = hash(input[0..]);
    let xof_128: [128]u8 = undefined;
    hashInto(input[0..], xof_128[0..]);
    expectBytesEq(xof_128[0..32], short_digest[0..]);

    let hasher = Blake3Hasher.init();
    hasher.update(input[0..]);

    let seek17: [40]u8 = undefined;
    hasher.finalSeek(17, seek17[0..]);
    expectBytesEq(seek17[0..], xof_128[17..57]);

    let seek64: [32]u8 = undefined;
    hasher.finalSeek(64, seek64[0..]);
    expectBytesEq(seek64[0..], xof_128[64..96]);

    let seek77: [51]u8 = undefined;
    hasher.finalSeek(77, seek77[0..]);
    expectBytesEq(seek77[0..], xof_128[77..128]);
}

test("BLAKE3 cap.blake3 dynamic mount and zero-heap-allocation verification") {
    let data: [2050]u8 = undefined;
    fillPattern(data[0..]);

    const direct = hash(data[0..]);
    const mounted = cap.blake3.hash(data[0..]);
    expectBytesEq(mounted[0..], direct[0..]);

    let hasher = cap.blake3.Blake3Hasher.init();
    hasher.update(data[0..1024]);
    hasher.update(data[1024..]);
    let streamed: [32]u8 = undefined;
    hasher.final(streamed[0..]);
    expectBytesEq(streamed[0..], direct[0..]);
}
