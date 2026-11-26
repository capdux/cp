import { test, expect_eq } from "cap:test";
import { Hc128Rng } from "../../packages/@cap/rand-hc/src/index.cp";

fn main() {
    test("1. @cap/rand-hc — all-zero seed determinism & known eSTREAM HC-128 first words", () => {
        const zero_seed: [32]u8 = [0; 32];
        let r1: Hc128Rng = Hc128Rng.fromSeed(zero_seed);
        let r2: Hc128Rng = Hc128Rng.fromSeed(zero_seed);

        const w0: u32 = r1.nextU32();
        const w1: u32 = r1.nextU32();
        const w2: u32 = r1.nextU32();
        const w3: u32 = r1.nextU32();

        // Official eSTREAM HC-128 test vector for Key=0, IV=0:
        // First two 32-bit keystream words: 0x73150082, 0x3bfd03a0
        expect_eq(w0, 0x73150082 as u32);
        expect_eq(w1, 0x3bfd03a0 as u32);

        expect_eq(r2.nextU32(), w0);
        expect_eq(r2.nextU32(), w1);
        expect_eq(r2.nextU32(), w2);
        expect_eq(r2.nextU32(), w3);
    });

    test("2. @cap/rand-hc — 1-bit seed flip produces avalanche divergence", () => {
        const seed_a: [32]u8 = [0; 32];
        let seed_b: [32]u8 = [0; 32];
        seed_b[0] = 1;

        let ra: Hc128Rng = Hc128Rng.fromSeed(seed_a);
        let rb: Hc128Rng = Hc128Rng.fromSeed(seed_b);

        let diff_words: usize = 0;
        let i: usize = 0;
        while (i < 16) : (i += 1) {
            if (ra.nextU32() != rb.nextU32()) {
                diff_words += 1;
            }
        }
        expect_eq(diff_words, 16 as usize);
    });

    test("3. @cap/rand-hc — IV sensitivity (same key, different IV)", () => {
        let seed_a: [32]u8 = [0; 32];
        let seed_b: [32]u8 = [0; 32];
        seed_a[0] = 0x55;
        seed_b[0] = 0x55;
        seed_b[16] = 0x01; // flip lowest bit of IV

        let ra: Hc128Rng = Hc128Rng.fromSeed(seed_a);
        let rb: Hc128Rng = Hc128Rng.fromSeed(seed_b);

        expect_eq(ra.nextU32() != rb.nextU32(), true);
        expect_eq(ra.nextU32() != rb.nextU32(), true);
    });

    test("4. @cap/rand-hc — nextU64 matches little-endian pair of nextU32", () => {
        let seed: [32]u8 = [0; 32];
        seed[3] = 0xAB;
        seed[19] = 0xCD;

        let r32: Hc128Rng = Hc128Rng.fromSeed(seed);
        let r64: Hc128Rng = Hc128Rng.fromSeed(seed);

        const lo: u64 = u64(r32.nextU32());
        const hi: u64 = u64(r32.nextU32());
        const expected: u64 = (hi << 32) | lo;

        expect_eq(r64.nextU64(), expected);
    });

    test("5. @cap/rand-hc — fillBytes matches nextU32 little-endian stream including unaligned tail", () => {
        const seed: [32]u8 = [0; 32];
        let rw: Hc128Rng = Hc128Rng.fromSeed(seed);
        let rb: Hc128Rng = Hc128Rng.fromSeed(seed);

        const w0: u32 = rw.nextU32();
        const w1: u32 = rw.nextU32();

        let buf: [7]u8 = [0; 7];
        rb.fillBytes(buf[0..7]);

        expect_eq(buf[0], u8(w0 & 0xFF));
        expect_eq(buf[1], u8((w0 >> 8) & 0xFF));
        expect_eq(buf[2], u8((w0 >> 16) & 0xFF));
        expect_eq(buf[3], u8((w0 >> 24) & 0xFF));
        expect_eq(buf[4], u8(w1 & 0xFF));
        expect_eq(buf[5], u8((w1 >> 8) & 0xFF));
        expect_eq(buf[6], u8((w1 >> 16) & 0xFF));
    });

    test("6. @cap/rand-hc — counter wraps cleanly across P (0..511) and Q (512..1023) halves", () => {
        const seed: [32]u8 = [0; 32];
        let r1: Hc128Rng = Hc128Rng.fromSeed(seed);
        let r2: Hc128Rng = Hc128Rng.fromSeed(seed);

        // Advance 1025 steps (crosses 512 boundary and 1024 wrap boundary)
        let last1: u32 = 0;
        let last2: u32 = 0;
        let i: usize = 0;
        while (i < 1025) : (i += 1) {
            last1 = r1.nextU32();
            last2 = r2.nextU32();
        }
        expect_eq(r1.counter, 1 as usize);
        expect_eq(last1, last2);
        expect_eq(last1 != 0, true);
    });
}
