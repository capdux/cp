import { test, expect_eq } from "cap:test";
import {
    Ulid,
    encodeUlid,
    decodeUlid,
    isValidUlid,
} from "../../packages/@cap/ulid-crate/src/index.cp";

fn eqStr(a: string, b: string): bool {
    if (a.len != b.len) { return false; }
    let i: usize = 0;
    while (i < a.len) {
        if (a[i] != b[i]) { return false; }
        i += 1;
    }
    return true;
}

fn main() {
    test("1. @cap/ulid-crate — fromParts, timestampMs & Crockford Base32 spec vector", () => {
        // Official ULID spec timestamps:
        // 1469918176385 -> "01ARYZ6S41", 1469922850259 -> "01ARZ3NDEK"
        const u = Ulid.fromParts(1469918176385 as u64, 0 as u16, 0 as u64);
        expect_eq(u.timestampMs(), 1469918176385 as u64);

        let buf: [26]u8 = undefined;
        const n = encodeUlid(u, buf[0..]);
        expect_eq(n, 26 as usize);
        expect_eq(eqStr(buf[0..10], "01ARYZ6S41"), true);
        expect_eq(eqStr(buf[10..26], "0000000000000000"), true);

        const u2 = Ulid.fromParts(1469922850259 as u64, 0 as u16, 0 as u64);
        expect_eq(encodeUlid(u2, buf[0..]), 26 as usize);
        expect_eq(eqStr(buf[0..10], "01ARZ3NDEK"), true);
    });

    test("2. @cap/ulid-crate — encodeUlid & decodeUlid round-trip + min/max extremes", () => {
        const u = Ulid.fromParts(1700000000123 as u64, 0xABCD as u16, 0x0123456789ABCDEF as u64);
        let buf: [26]u8 = undefined;
        expect_eq(encodeUlid(u, buf[0..]), 26 as usize);

        const decoded = decodeUlid(buf[0..26]) ?? Ulid { hi: 0, lo: 0 };
        expect_eq(decoded.eq(u), true);
        expect_eq(decoded.timestampMs(), 1700000000123 as u64);

        const maxU = decodeUlid("7ZZZZZZZZZZZZZZZZZZZZZZZZZ") ?? Ulid { hi: 0, lo: 0 };
        expect_eq(maxU.hi, 0xFFFFFFFFFFFFFFFF as u64);
        expect_eq(maxU.lo, 0xFFFFFFFFFFFFFFFF as u64);

        const minU = decodeUlid("00000000000000000000000000") ?? Ulid { hi: 1, lo: 1 };
        expect_eq(minU.hi, 0 as u64);
        expect_eq(minU.lo, 0 as u64);
    });

    test("3. @cap/ulid-crate — Crockford case-insensitivity, I/L/O aliases & overflow rejection", () => {
        const canonical = decodeUlid("01ARZ3NDEK0000000000000010") ?? Ulid { hi: 0, lo: 0 };
        const aliased = decodeUlid("o1arz3ndekooooooooooooooio") ?? Ulid { hi: 1, lo: 1 };
        const aliasedL = decodeUlid("O1ARZ3NDEKOOOOOOOOOOOOOOLO") ?? Ulid { hi: 1, lo: 1 };
        expect_eq(canonical.eq(aliased), true);
        expect_eq(canonical.eq(aliasedL), true);

        // Overflow: first char '8' (> '7') exceeds 128 bits
        expect_eq(isValidUlid("80000000000000000000000000"), false);
        // Invalid char 'U' and wrong lengths
        expect_eq(isValidUlid("01ARZ3NDEKUUUUUUUUUUUUUUUU"), false);
        expect_eq(isValidUlid("01ARZ3NDEK"), false);
        expect_eq(isValidUlid("01ARZ3NDEK00000000000000000"), false);
        expect_eq(isValidUlid("01ARZ3NDEKTSV4RRFFQ69G5FAV"), true);
    });

    test("4. @cap/ulid-crate — monotonic increment carry & 80-bit overflow", () => {
        // Simple lo + 1
        const a = Ulid.fromParts(1000 as u64, 0x0001 as u16, 41 as u64);
        const b = a.increment() ?? Ulid { hi: 0, lo: 0 };
        expect_eq(b.timestampMs(), 1000 as u64);
        expect_eq(b.lo, 42 as u64);

        // Carry from lo == u64::MAX into rand_hi16
        const c = Ulid.fromParts(2000 as u64, 0x00FF as u16, 0xFFFFFFFFFFFFFFFF as u64);
        const d = c.increment() ?? Ulid { hi: 0, lo: 0 };
        expect_eq(d.timestampMs(), 2000 as u64);
        expect_eq(d.hi & (0xFFFF as u64), 0x0100 as u64);
        expect_eq(d.lo, 0 as u64);

        // 80-bit random overflow returns null
        const full = Ulid.fromParts(3000 as u64, 0xFFFF as u16, 0xFFFFFFFFFFFFFFFF as u64);
        expect_eq(full.increment() == null, true);
    });

    test("5. @cap/ulid-crate — 16-byte big-endian toBytes & fromBytes round-trip", () => {
        const u = Ulid.fromParts(0x01890A1B2C3D as u64, 0xBEEF as u16, 0xCAFEBABE11223344 as u64);
        let bytes: [16]u8 = undefined;
        expect_eq(u.toBytes(bytes[0..]), 16 as usize);
        expect_eq(bytes[0], 0x01 as u8);
        expect_eq(bytes[5], 0x3D as u8);
        expect_eq(bytes[6], 0xBE as u8);
        expect_eq(bytes[7], 0xEF as u8);
        expect_eq(bytes[15], 0x44 as u8);

        const back = Ulid.fromBytes(bytes[0..]) ?? Ulid { hi: 0, lo: 0 };
        expect_eq(back.eq(u), true);
        expect_eq(Ulid.fromBytes(bytes[0..8]) == null, true);
    });

    test("6. @cap/ulid-crate — cmp & lexicographic sort order matches numeric order", () => {
        const u1 = Ulid.fromParts(1000 as u64, 0x0001 as u16, 0x0001 as u64);
        const u2 = Ulid.fromParts(1000 as u64, 0x0001 as u16, 0x0002 as u64);
        const u3 = Ulid.fromParts(1001 as u64, 0x0000 as u16, 0x0000 as u64);

        expect_eq(u1.cmp(u2), -1 as i32);
        expect_eq(u2.cmp(u1), 1 as i32);
        expect_eq(u2.cmp(u2), 0 as i32);
        expect_eq(u2.cmp(u3), -1 as i32);

        let s1: [26]u8 = undefined;
        let s2: [26]u8 = undefined;
        let s3: [26]u8 = undefined;
        expect_eq(encodeUlid(u1, s1[0..]), 26 as usize);
        expect_eq(encodeUlid(u2, s2[0..]), 26 as usize);
        expect_eq(encodeUlid(u3, s3[0..]), 26 as usize);

        // Verify string encoding of u2 < u3 at timestamp prefix
        let diff_lt = false;
        let i: usize = 0;
        while (i < 26) {
            if (s2[i] != s3[i]) {
                diff_lt = s2[i] < s3[i];
                break;
            }
            i += 1;
        }
        expect_eq(diff_lt, true);
    });
}
