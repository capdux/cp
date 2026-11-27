// cap/test/testCapRot13.cp
// Comprehensive unit tests for @cap/rot13 (ROT13, ROT47, Caesar, Atbash)

import { test, expect_eq } from "cap:test";
import {
    rot13,
    rot13InPlace,
    rot47,
    caesar,
    atbash,
} from "../../packages/rot13-cipher/src/index.cp";

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
    test("1. @cap/rot13 — classic 'Hello, World!' -> 'Uryyb, Jbeyq!' & self-inverse", () => {
        let b1: [32]u8 = undefined;
        let b2: [32]u8 = undefined;
        const n1: usize = rot13("Hello, World!", b1[0..32]);
        expect_eq(sliceEq(b1[0..n1], "Uryyb, Jbeyq!"), true);

        const n2: usize = rot13(b1[0..n1], b2[0..32]);
        expect_eq(sliceEq(b2[0..n2], "Hello, World!"), true);
    });

    test("2. @cap/rot13 — rot13InPlace mutates buffer & is self-inverse", () => {
        let buf: [6]u8 = ['C' as u8, 'a' as u8, 'p' as u8, '4' as u8, '0' as u8, '0' as u8];
        rot13InPlace(buf[0..6]);
        expect_eq(sliceEq(buf[0..6], "Pnc400"), true);
        rot13InPlace(buf[0..6]);
        expect_eq(sliceEq(buf[0..6], "Cap400"), true);
    });

    test("3. @cap/rot13 — rot47 with digits & symbols is self-inverse", () => {
        const src = "Cap v1.0! @2026 #400";
        let b1: [64]u8 = undefined;
        let b2: [64]u8 = undefined;

        const n1: usize = rot47(src, b1[0..64]);
        expect_eq(n1, src.len);
        expect_eq(sliceEq(b1[0..n1], src), false);

        const n2: usize = rot47(b1[0..n1], b2[0..64]);
        expect_eq(sliceEq(b2[0..n2], src), true);
    });

    test("4. @cap/rot13 — caesar positive & negative shifts round-trip", () => {
        let b1: [32]u8 = undefined;
        let b2: [32]u8 = undefined;

        const n1: usize = caesar("Meet at Dawn!", 3, b1[0..32]);
        expect_eq(sliceEq(b1[0..n1], "Phhw dw Gdzq!"), true);

        const n2: usize = caesar(b1[0..n1], -3, b2[0..32]);
        expect_eq(sliceEq(b2[0..n2], "Meet at Dawn!"), true);

        // Large negative shift wraps cleanly (-29 == -3 mod 26)
        const n3: usize = caesar(b1[0..n1], -29, b2[0..32]);
        expect_eq(sliceEq(b2[0..n3], "Meet at Dawn!"), true);
    });

    test("5. @cap/rot13 — atbash 'AbcZ' -> 'ZyxA' & self-inverse", () => {
        let b1: [16]u8 = undefined;
        let b2: [16]u8 = undefined;

        const n1: usize = atbash("AbcZ-9", b1[0..16]);
        expect_eq(sliceEq(b1[0..n1], "ZyxA-9"), true);

        const n2: usize = atbash(b1[0..n1], b2[0..16]);
        expect_eq(sliceEq(b2[0..n2], "AbcZ-9"), true);
    });

    test("6. @cap/rot13 — empty input & tiny buffer protection", () => {
        let buf: [16]u8 = undefined;
        expect_eq(rot13("", buf[0..16]), 0 as usize);
        expect_eq(rot47("", buf[0..16]), 0 as usize);
        expect_eq(caesar("", 5, buf[0..16]), 0 as usize);
        expect_eq(atbash("", buf[0..16]), 0 as usize);

        let tiny: [3]u8 = undefined;
        expect_eq(rot13("Hello", tiny[0..3]), 0 as usize);
        expect_eq(rot47("Hello", tiny[0..3]), 0 as usize);
        expect_eq(caesar("Hello", 3, tiny[0..3]), 0 as usize);
        expect_eq(atbash("Hello", tiny[0..3]), 0 as usize);
    });
}
