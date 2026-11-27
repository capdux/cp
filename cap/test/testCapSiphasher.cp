import { test, expect_eq } from "cap:test";
import {
    sipHash24, sipHash13, sipHash24Key16, sipHash24Str
} from "../../packages/siphasher/src/index.cp";

fn makeU64(hi: u32, lo: u32): u64 {
    return ((hi as u64) << 32) | (lo as u64);
}

fn main() {
    test("1. @cap/siphasher — official SipHash-2-4 vector (len=0, key=00..0f)", () => {
        let key: [16]u8 = [0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15];
        let msg: [1]u8 = [0];
        // Official vector for len=0: 0x726fdb47dd0e0e31
        let expected = makeU64(1919933255, 3708685873);
        expect_eq(sipHash24Key16(key[0..16], msg[0..0]) ?? 0, expected);
    });

    test("2. @cap/siphasher — official SipHash-2-4 vector (len=15, msg=00..0e, key=00..0f)", () => {
        let key: [16]u8 = [0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15];
        let msg: [15]u8 = [0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14];
        // Official Aumasson-Bernstein paper vector for 15-byte message: 0xa129ca6149be45e5
        let expected = makeU64(2703870561, 1237206501);
        expect_eq(sipHash24Key16(key[0..16], msg[0..15]) ?? 0, expected);
    });

    test("3. @cap/siphasher — official SipHash-2-4 vector (len=1, msg=[0x00])", () => {
        let key: [16]u8 = [0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15];
        let msg: [1]u8 = [0];
        // Official vector for len=1: 0x74f839c593dc67fd
        let expected = makeU64(1962424773, 2480695293);
        expect_eq(sipHash24Key16(key[0..16], msg[0..1]) ?? 0, expected);
    });

    test("4. @cap/siphasher — SipHash-1-3 produces distinct keyed hash from SipHash-2-4", () => {
        let msg: [5]u8 = [104, 101, 108, 108, 111];
        let h24 = sipHash24(1, 2, msg[0..5]);
        let h13 = sipHash13(1, 2, msg[0..5]);
        expect_eq(h24 == h13, false);
        expect_eq(sipHash13(1, 2, msg[0..5]), h13);
    });

    test("5. @cap/siphasher — sipHash24Str sensitivity to key and input", () => {
        let a = sipHash24Str(10, 20, "cap-lang");
        let b = sipHash24Str(10, 21, "cap-lang");
        let c = sipHash24Str(10, 20, "cap-lanh");
        expect_eq(a == b, false);
        expect_eq(a == c, false);
    });

    test("6. @cap/siphasher — sipHash24Key16 rejects short key (< 16 bytes)", () => {
        let short_k: [8]u8 = [0; 8];
        let msg: [2]u8 = [1, 2];
        expect_eq(sipHash24Key16(short_k[0..8], msg[0..2]) == null, true);
    });
}
