import { test, expect_eq } from "cap:test";
import {
    ShortIdGenerator, isValidShortId, SHORTID_MIN_LEN, SHORTID_MAX_LEN
} from "../../packages/shortid/src/index.cp";

fn bytesEq(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) { return false; }
    let i: usize = 0;
    while (i < a.len) {
        if (a[i] != b[i]) { return false; }
        i = i + 1;
    }
    return true;
}

fn main() {
    test("1. @cap/shortid — Generates valid URL-safe short IDs", () => {
        let gen = ShortIdGenerator.new(12345, 3);
        let buf: [32]u8 = undefined;
        let n = gen.generate(1700000000000, buf[0..32]);
        expect_eq(n >= SHORTID_MIN_LEN && n <= SHORTID_MAX_LEN, true);
        expect_eq(isValidShortId(buf[0..n]), true);
        expect_eq(gen.isValid(buf[0..n]), true);
    });

    test("2. @cap/shortid — Embeds and decodes worker ID and version", () => {
        let gen = ShortIdGenerator.new(999, 11);
        let buf: [32]u8 = undefined;
        let n = gen.generate(1700000001000, buf[0..32]);
        expect_eq(gen.decodeWorker(buf[0..n]), 11);
        expect_eq(gen.decodeVersion(buf[0..n]), 6);
    });

    test("3. @cap/shortid — Consecutive IDs within same second are unique", () => {
        let gen = ShortIdGenerator.new(777, 1);
        let b1: [32]u8 = undefined;
        let b2: [32]u8 = undefined;
        let b3: [32]u8 = undefined;
        let n1 = gen.generate(1700000005000, b1[0..32]);
        let n2 = gen.generate(1700000005000, b2[0..32]);
        let n3 = gen.generate(1700000005000, b3[0..32]);
        expect_eq(bytesEq(b1[0..n1], b2[0..n2]), false);
        expect_eq(bytesEq(b2[0..n2], b3[0..n3]), false);
        expect_eq(bytesEq(b1[0..n1], b3[0..n3]), false);
    });

    test("4. @cap/shortid — Deterministic seed produces identical sequence", () => {
        let g1 = ShortIdGenerator.new(42, 5);
        let g2 = ShortIdGenerator.new(42, 5);
        let a: [32]u8 = undefined;
        let b: [32]u8 = undefined;
        let na = g1.generate(1700000010000, a[0..32]);
        let nb = g2.generate(1700000010000, b[0..32]);
        expect_eq(bytesEq(a[0..na], b[0..nb]), true);
    });

    test("5. @cap/shortid — Different seeds yield distinct outputs", () => {
        let g1 = ShortIdGenerator.new(100, 2);
        let g2 = ShortIdGenerator.new(200, 2);
        let a: [32]u8 = undefined;
        let b: [32]u8 = undefined;
        let na = g1.generate(1700000010000, a[0..32]);
        let nb = g2.generate(1700000010000, b[0..32]);
        expect_eq(bytesEq(a[0..na], b[0..nb]), false);
    });

    test("6. @cap/shortid — Rejects invalid lengths and illegal characters", () => {
        expect_eq(isValidShortId(""), false);
        expect_eq(isValidShortId("abc123"), false);
        expect_eq(isValidShortId("123456789012345"), false);
        expect_eq(isValidShortId("abc+def/12"), false);
        expect_eq(isValidShortId("PPBqWA9"), true);
        expect_eq(isValidShortId("dBvJIh-H"), true);
    });
}
