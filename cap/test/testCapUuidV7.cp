import { test, expect_eq } from "cap:test";
import {
    UuidV7,
    fromParts,
    timestampMs,
    version,
    isRfcVariant,
    formatHyphenated,
    parse,
    isValidUuidV7,
    compare,
} from "../../packages/uuidv7/src/index.cp";

fn testEq(a: []const u8, b: []const u8) -> bool {
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
    test("1. @cap/uuid-v7 — RFC 9562 Appendix A.6 reference timestamp & version/variant bits", () => {
        // RFC 9562 A.6 example: 017f22e2-79b0-7cc3-98c4-dc0c0c07398f -> ts = 1645557742000 ms
        const rand: [10]u8 = [0xFC, 0xC3, 0xD8, 0xC4, 0xDC, 0x0C, 0x0C, 0x07, 0x39, 0x8F];
        const u = fromParts(1645557742000, rand);
        expect_eq(timestampMs(u), 1645557742000 as u64);
        expect_eq(version(u), 7 as u8);
        expect_eq(isRfcVariant(u), true);

        let buf: [40]u8 = undefined;
        const n = formatHyphenated(u, buf[0..40]);
        expect_eq(n, 36 as usize);
        expect_eq(testEq(buf[0..36], "017f22e2-79b0-7cc3-98c4-dc0c0c07398f"), true);
    });

    test("2. @cap/uuid-v7 — formatHyphenated 36-byte output & parse roundtrip", () => {
        const rand: [10]u8 = [0x01, 0x23, 0x45, 0x67, 0x89, 0xAB, 0xCD, 0xEF, 0x10, 0x20];
        const orig = fromParts(1764160000123, rand);

        let buf: [36]u8 = undefined;
        expect_eq(formatHyphenated(orig, buf[0..36]), 36 as usize);

        let parsed = UuidV7 { bytes: [0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0] };
        expect_eq(parse(buf[0..36], &parsed), true);
        expect_eq(compare(orig, parsed), 0 as i32);
        expect_eq(timestampMs(parsed), 1764160000123 as u64);
    });

    test("3. @cap/uuid-v7 — 32-char compact hex & uppercase parsing", () => {
        let parsed = UuidV7 { bytes: [0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0] };
        expect_eq(parse("017F22E279B07CC398C4DC0C0C07398F", &parsed), true);
        expect_eq(timestampMs(parsed), 1645557742000 as u64);
        expect_eq(version(parsed), 7 as u8);
        expect_eq(isValidUuidV7("017F22E2-79B0-7CC3-98C4-DC0C0C07398F"), true);
    });

    test("4. @cap/uuid-v7 — Monotonic chronological ordering via compare()", () => {
        const r: [10]u8 = [0, 0, 0, 0, 0, 0, 0, 0, 0, 0];
        const u1 = fromParts(1700000000000, r);
        const u2 = fromParts(1700000000001, r);
        const u3 = fromParts(1700000099999, r);

        expect_eq(compare(u1, u2), -1 as i32);
        expect_eq(compare(u2, u3), -1 as i32);
        expect_eq(compare(u3, u1), 1 as i32);
        expect_eq(compare(u2, u2), 0 as i32);
    });

    test("5. @cap/uuid-v7 — Rejects UUIDv4 (version != 7), bad variant & malformed input", () => {
        // UUIDv4 (version 4)
        expect_eq(isValidUuidV7("550e8400-e29b-41d4-a716-446655440000"), false);
        // Version 7 but invalid variant nibble (0x00 instead of 0x80..0xBF)
        expect_eq(isValidUuidV7("017f22e2-79b0-7cc3-08c4-dc0c0c07398f"), false);
        // Wrong length / misplaced hyphens / non-hex
        expect_eq(isValidUuidV7("017f22e2-79b0-7cc3-98c4-dc0c0c07398"), false);
        expect_eq(isValidUuidV7("017f22e279b0-7cc3-98c4-dc0c0c07398f"), false);
        expect_eq(isValidUuidV7("zzzzzzzz-79b0-7cc3-98c4-dc0c0c07398f"), false);
    });

    test("6. @cap/uuid-v7 — Undersized buffer returns 0", () => {
        const r: [10]u8 = [1, 2, 3, 4, 5, 6, 7, 8, 9, 10];
        const u = fromParts(1700000000000, r);
        let tiny: [35]u8 = undefined;
        expect_eq(formatHyphenated(u, tiny[0..35]), 0 as usize);
    });
}
