// cap/test/testCapUlid.cp
// Comprehensive unit tests for @cap/ulid

import { test, expect_eq } from "cap:test";
import {
    Ulid,
    fromParts,
    timestampMs,
    encode,
    decode,
    isValidUlid,
    increment,
    compare,
} from "../../packages/ulid/src/index.cp";

fn sliceEq(a: []const u8, b: []const u8): bool {
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

fn zeroUlid(): Ulid {
    let b: [16]u8 = undefined;
    let i: usize = 0;
    while (i < 16) {
        b[i] = 0 as u8;
        i += 1;
    }
    return Ulid{ bytes: b };
}

fn main() {
    test("1. @cap/ulid — fromParts / encode / decode / timestampMs full round-trip", () => {
        let rand: [10]u8 = undefined;
        let i: usize = 0;
        while (i < 10) {
            rand[i] = (0x10 + i * 7) as u8;
            i += 1;
        }
        const ts: u64 = 1700000000123 as u64;
        const u = fromParts(ts, rand);
        expect_eq(timestampMs(u), ts);

        let buf: [26]u8 = undefined;
        expect_eq(encode(u, buf[0..26]), 26 as usize);
        expect_eq(isValidUlid(buf[0..26]), true);

        let decoded = zeroUlid();
        expect_eq(decode(buf[0..26], &decoded), true);
        expect_eq(compare(u, decoded), 0 as i32);
        expect_eq(timestampMs(decoded), ts);
    });

    test("2. @cap/ulid — Lexicographical time ordering & compare", () => {
        let rand: [10]u8 = undefined;
        let i: usize = 0;
        while (i < 10) {
            rand[i] = 0x55 as u8;
            i += 1;
        }
        const u1 = fromParts(1000000 as u64, rand);
        const u2 = fromParts(2000000 as u64, rand);
        expect_eq(compare(u1, u2), -1 as i32);
        expect_eq(compare(u2, u1), 1 as i32);

        let s1: [26]u8 = undefined;
        let s2: [26]u8 = undefined;
        * = encode(u1, s1[0..26]);
        * = encode(u2, s2[0..26]);
        // Earlier timestamp must sort lexicographically smaller in Base32 too.
        let lt = false;
        let k: usize = 0;
        while (k < 26) {
            if (s1[k] != s2[k]) {
                lt = s1[k] < s2[k];
                break;
            }
            k += 1;
        }
        expect_eq(lt, true);
    });

    test("3. @cap/ulid — Crockford Base32 case-insensitivity & I/L/O tolerance", () => {
        let uCanon = zeroUlid();
        let uTolerant = zeroUlid();
        expect_eq(decode("01ARZ3NDEKTSV4RRFFQ69G5FAV", &uCanon), true);
        expect_eq(decode("o1arz3ndektsv4rrffq69g5fav", &uTolerant), true);
        expect_eq(compare(uCanon, uTolerant), 0 as i32);

        let uIl = zeroUlid();
        let uOnes = zeroUlid();
        expect_eq(decode("0IL00000000000000000000000", &uIl), true);
        expect_eq(decode("01100000000000000000000000", &uOnes), true);
        expect_eq(compare(uIl, uOnes), 0 as i32);
    });

    test("4. @cap/ulid — Same-millisecond monotonic increment with carry & overflow", () => {
        let rand: [10]u8 = undefined;
        let i: usize = 0;
        while (i < 9) {
            rand[i] = 0x00 as u8;
            i += 1;
        }
        rand[9] = 0xFF as u8;

        const u = fromParts(1700000000000 as u64, rand);
        let next = zeroUlid();
        expect_eq(increment(u, &next), true);
        expect_eq(timestampMs(next), timestampMs(u));
        expect_eq(next.bytes[14], 0x01 as u8);
        expect_eq(next.bytes[15], 0x00 as u8);
        expect_eq(compare(u, next), -1 as i32);

        // Overflow when all 80 entropy bits are 0xFF.
        let maxRand: [10]u8 = undefined;
        let j: usize = 0;
        while (j < 10) {
            maxRand[j] = 0xFF as u8;
            j += 1;
        }
        const uMaxEnt = fromParts(1700000000000 as u64, maxRand);
        expect_eq(increment(uMaxEnt, &next), false);
    });

    test("5. @cap/ulid — Reject overflow ('8...'), wrong length, invalid char 'U' & tiny buffer", () => {
        expect_eq(isValidUlid("80000000000000000000000000"), false);
        expect_eq(isValidUlid("01ARZ3NDEKTSV4RRFFQ69G5FA"), false);
        expect_eq(isValidUlid("01ARZ3NDEKTSV4RRFFQ69G5FAVX"), false);
        expect_eq(isValidUlid("01ARZ3NDEKTSV4RRFFQ69G5FAU"), false);
        expect_eq(isValidUlid("01ARZ3NDEKTSV4RRFFQ69G5FA!"), false);

        let tiny: [10]u8 = undefined;
        expect_eq(encode(zeroUlid(), tiny[0..10]), 0 as usize);
    });

    test("6. @cap/ulid — Min (all-zero) & Max (7ZZZZZZZZZZZZZZZZZZZZZZZZZ) boundaries", () => {
        const minStr = "00000000000000000000000000";
        let uMin = zeroUlid();
        expect_eq(decode(minStr, &uMin), true);
        expect_eq(timestampMs(uMin), 0 as u64);
        let encMin: [26]u8 = undefined;
        expect_eq(encode(uMin, encMin[0..26]), 26 as usize);
        expect_eq(sliceEq(encMin[0..26], minStr), true);

        const maxStr = "7ZZZZZZZZZZZZZZZZZZZZZZZZZ";
        let uMax = zeroUlid();
        expect_eq(decode(maxStr, &uMax), true);
        expect_eq(uMax.bytes[0], 0xFF as u8);
        expect_eq(uMax.bytes[15], 0xFF as u8);
        let encMax: [26]u8 = undefined;
        expect_eq(encode(uMax, encMax[0..26]), 26 as usize);
        expect_eq(sliceEq(encMax[0..26], maxStr), true);
    });
}
