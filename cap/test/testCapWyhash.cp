import { test, expect_eq } from "cap:test";
import { wymum, wyhash, wyhashBytes, WyRng } from "../../packages/@cap/wyhash/src/index.cp";

fn popcount64(x: u64) -> u32 {
    let v = x;
    let c: u32 = 0;
    while (v != 0) {
        v &= v - 1;
        c += 1;
    }
    return c;
}

fn main() {
    test("1. @cap/wyhash — Determinism across empty, 1..3B, 4..16B, 17..48B & >64B inputs", () => {
        const s0 = "";
        const s1 = "ab";
        const s2 = "hello world";
        const s3 = "this is a medium length string for wyhash!";
        const s4 = "this is a much longer string exceeding sixty four bytes to exercise the 48-byte parallel lane loop in wyhash v4!";

        expect_eq(wyhash(s0, 0), wyhash(s0, 0));
        expect_eq(wyhash(s1, 0), wyhash(s1, 0));
        expect_eq(wyhash(s2, 0), wyhash(s2, 0));
        expect_eq(wyhash(s3, 0), wyhash(s3, 0));
        expect_eq(wyhash(s4, 0), wyhash(s4, 0));

        // All distinct inputs produce distinct 64-bit hashes
        expect_eq(wyhash(s0, 0) != wyhash(s1, 0), true);
        expect_eq(wyhash(s1, 0) != wyhash(s2, 0), true);
        expect_eq(wyhash(s2, 0) != wyhash(s3, 0), true);
        expect_eq(wyhash(s3, 0) != wyhash(s4, 0), true);
    });

    test("2. @cap/wyhash — Different seeds produce strong avalanche", () => {
        const h1 = wyhash("cp-lang wyhash", 0);
        const h2 = wyhash("cp-lang wyhash", 1);
        const h3 = wyhash("cp-lang wyhash", 42);

        expect_eq(h1 != h2, true);
        expect_eq(h2 != h3, true);
        expect_eq(popcount64(h1 ^ h2) >= 16, true);
    });

    test("3. @cap/wyhash — Single-byte flip avalanche on short & long inputs", () => {
        const h_a = wyhash("hello world!", 0);
        const h_b = wyhash("hello world?", 0);
        expect_eq(h_a != h_b, true);
        expect_eq(popcount64(h_a ^ h_b) >= 16, true);

        let long_a: [80]u8 = undefined;
        let long_b: [80]u8 = undefined;
        let i: usize = 0;
        while (i < 80) {
            long_a[i] = (i & 0xFF) as u8;
            long_b[i] = (i & 0xFF) as u8;
            i += 1;
        }
        long_b[55] ^= 0x01;
        const hl_a = wyhashBytes(long_a[0..80], 123);
        const hl_b = wyhashBytes(long_b[0..80], 123);
        expect_eq(hl_a != hl_b, true);
        expect_eq(popcount64(hl_a ^ hl_b) >= 16, true);
    });

    test("4. @cap/wyhash — wyhash & wyhashBytes exact consistency + wymum non-overflow", () => {
        const msg = "consistent bytes";
        expect_eq(wyhash(msg, 999), wyhashBytes(msg, 999));

        const max64: u64 = ((0xFFFFFFFF as u64) << 32) | (0xFFFFFFFF as u64);
        // (2^64 - 1)^2 = (2^64 - 2) * 2^64 + 1 -> hi = 2^64 - 2, lo = 1 -> lo ^ hi = 2^64 - 1
        expect_eq(wymum(max64, max64), max64);
        expect_eq(wymum(0, max64), 0 as u64);
    });

    test("5. @cap/wyhash — WyRng deterministic sequence & nextU32", () => {
        let r1 = WyRng.new(12345);
        let r2 = WyRng.new(12345);
        let i: usize = 0;
        while (i < 30) {
            expect_eq(r1.nextU64(), r2.nextU64());
            i += 1;
        }
        const u32_a = r1.nextU32();
        const u32_b = r2.nextU32();
        expect_eq(u32_a, u32_b);
    });

    test("6. @cap/wyhash — WyRng nextF64 1000 samples in [0.0, 1.0) with mean ~0.5", () => {
        let rng = WyRng.new(2026);
        let sum: f64 = 0.0;
        let i: usize = 0;
        while (i < 1000) {
            const v = rng.nextF64();
            expect_eq(v >= 0.0 && v < 1.0, true);
            sum += v;
            i += 1;
        }
        const mean = sum / 1000.0;
        expect_eq(mean > 0.45 && mean < 0.55, true);
    });
}
