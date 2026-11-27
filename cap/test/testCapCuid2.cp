import { test, expect_eq } from "cap:test";
import {
    Cuid2Generator, isCuid2, isCuid2WithBounds, CUID2_DEFAULT_LEN
} from "../../packages/@paralleldrive/cuid2/src/index.cp";

fn eqBuf(a: []const u8, b: []const u8, len: usize): bool {
    let i: usize = 0;
    while (i < len) {
        if (a[i] != b[i]) { return false; }
        i = i + 1;
    }
    return true;
}

fn main() {
    test("1. @cap/cuid2 — Generates valid 24-char CUID2 starting with lowercase letter", () => {
        let gen = Cuid2Generator.new(42, "node-1", CUID2_DEFAULT_LEN);
        let out: [32]u8 = undefined;
        let n = gen.next(1700000000000, out[0..32]);
        expect_eq(n, 24);
        expect_eq(isCuid2(out[0..n] as string), true);
        expect_eq(out[0] >= 97 && out[0] <= 122, true);
    });

    test("2. @cap/cuid2 — Consecutive IDs at same timestamp are unique", () => {
        let gen = Cuid2Generator.new(999, "worker-a", 24);
        let a: [32]u8 = undefined;
        let b: [32]u8 = undefined;
        let c: [32]u8 = undefined;
        gen.next(1700000000000, a[0..32]);
        gen.next(1700000000000, b[0..32]);
        gen.next(1700000000000, c[0..32]);
        expect_eq(eqBuf(a[0..24], b[0..24], 24), false);
        expect_eq(eqBuf(b[0..24], c[0..24], 24), false);
        expect_eq(eqBuf(a[0..24], c[0..24], 24), false);
    });

    test("3. @cap/cuid2 — Different host fingerprints produce distinct IDs", () => {
        let g1 = Cuid2Generator.new(123, "host-alpha", 24);
        let g2 = Cuid2Generator.new(123, "host-beta", 24);
        let a: [32]u8 = undefined;
        let b: [32]u8 = undefined;
        g1.next(1700000000000, a[0..32]);
        g2.next(1700000000000, b[0..32]);
        expect_eq(eqBuf(a[0..24], b[0..24], 24), false);
    });

    test("4. @cap/cuid2 — Custom length clamping (min 2, max 32)", () => {
        let g_short = Cuid2Generator.new(1, "fp", 1);
        let g_long = Cuid2Generator.new(1, "fp", 100);
        expect_eq(g_short.length(), 2);
        expect_eq(g_long.length(), 32);
        let out: [32]u8 = undefined;
        let n = g_long.next(1000, out[0..32]);
        expect_eq(n, 32);
        expect_eq(isCuid2(out[0..n] as string), true);
    });

    test("5. @cap/cuid2 — isCuid2 validation accepts valid & rejects invalid strings", () => {
        expect_eq(isCuid2("tz4a98xxat96iws9zmbrgj3a"), true);
        expect_eq(isCuid2("1abcde"), false); // starts with digit
        expect_eq(isCuid2("Abcde"), false);  // uppercase
        expect_eq(isCuid2("ab-cd"), false);  // hyphen not allowed
        expect_eq(isCuid2("a"), false);      // too short
        expect_eq(isCuid2WithBounds("abcd", 4, 8), true);
        expect_eq(isCuid2WithBounds("abc", 4, 8), false);
    });

    test("6. @cap/cuid2 — Buffer too small returns 0 safely", () => {
        let gen = Cuid2Generator.new(7, "fp", 24);
        let tiny: [8]u8 = undefined;
        expect_eq(gen.next(1000, tiny[0..8]), 0);
    });
}
