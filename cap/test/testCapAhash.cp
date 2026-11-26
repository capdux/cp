// cap/test/testCapAhash.cp
// Comprehensive unit tests for @cap/ahash

import { test, expect_eq } from "cap:test";
import {
    RandomState,
    AHasher,
    hashBytes,
    hashU64,
} from "../../packages/@cap/ahash/src/index.cp";

fn popcount64(x_in: u64) -> usize {
    let x = x_in;
    let c: usize = 0;
    while (x != (0 as u64)) {
        x = x & (x - (1 as u64));
        c += 1;
    }
    return c;
}

fn main() {
    test("1. @cap/ahash — Determinism & empty / integer hashing", () => {
        const rs = RandomState.defaultState();
        const h1 = rs.hashOneBytes("");
        const h2 = rs.hashOneBytes("");
        expect_eq(h1, h2);

        const ha = rs.hashOneBytes("alpha");
        const hb = rs.hashOneBytes("beta");
        expect_eq(h1 != ha, true);
        expect_eq(ha != hb, true);

        const u1 = rs.hashOneU64(42 as u64);
        const u2 = rs.hashOneU64(43 as u64);
        expect_eq(u1 != u2, true);
        expect_eq(hashU64(100 as u64, 7 as u64, 9 as u64), hashU64(100 as u64, 7 as u64, 9 as u64));
    });

    test("2. @cap/ahash — 4-key RandomState strong seed isolation", () => {
        const s0 = RandomState.withSeeds(1 as u64, 2 as u64, 3 as u64, 4 as u64);
        const s1 = RandomState.withSeeds(9 as u64, 2 as u64, 3 as u64, 4 as u64);
        const s2 = RandomState.withSeeds(1 as u64, 9 as u64, 3 as u64, 4 as u64);
        const s3 = RandomState.withSeeds(1 as u64, 2 as u64, 9 as u64, 4 as u64);
        const s4 = RandomState.withSeeds(1 as u64, 2 as u64, 3 as u64, 9 as u64);

        const h0 = s0.hashOneBytes("same-input-key");
        expect_eq(h0 != s1.hashOneBytes("same-input-key"), true);
        expect_eq(h0 != s2.hashOneBytes("same-input-key"), true);
        expect_eq(h0 != s3.hashOneBytes("same-input-key"), true);
        expect_eq(h0 != s4.hashOneBytes("same-input-key"), true);
    });

    test("3. @cap/ahash — Streaming chunked writeBytes matches monolithic hash", () => {
        const msg = "Zero-allocation high-speed randomized hashing for CP language!";
        const rs = RandomState.withSeeds(11 as u64, 22 as u64, 33 as u64, 44 as u64);
        const whole = rs.hashOneBytes(msg);

        let h = rs.buildHasher();
        h.writeBytes(msg[0..5]);
        h.writeBytes(msg[5..14]);
        h.writeBytes(msg[14..30]);
        h.writeBytes(msg[30..msg.len]);
        expect_eq(h.finish(), whole);
    });

    test("4. @cap/ahash — Length boundary sweep (0..65 bytes)", () => {
        let buf: [65]u8 = undefined;
        let i: usize = 0;
        while (i < 65) {
            buf[i] = ('a' as u8) + ((i % (26 as usize)) as u8);
            i += 1;
        }
        const rs = RandomState.defaultState();
        let prev = rs.hashOneBytes(buf[0..0]);
        let n: usize = 1;
        while (n <= 65) {
            const cur = rs.hashOneBytes(buf[0..n]);
            expect_eq(cur != prev, true);
            prev = cur;
            n += 1;
        }
    });

    test("5. @cap/ahash — Single-bit flip avalanche effect", () => {
        const rs = RandomState.defaultState();
        const a = rs.hashOneBytes("ahash-avalanche-key-0");
        const b = rs.hashOneBytes("ahash-avalanche-key-1");
        const diffBits = popcount64(a ^ b);
        expect_eq(diffBits >= (16 as usize), true);

        const ua = rs.hashOneU64(0x10000000 as u64);
        const ub = rs.hashOneU64(0x10000001 as u64);
        expect_eq(popcount64(ua ^ ub) >= (16 as usize), true);
    });

    test("6. @cap/ahash — Mixed writeU8/writeU32/writeU64 & top-level hashBytes", () => {
        let h = AHasher.init(5 as u64, 6 as u64, 7 as u64, 8 as u64);
        h.writeU8(0xAB as u8);
        h.writeU32(0x12345678 as u32);
        h.writeU64(0x0102030405060708 as u64);
        const r1 = h.finish();

        let h2 = AHasher.init(5 as u64, 6 as u64, 7 as u64, 8 as u64);
        h2.writeU8(0xAB as u8);
        h2.writeU32(0x12345678 as u32);
        h2.writeU64(0x0102030405060708 as u64);
        expect_eq(r1, h2.finish());

        const b1 = hashBytes("top-level", 1 as u64, 2 as u64);
        const b2 = hashBytes("top-level", 1 as u64, 3 as u64);
        expect_eq(b1 != b2, true);
    });
}
