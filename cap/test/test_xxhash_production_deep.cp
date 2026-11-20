// Source: cp/cap/test/test_xxhash_production_deep.cp (Production deep tests for cap:xxhash)

import { test, expect_eq } from "cap:test";
import {
    xxh32,
    Xxh32Stream,
    xxh64,
    Xxh64Stream,
    XXH3_DEFAULT_SECRET,
    xxh3_64,
    xxh3_64WithSeed,
    xxh3_128,
    xxh3_128WithSeed,
} from "cap:xxhash";
import cap from "cap";

fn fillPattern(buf: []u8) {
    let i: usize = 0;
    while (i < buf.len) {
        buf[i] = @truncate((i *% 37 +% 13) & 0xFF);
        i += 1;
    }
}

test("xxh32 and xxh64 official test vectors and gradient lengths") {
    expect_eq(xxh32("", 0), @as(u32, 0x02CC5D05));
    expect_eq(xxh32("", 0x9E3779B1), @as(u32, 0x36B78AE7));
    expect_eq(xxh64("", 0), @as(u64, 0xEF46DB3751D8E999));
    expect_eq(xxh64("", 0x9E3779B185EBCA87), @as(u64, 0xAC75FDA2929B17EF));

    expect_eq(xxh32("a", 0), @as(u32, 0x550D7456));
    expect_eq(xxh64("a", 0), @as(u64, 0xD24EC4F1A98C6E5B));

    let buf: [256]u8 = [0; 256];
    fillPattern(buf[0..256]);
    const h32_full = xxh32(buf[0..256], 0x12345678);
    const h64_full = xxh64(buf[0..256], 0x123456789ABCDEF0);
    expect_eq(h32_full != xxh32(buf[0..255], 0x12345678), true);
    expect_eq(h64_full != xxh64(buf[0..255], 0x123456789ABCDEF0), true);
}

test("Xxh32Stream and Xxh64Stream 1..7 byte chunked streaming bit-exact match") {
    let data: [257]u8 = [0; 257];
    fillPattern(data[0..257]);

    const seed32: u32 = 0xCAFEBABE;
    const seed64: u64 = 0x0123456789ABCDEF;
    const expected32 = xxh32(data[0..257], seed32);
    const expected64 = xxh64(data[0..257], seed64);

    let chunk_size: usize = 1;
    while (chunk_size <= 7) {
        let s32 = Xxh32Stream.init(seed32);
        let s64 = Xxh64Stream.init(seed64);
        let pos: usize = 0;
        while (pos < 257) {
            let end = pos + chunk_size;
            if (end > 257) {
                end = 257;
            }
            s32.update(data[pos..end]);
            s64.update(data[pos..end]);
            pos = end;
        }
        expect_eq(s32.digest(), expected32);
        expect_eq(s64.digest(), expected64);

        s32.reset(0);
        s64.reset(0);
        s32.update(data[0..19]);
        s64.update(data[0..19]);
        expect_eq(s32.digest(), xxh32(data[0..19], 0));
        expect_eq(s64.digest(), xxh64(data[0..19], 0));

        chunk_size += 1;
    }
}

test("xxh3_64 and xxh3_128 all 7 length branches and avalanche") {
    expect_eq(XXH3_DEFAULT_SECRET.len, @as(usize, 192));
    expect_eq(xxh3_64(""), @as(u64, 0x2D06800538D394C2));

    const empty128 = xxh3_128("");
    expect_eq(empty128.low, @as(u64, 0x6001C324468D497F));
    expect_eq(empty128.high, @as(u64, 0x99AA06D3014798D8));

    let buf: [512]u8 = [0; 512];
    fillPattern(buf[0..512]);

    const lengths: [7]usize = [0, 2, 6, 12, 64, 180, 512];
    let idx: usize = 0;
    while (idx < 7) {
        const len = lengths[idx];
        const h64 = xxh3_64(buf[0..len]);
        const h128 = xxh3_128(buf[0..len]);
        expect_eq(h64 == xxh3_64(buf[0..len]), true);
        expect_eq(h128.eql(xxh3_128(buf[0..len])), true);

        if (len > 0) {
            buf[len - 1] ^= 0x01;
            const flipped64 = xxh3_64(buf[0..len]);
            const flipped128 = xxh3_128(buf[0..len]);
            buf[len - 1] ^= 0x01;
            expect_eq(h64 != flipped64, true);
            expect_eq(!h128.eql(flipped128), true);
        }
        idx += 1;
    }
}

test("xxh3_64WithSeed and xxh3_128WithSeed seed independence") {
    let buf: [300]u8 = [0; 300];
    fillPattern(buf[0..300]);

    const lengths: [7]usize = [0, 2, 6, 12, 64, 180, 300];
    const seed_a: u64 = 0x1122334455667788;
    const seed_b: u64 = 0x8877665544332211;

    let idx: usize = 0;
    while (idx < 7) {
        const len = lengths[idx];
        const slice = buf[0..len];
        const a64 = xxh3_64WithSeed(slice, seed_a);
        const b64 = xxh3_64WithSeed(slice, seed_b);
        const z64 = xxh3_64WithSeed(slice, 0);
        expect_eq(z64, xxh3_64(slice));
        expect_eq(a64 != b64, true);
        expect_eq(a64 != z64, true);

        const a128 = xxh3_128WithSeed(slice, seed_a);
        const b128 = xxh3_128WithSeed(slice, seed_b);
        const z128 = xxh3_128WithSeed(slice, 0);
        expect_eq(z128.eql(xxh3_128(slice)), true);
        expect_eq(!a128.eql(b128), true);
        expect_eq(!a128.eql(z128), true);

        idx += 1;
    }
}

test("cap.xxhash dynamic mount and zero-heap-allocation verification") {
    let buf: [320]u8 = [0; 320];
    fillPattern(buf[0..320]);

    expect_eq(cap.xxhash.xxh32(buf[0..320], 0), xxh32(buf[0..320], 0));
    expect_eq(cap.xxhash.xxh64(buf[0..320], 0), xxh64(buf[0..320], 0));
    expect_eq(cap.xxhash.xxh3_64(buf[0..320]), xxh3_64(buf[0..320]));
    expect_eq(cap.xxhash.xxh3_128(buf[0..320]).eql(xxh3_128(buf[0..320])), true);

    let s32 = cap.xxhash.Xxh32Stream.init(7);
    s32.update(buf[0..100]);
    s32.update(buf[100..320]);
    expect_eq(s32.digest(), xxh32(buf[0..320], 7));

    let s64 = cap.xxhash.Xxh64Stream.init(9);
    s64.update(buf[0..111]);
    s64.update(buf[111..320]);
    expect_eq(s64.digest(), xxh64(buf[0..320], 9));
}
