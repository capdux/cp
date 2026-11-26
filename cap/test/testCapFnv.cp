// cap/test/testCapFnv.cp
// Comprehensive unit tests for @cap/fnv

import { test, expect_eq } from "cap:test";
import {
    FNV32_OFFSET_BASIS,
    FNV64_OFFSET_BASIS,
    Fnv32Hasher,
    Fnv64Hasher,
    FxHasher,
    fnv1a32,
    fnv1a64,
    fxHash64,
} from "../../packages/@cap/fnv/src/index.cp";

fn u64Pair(hi: u32, lo: u32) -> u64 {
    return ((hi as u64) << 32) | (lo as u64);
}

fn popcount64(x: u64) -> u32 {
    let v = x;
    let count: u32 = 0;
    while (v != 0) {
        count += (v & 1) as u32;
        v >>= 1;
    }
    return count;
}

fn main() {
    test("1. @cap/fnv — FNV-1a 32-bit official IETF test vectors", () => {
        expect_eq(fnv1a32(""), 0x811c9dc5 as u32);
        expect_eq(fnv1a32("a"), 0xe40c292c as u32);
        expect_eq(fnv1a32("foobar"), 0xbf9cf968 as u32);
        expect_eq(FNV32_OFFSET_BASIS, 0x811c9dc5 as u32);
    });

    test("2. @cap/fnv — FNV-1a 64-bit official IETF test vectors", () => {
        expect_eq(fnv1a64(""), u64Pair(0xcbf29ce4, 0x84222325));
        expect_eq(fnv1a64("a"), u64Pair(0xaf63dc4c, 0x8601ec8c));
        expect_eq(fnv1a64("foobar"), u64Pair(0x85944171, 0xf73967e8));
        expect_eq(FNV64_OFFSET_BASIS, u64Pair(0xcbf29ce4, 0x84222325));
    });

    test("3. @cap/fnv — Streaming chunked hashing matches one-shot FNV-1a 32 & 64", () => {
        let h32 = Fnv32Hasher.init();
        h32.write("foo");
        h32.writeByte('b');
        h32.write("ar");
        expect_eq(h32.finish(), fnv1a32("foobar"));

        let h64 = Fnv64Hasher.init();
        h64.write("foo");
        h64.writeByte('b');
        h64.write("ar");
        expect_eq(h64.finish(), fnv1a64("foobar"));
    });

    test("4. @cap/fnv — FxHash 64-bit determinism, seeding & chunk lengths", () => {
        expect_eq(fxHash64(""), 0 as u64);
        expect_eq(fxHash64("hello world"), fxHash64("hello world"));
        expect_eq(fxHash64("hello world") != fxHash64("hello worle"), true);

        // Exercise 8-byte, 4-byte, 2-byte, and 1-byte absorption paths (15 bytes = 8+4+2+1)
        const h15 = fxHash64("1234567890abcde");
        expect_eq(h15 != 0 as u64, true);

        let s1 = FxHasher.withSeed(1);
        let s2 = FxHasher.withSeed(2);
        s1.writeBytes("same-key");
        s2.writeBytes("same-key");
        expect_eq(s1.finish() != s2.finish(), true);
    });

    test("5. @cap/fnv — FxHash writeU64 avalanche & low collision on sequential keys", () => {
        let h_a = FxHasher.init();
        h_a.writeU64(1000);
        h_a.writeU64(2000);

        let h_b = FxHasher.init();
        h_b.writeU64(1000);
        h_b.writeU64(2001);

        const diff_bits = popcount64(h_a.finish() ^ h_b.finish());
        // Single-bit input change should flip many output bits (strong avalanche)
        expect_eq(diff_bits >= (16 as u32), true);
    });

    test("6. @cap/fnv — Distinctness across short similar strings", () => {
        const a = fnv1a64("key_01");
        const b = fnv1a64("key_02");
        const c = fnv1a64("key_10");
        expect_eq(a != b, true);
        expect_eq(b != c, true);
        expect_eq(a != c, true);

        const fa = fxHash64("key_01");
        const fb = fxHash64("key_02");
        const fc = fxHash64("key_10");
        expect_eq(fa != fb, true);
        expect_eq(fb != fc, true);
        expect_eq(fa != fc, true);
    });
}
