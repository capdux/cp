// Source: cp/cap/test/test_murmur3_production_deep.cp
const std = @import("std");
import { test, expect_eq } from "cap:test";
import cap from "cap";
import {
    murmur3_32,
    murmur3_128,
    Murmur3_32Stream,
    Murmur3_128Stream,
} from "cap:murmur3";

test("1. MurmurHash3_x86_32 official specification test vectors") {
    expect_eq(murmur3_32("", 0), 0x00000000);
    expect_eq(murmur3_32("", 1), 0x514E28B7);
    expect_eq(murmur3_32("", 0xffffffff), 0x81F16F39);
    expect_eq(murmur3_32("hello", 0), 0x248BFA47);
    expect_eq(murmur3_32("hello", 1), 0xBB4ABCAD);
    expect_eq(murmur3_32("The quick brown fox jumps over the lazy dog", 0), 0x2E4FF723);
}

test("2. MurmurHash3_x64_128 official specification test vectors") {
    const empty_0 = murmur3_128("", 0);
    let zeroes: [16]u8 = [ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 ];
    try std.testing.expectEqualSlices(u8, &zeroes, &empty_0);

    const h1 = murmur3_128("hello", 0);
    const h2 = murmur3_128("hello", 1);
    try std.testing.expect(!std.mem.eql(u8, &h1, &h2));

    // Verify 128-bit SMHasher vector for "The quick brown fox jumps over the lazy dog" seed 0
    // h1=0x6c1b07bc7bbc4be3, h2=0x46936915688894c2 (little endian)
    const fox = murmur3_128("The quick brown fox jumps over the lazy dog", 0);
    expect_eq(std.mem.readInt(u64, fox[0..8], .little), 0x6c1b07bc7bbc4be3);
}

test("3. Streaming 1..7 byte chunked feeding bit-exact with oneshot") {
    const payload = "MurmurHash3 streaming chunk test across multiple 4-byte and 16-byte boundaries!";
    const oneshot32 = murmur3_32(payload, 0x9747b28c);
    const oneshot128 = murmur3_128(payload, 0x9747b28c);

    let s32 = Murmur3_32Stream.init(0x9747b28c);
    let s128 = Murmur3_128Stream.init(0x9747b28c);
    let idx: usize = 0;
    let step: usize = 1;
    while (idx < payload.len) {
        const end = @min(idx + step, payload.len);
        s32.update(payload[idx..end]);
        s128.update(payload[idx..end]);
        idx = end;
        step = (step % 7) + 1;
    }
    expect_eq(s32.final(), oneshot32);
    try std.testing.expectEqualSlices(u8, &oneshot128, &s128.final());
}

test("4. Avalanche sensitivity across 0..64 byte lengths") {
    let prev = murmur3_32("seed-0", 42);
    let buf: [64]u8 = undefined;
    let i: usize = 0;
    while (i < 64) : (i += 1) {
        buf[i] = @intCast(i);
    }
    let len: usize = 1;
    while (len <= 64) : (len += 1) {
        const cur = murmur3_32(buf[0..len], 42);
        try std.testing.expect(cur != prev);
        prev = cur;
    }
}

test("5. Dynamic cap.murmur3 namespace and zero-alloc check") {
    expect_eq(cap.murmur3.murmur3_32("hello", 0), 0x248BFA47);
    const h128 = cap.murmur3.murmur3_128("cap", 7);
    expect_eq(h128.len, 16);
}
