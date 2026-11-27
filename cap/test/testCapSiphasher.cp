// cap/test/testCapSiphasher.cp
// Comprehensive unit tests for @cap/siphasher (SipHash-2-4 & SipHash-1-3)

import { test, expect_eq } from "cap:test";
import {
    SipHasher24,
    SipHasher13,
    siphash24,
    siphash13,
} from "../../packages/siphasher/src/index.cp";

fn shPair(hi: u32, lo: u32): u64 {
    return ((hi as u64) << 32) | (lo as u64);
}

fn main() {
    test("1. @cap/siphasher — SipHash-2-4 official vectors & streaming equivalence", () => {
        const k0 = shPair(0x07060504, 0x03020100);
        const k1 = shPair(0x0f0e0d0c, 0x0b0a0908);

        expect_eq(siphash24(k0, k1, ""), shPair(0x726fdb47, 0xdd0e0e31));

        let h = SipHasher24.init(k0, k1);
        h.write("hello ");
        h.write("world");
        expect_eq(h.finish(), siphash24(k0, k1, "hello world"));
    });

    test("2. @cap/siphasher — SipHash-1-3 streaming equivalence & key sensitivity", () => {
        const k0 = shPair(0x07060504, 0x03020100);
        const k1 = shPair(0x0f0e0d0c, 0x0b0a0908);

        let h = SipHasher13.init(k0, k1);
        h.write("abc");
        h.write("def");
        expect_eq(h.finish(), siphash13(k0, k1, "abcdef"));
        expect_eq(siphash13(k0, k1, "abcdef") != siphash13(k0 + 1, k1, "abcdef"), true);
    });
}
