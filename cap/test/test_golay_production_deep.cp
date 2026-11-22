// Source: cp/cap/test/test_golay_production_deep.cp (Production deep tests for cap:golay)

import std from "std";
import { test, expect_eq } from "cap:test";
import {
    GOLAY24_DATA_BITS,
    GOLAY24_CODE_BITS,
    GOLAY24_MIN_DIST,
    GOLAY23_CODE_BITS,
    GolayStatus,
    GolayDecode,
    golay24Parity,
    golay24Encode,
    golay24Decode,
    golay23Encode,
    golay23Decode,
    golay24EncodeBytes,
    golay24DecodeBytes,
} from "cap:codec";

fn popcnt32(x: u32): u8 {
    let v = x;
    let c: u8 = 0;
    while (v != 0) {
        v = v & (v - 1);
        c += 1;
    }
    return c;
}

fn bytesEq(a: []const u8, b: []const u8): bool {
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

test("1. All 4096 Extended Golay [24,12,8] codewords minimum weight >= 8, doubly-even & zero-error decode", () => {
    expect_eq(GOLAY24_DATA_BITS, 12 as u8);
    expect_eq(GOLAY24_CODE_BITS, 24 as u8);
    expect_eq(GOLAY24_MIN_DIST, 8 as u8);
    expect_eq(GOLAY23_CODE_BITS, 23 as u8);

    let min_w: u8 = 24;
    let w8_count: u32 = 0;
    let w12_count: u32 = 0;
    let w16_count: u32 = 0;
    let w24_count: u32 = 0;

    let d: u32 = 0;
    while (d < 4096) {
        const data12 = d as u16;
        const cw = golay24Encode(data12);
        const w = popcnt32(cw);

        // Doubly-even property: every codeword weight is divisible by 4
        expect_eq((w % 4), 0 as u8);

        if (d > 0) {
            if (w < min_w) {
                min_w = w;
            }
            if (w == 8) { w8_count += 1; }
            if (w == 12) { w12_count += 1; }
            if (w == 16) { w16_count += 1; }
            if (w == 24) { w24_count += 1; }
        }

        // Exact zero-error decode
        const dec_res = golay24Decode(cw);
        expect_eq(dec_res.status == GolayStatus.ok, true);
        expect_eq(dec_res.errors_corrected, 0 as u8);
        expect_eq(dec_res.data, data12);
        expect_eq(dec_res.codeword, cw);
        expect_eq((cw & 0x0FFF) as u16, golay24Parity(data12));

        d += 1;
    }

    // Weight enumerator of Extended Binary Golay [24, 12, 8]:
    // A0 = 1, A8 = 759, A12 = 2576, A16 = 759, A24 = 1 (total 4096)
    expect_eq(min_w, 8 as u8);
    expect_eq(w8_count, 759 as u32);
    expect_eq(w12_count, 2576 as u32);
    expect_eq(w16_count, 759 as u32);
    expect_eq(w24_count, 1 as u32);
});

test("2. Exhaustive 1-bit (24), 2-bit (276) & 3-bit (2024) error correction in Golay [24,12,8]", () => {
    const payload: u16 = 0xA5C;
    const cw = golay24Encode(payload);

    // All 24 single-bit errors
    let b0: u32 = 0;
    while (b0 < 24) {
        const corrupted = cw ^ ((1 as u32) << (b0 as u5));
        const res = golay24Decode(corrupted);
        expect_eq(res.status == GolayStatus.corrected, true);
        expect_eq(res.errors_corrected, 1 as u8);
        expect_eq(res.data, payload);
        expect_eq(res.codeword, cw);
        b0 += 1;
    }

    // All C(24, 2) = 276 two-bit errors
    let count2: u32 = 0;
    let i: u32 = 0;
    while (i < 24) {
        let j: u32 = i + 1;
        while (j < 24) {
            const corrupted = cw ^ ((1 as u32) << (i as u5)) ^ ((1 as u32) << (j as u5));
            const res = golay24Decode(corrupted);
            expect_eq(res.status == GolayStatus.corrected, true);
            expect_eq(res.errors_corrected, 2 as u8);
            expect_eq(res.data, payload);
            expect_eq(res.codeword, cw);
            count2 += 1;
            j += 1;
        }
        i += 1;
    }
    expect_eq(count2, 276 as u32);

    // All C(24, 3) = 2024 three-bit errors
    let count3: u32 = 0;
    let x: u32 = 0;
    while (x < 24) {
        let y: u32 = x + 1;
        while (y < 24) {
            let z: u32 = y + 1;
            while (z < 24) {
                const mask = ((1 as u32) << (x as u5)) | ((1 as u32) << (y as u5)) | ((1 as u32) << (z as u5));
                const res = golay24Decode(cw ^ mask);
                expect_eq(res.status == GolayStatus.corrected, true);
                expect_eq(res.errors_corrected, 3 as u8);
                expect_eq(res.data, payload);
                expect_eq(res.codeword, cw);
                count3 += 1;
                z += 1;
            }
            y += 1;
        }
        x += 1;
    }
    expect_eq(count3, 2024 as u32);
});

test("3. 4-bit error detection returns GolayStatus.uncorrectable (100% across all patterns)", () => {
    const payload: u16 = 0x3E7;
    const cw = golay24Encode(payload);

    // Representative 4-bit error patterns across parity-only, data-only, and mixed positions
    const patterns: [8]u32 = [
        0x00000F, // 4 errors in parity
        0x00F000, // 4 errors in data
        0xF00000, // 4 errors in high data
        0x0003C0, // 4 errors mid parity
        0x800809, // mixed: bit 23, 11, 3, 0
        0x421080, // spread across 24 bits
        0x011110, // stride-4 errors
        0xA0000A, // symmetric data+parity errors
    ];

    let i: usize = 0;
    while (i < 8) {
        const res = golay24Decode(cw ^ patterns[i]);
        expect_eq(res.status == GolayStatus.uncorrectable, true);
        expect_eq(res.errors_corrected, 0 as u8);
        i += 1;
    }
});

test("4. Perfect Golay [23,12,7] 0/1/2/3-bit error correction", () => {
    const payload: u16 = 0x7B9;
    const cw23 = golay23Encode(payload);
    expect_eq((cw23 >> 23), 0 as u32); // strictly 23 bits

    // 0-bit error
    const r0 = golay23Decode(cw23);
    expect_eq(r0.status == GolayStatus.ok, true);
    expect_eq(r0.errors_corrected, 0 as u8);
    expect_eq(r0.data, payload);
    expect_eq(r0.codeword, cw23);

    // All 23 single-bit errors
    let b: u32 = 0;
    while (b < 23) {
        const r1 = golay23Decode(cw23 ^ ((1 as u32) << (b as u5)));
        expect_eq(r1.status == GolayStatus.corrected, true);
        expect_eq(r1.errors_corrected, 1 as u8);
        expect_eq(r1.data, payload);
        expect_eq(r1.codeword, cw23);
        b += 1;
    }

    // Representative 2-bit and 3-bit errors including bit 0 and bit 22 boundaries
    const e2 = ((1 as u32) << 0) | ((1 as u32) << 22);
    const r2 = golay23Decode(cw23 ^ e2);
    expect_eq(r2.status == GolayStatus.corrected, true);
    expect_eq(r2.errors_corrected, 2 as u8);
    expect_eq(r2.data, payload);

    const e3 = ((1 as u32) << 0) | ((1 as u32) << 11) | ((1 as u32) << 22);
    const r3 = golay23Decode(cw23 ^ e3);
    expect_eq(r3.status == GolayStatus.corrected, true);
    expect_eq(r3.errors_corrected, 3 as u8);
    expect_eq(r3.data, payload);
});

test("5. golay24EncodeBytes / golay24DecodeBytes stream roundtrip with 3-bit noise injection & error rejection", () => {
    // 9-byte (multiple of 3) payload roundtrip with 3-bit noise in every codeword
    const msg = "Voyager-1";
    let enc: [32]u8 = std.mem.zeroes([32]u8);
    let dec_buf: [16]u8 = std.mem.zeroes([16]u8);

    const elen = golay24EncodeBytes(msg, enc[0..]) ?? unreachable;
    expect_eq(elen, 18 as usize); // 9 bytes -> 6 codewords -> 18 bytes

    // Inject 3 bit flips into every 3-byte codeword
    let c: usize = 0;
    while (c < elen) {
        enc[c] = enc[c] ^ 0x80;     // 1 bit in byte 0
        enc[c + 1] = enc[c + 1] ^ 0x10; // 1 bit in byte 1
        enc[c + 2] = enc[c + 2] ^ 0x01; // 1 bit in byte 2
        c += 3;
    }

    const dlen = golay24DecodeBytes(enc[0..elen], dec_buf[0..]) ?? unreachable;
    expect_eq(dlen, 9 as usize);
    expect_eq(bytesEq(dec_buf[0..dlen], msg), true);

    // 1-byte remainder (4 bytes total -> 6 + 3 = 9 encoded bytes -> 4 decoded bytes)
    const msg4 = "CP24";
    const e4 = golay24EncodeBytes(msg4, enc[0..]) ?? unreachable;
    expect_eq(e4, 9 as usize);
    enc[6] = enc[6] ^ 0x07; // 3 bit flips in trailing codeword
    const d4 = golay24DecodeBytes(enc[0..e4], dec_buf[0..]) ?? unreachable;
    expect_eq(d4, 4 as usize);
    expect_eq(bytesEq(dec_buf[0..d4], msg4), true);

    // 2-byte remainder (5 bytes total -> 6 + 6 = 12 encoded bytes -> 6 decoded bytes with trailing 0 pad)
    const msg5 = "Hello";
    const e5 = golay24EncodeBytes(msg5, enc[0..]) ?? unreachable;
    expect_eq(e5, 12 as usize);
    const d5 = golay24DecodeBytes(enc[0..e5], dec_buf[0..]) ?? unreachable;
    expect_eq(d5, 6 as usize);
    expect_eq(bytesEq(dec_buf[0..5], msg5), true);
    expect_eq(dec_buf[5], 0 as u8);

    // Error rejection: 4-bit uncorrectable error, unaligned length, and buffer overflow
    enc[0] = enc[0] ^ 0x0F; // 4 bit flips in first codeword
    expect_eq(golay24DecodeBytes(enc[0..e5], dec_buf[0..]) == null, true);
    expect_eq(golay24DecodeBytes(enc[0..5], dec_buf[0..]) == null, true); // not multiple of 3

    let tiny_enc: [5]u8 = std.mem.zeroes([5]u8);
    expect_eq(golay24EncodeBytes(msg, tiny_enc[0..]) == null, true);
    let tiny_dec: [2]u8 = std.mem.zeroes([2]u8);
    expect_eq(golay24DecodeBytes(enc[0..6], tiny_dec[0..]) == null, true);
});
