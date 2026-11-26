// cap/test/testCapSiphash.cp
// Comprehensive unit tests for @cap/siphash

import { test, expect_eq } from "cap:test";
import {
    SipHasher24,
    SipHasher13,
    siphash24,
    siphash13,
} from "../../packages/@cap/siphash/src/index.cp";

fn u64Pair(hi: u32, lo: u32) -> u64 {
    return ((hi as u64) << 32) | (lo as u64);
}

fn main() {
    test("1. @cap/siphash — SipHash-2-4 official Aumasson-Bernstein test vectors (len 0, 1, 15)", () => {
        const k0 = u64Pair(0x07060504, 0x03020100);
        const k1 = u64Pair(0x0f0e0d0c, 0x0b0a0908);

        // len = 0: "" -> 0x726fdb47dd0e0e31
        expect_eq(siphash24(k0, k1, ""), u64Pair(0x726fdb47, 0xdd0e0e31));

        // len = 1: "\x00" -> 0x74f839c593dc67fd
        let msg1: [1]u8 = [0x00];
        expect_eq(siphash24(k0, k1, msg1[0..1]), u64Pair(0x74f839c5, 0x93dc67fd));

        // len = 15: 00 01 ... 0e -> 0xa129ca6149be45e5
        let msg15: [15]u8 = [0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14];
        expect_eq(siphash24(k0, k1, msg15[0..15]), u64Pair(0xa129ca61, 0x49be45e5));
    });

    test("2. @cap/siphash — SipHash-2-4 additional official vectors (len 8 & len 16)", () => {
        const k0 = u64Pair(0x07060504, 0x03020100);
        const k1 = u64Pair(0x0f0e0d0c, 0x0b0a0908);

        // len = 8: 00..07 -> 0x93f5f5799a932462
        let msg8: [8]u8 = [0, 1, 2, 3, 4, 5, 6, 7];
        expect_eq(siphash24(k0, k1, msg8[0..8]), u64Pair(0x93f5f579, 0x9a932462));

        // len = 16: 00..0f -> 0x3f2acc7f57c29bdb
        let msg16: [16]u8 = [0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15];
        expect_eq(siphash24(k0, k1, msg16[0..16]), u64Pair(0x3f2acc7f, 0x57c29bdb));
    });

    test("3. @cap/siphash — Streaming byte-by-byte & chunked writes match one-shot SipHash-2-4", () => {
        const k0 = u64Pair(0x07060504, 0x03020100);
        const k1 = u64Pair(0x0f0e0d0c, 0x0b0a0908);
        let msg15: [15]u8 = [0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14];

        let h_byte = SipHasher24.init(k0, k1);
        let i: usize = 0;
        while (i < 15) {
            h_byte.write(msg15[i..(i + 1)]);
            i += 1;
        }
        expect_eq(h_byte.finish(), u64Pair(0xa129ca61, 0x49be45e5));
        // Idempotent finish()
        expect_eq(h_byte.finish(), u64Pair(0xa129ca61, 0x49be45e5));

        let h_chunks = SipHasher24.init(k0, k1);
        h_chunks.write(msg15[0..3]);
        h_chunks.write(msg15[3..11]);
        h_chunks.write(msg15[11..15]);
        expect_eq(h_chunks.finish(), u64Pair(0xa129ca61, 0x49be45e5));
    });

    test("4. @cap/siphash — SipHash-1-3 determinism & streaming equivalence", () => {
        const k0 = u64Pair(0x07060504, 0x03020100);
        const k1 = u64Pair(0x0f0e0d0c, 0x0b0a0908);

        const oneshot = siphash13(k0, k1, "The quick brown fox jumps over the lazy dog");
        expect_eq(oneshot != 0 as u64, true);
        expect_eq(oneshot != siphash24(k0, k1, "The quick brown fox jumps over the lazy dog"), true);

        let stream = SipHasher13.init(k0, k1);
        stream.write("The quick ");
        stream.write("brown fox ");
        stream.write("jumps over ");
        stream.write("the lazy dog");
        expect_eq(stream.finish(), oneshot);
    });

    test("5. @cap/siphash — Key isolation (HashDoS protection)", () => {
        const msg = "attack_collision_candidate_key";
        const d1 = siphash24(1, 2, msg);
        const d2 = siphash24(1, 3, msg);
        const d3 = siphash24(2, 2, msg);
        expect_eq(d1 != d2, true);
        expect_eq(d1 != d3, true);
        expect_eq(d2 != d3, true);

        const f1 = siphash13(100, 200, msg);
        const f2 = siphash13(100, 201, msg);
        expect_eq(f1 != f2, true);
    });

    test("6. @cap/siphash — Sensitivity to 1-bit input difference", () => {
        const k0: u64 = 0x11223344;
        const k1: u64 = 0x55667788;
        const a = siphash24(k0, k1, "capdux-0");
        const b = siphash24(k0, k1, "capdux-1");
        expect_eq(a != b, true);

        const fa = siphash13(k0, k1, "capdux-0");
        const fb = siphash13(k0, k1, "capdux-1");
        expect_eq(fa != fb, true);
    });
}
