// cap/test/testCapKsuid.cp
// Comprehensive unit tests for @cap/ksuid (Segment KSUID official vectors)

import { test, expect_eq } from "cap:test";
import {
    KSUID_EPOCH,
    Ksuid,
    fromParts,
    timestampSecs,
    encode,
    decode,
    isValidKsuid,
    compare,
} from "../../packages/ksuid/src/index.cp";

fn sliceEq(a: []const u8, b: []const u8) -> bool {
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

fn main() {
    test("1. @cap/ksuid — all-zero KSUID encodes to 27 '0's", () => {
        const zero_payload: [16]u8 = [0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0];
        const k: Ksuid = fromParts(KSUID_EPOCH, zero_payload);
        let out: [27]u8 = undefined;
        const n: usize = encode(k, out[0..27]);
        expect_eq(n, 27 as usize);
        expect_eq(sliceEq(out[0..27], "000000000000000000000000000"), true);
        expect_eq(timestampSecs(k), KSUID_EPOCH);
    });

    test("2. @cap/ksuid — max all-0xFF KSUID matches official Segment vector", () => {
        let max_k = Ksuid { bytes: undefined };
        let i: usize = 0;
        while (i < 20) {
            max_k.bytes[i] = 0xFF;
            i += 1;
        }
        let out: [27]u8 = undefined;
        const n: usize = encode(max_k, out[0..27]);
        expect_eq(n, 27 as usize);
        expect_eq(sliceEq(out[0..27], "aWgEPTl1tmebfsQzFP4bxwgy80V"), true);

        let dec_k: Ksuid = undefined;
        expect_eq(decode("aWgEPTl1tmebfsQzFP4bxwgy80V", &dec_k), true);
        expect_eq(compare(max_k, dec_k), 0 as i32);
    });

    test("3. @cap/ksuid — fromParts / encode / decode / timestampSecs round-trip", () => {
        const payload: [16]u8 = [
            0xDE, 0xAD, 0xBE, 0xEF, 0x01, 0x23, 0x45, 0x67,
            0x89, 0xAB, 0xCD, 0xEF, 0xFE, 0xDC, 0xBA, 0x98,
        ];
        const ts: u64 = 1732620000;
        const k: Ksuid = fromParts(ts, payload);
        expect_eq(timestampSecs(k), ts);

        let out: [27]u8 = undefined;
        const n: usize = encode(k, out[0..27]);
        expect_eq(n, 27 as usize);
        expect_eq(isValidKsuid(out[0..27]), true);

        let back: Ksuid = undefined;
        expect_eq(decode(out[0..27], &back), true);
        expect_eq(compare(k, back), 0 as i32);
        expect_eq(timestampSecs(back), ts);
    });

    test("4. @cap/ksuid — chronological K-sortable compare & ASCII ordering", () => {
        const p: [16]u8 = [0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1];
        const k1: Ksuid = fromParts(1700000000, p);
        const k2: Ksuid = fromParts(1700000010, p);
        expect_eq(compare(k1, k2), -1 as i32);
        expect_eq(compare(k2, k1), 1 as i32);

        let s1: [27]u8 = undefined;
        let s2: [27]u8 = undefined;
        * = encode(k1, s1[0..27]);
        * = encode(k2, s2[0..27]);
        // Verify ASCII lexicographical order matches chronological order
        let cmp: i32 = 0;
        let i: usize = 0;
        while (i < 27) {
            if (s1[i] < s2[i]) {
                cmp = -1;
                break;
            }
            if (s1[i] > s2[i]) {
                cmp = 1;
                break;
            }
            i += 1;
        }
        expect_eq(cmp, -1 as i32);
    });

    test("5. @cap/ksuid — rejects >2^160-1 overflow, wrong length & invalid Base62", () => {
        expect_eq(isValidKsuid("zzzzzzzzzzzzzzzzzzzzzzzzzzz"), false);
        // Max valid is aWgEPTl1tmebfsQzFP4bxwgy80V; +1 (80W) overflows 160 bits
        expect_eq(isValidKsuid("aWgEPTl1tmebfsQzFP4bxwgy80W"), false);
        expect_eq(isValidKsuid("too_short"), false);
        expect_eq(isValidKsuid("00000000000000000000000000!"), false);
    });

    test("6. @cap/ksuid — tiny buffer protection & pre-epoch clamping", () => {
        const p: [16]u8 = [0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0];
        const pre: Ksuid = fromParts(1000, p);
        expect_eq(timestampSecs(pre), KSUID_EPOCH);

        let tiny: [16]u8 = undefined;
        expect_eq(encode(pre, tiny[0..16]), 0 as usize);
    });
}
