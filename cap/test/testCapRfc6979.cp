// cap/test/testCapRfc6979.cp
// Comprehensive unit tests for @cap/rfc6979

import { test, expect_eq } from "cap:test";
import {
    HmacDrbg256,
    generateK
} from "../../packages/@cap/rfc6979/src/index.cp";

fn bytesEq32(a: []u8, b: []u8): bool {
    let i: usize = 0;
    while (i < 32) {
        if (a[i] != b[i]) {
            return false;
        }
        i = i + 1;
    }
    return true;
}

fn cmpBe32(a: []u8, b: []u8): i32 {
    let i: usize = 0;
    while (i < 32) {
        if (a[i] < b[i]) { return -1; }
        if (a[i] > b[i]) { return 1; }
        i = i + 1;
    }
    return 0;
}

fn isNonZero32(a: []u8): bool {
    let i: usize = 0;
    while (i < 32) {
        if (a[i] != 0) {
            return true;
        }
        i = i + 1;
    }
    return false;
}

fn makeP256Order(out: []u8) {
    // NIST P-256 curve order q = FFFFFFFF00000000FFFFFFFFFFFFFFFFBCE6FAADA7179E84F3B9CAC2FC632551
    let q: [32]u8 = [
        0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00,
        0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
        0xBC, 0xE6, 0xFA, 0xAD, 0xA7, 0x17, 0x9E, 0x84,
        0xF3, 0xB9, 0xCA, 0xC2, 0xFC, 0x63, 0x25, 0x51
    ];
    let i: usize = 0;
    while (i < 32) {
        out[i] = q[i];
        i = i + 1;
    }
}

fn main() {
    test("1. @cap/rfc6979 — Deterministic repeat generation produces identical k", () => {
        let sk: [32]u8 = [
            0xC9, 0xAF, 0xA9, 0xD8, 0x45, 0xBA, 0x75, 0x16,
            0x6B, 0x5C, 0x21, 0x57, 0x67, 0xB1, 0xD6, 0x93,
            0x4E, 0x50, 0xC3, 0xDB, 0x36, 0xE8, 0x9B, 0x12,
            0x7B, 0x8A, 0x62, 0x2B, 0x12, 0x0F, 0x67, 0x21
        ];
        // SHA-256("sample")
        let h: [32]u8 = [
            0xAF, 0x2B, 0xDB, 0xE1, 0xAA, 0x9B, 0x6E, 0xC1,
            0xE2, 0xAD, 0xE1, 0xD6, 0x94, 0xF4, 0x1F, 0xC7,
            0x1A, 0x83, 0x1D, 0x02, 0x68, 0xE9, 0x89, 0x15,
            0x62, 0x11, 0x3D, 0x8A, 0x62, 0xAD, 0xD1, 0xBF
        ];
        let q: [32]u8 = undefined;
        makeP256Order(q[0..32]);

        let k1: [32]u8 = undefined;
        let k2: [32]u8 = undefined;
        expect_eq(generateK(sk[0..32], h[0..32], q[0..32], k1[0..32]), true);
        expect_eq(generateK(sk[0..32], h[0..32], q[0..32], k2[0..32]), true);
        expect_eq(bytesEq32(k1[0..32], k2[0..32]), true);

        // RFC 6979 Appendix A.2.5 official P-256 / SHA-256 / "sample" vector:
        // k = A6E3C57DD01ABE90086538398355DD4C3B17AA873382B0F24D6129493D8AAD60
        let expected_k: [32]u8 = [
            0xA6, 0xE3, 0xC5, 0x7D, 0xD0, 0x1A, 0xBE, 0x90,
            0x08, 0x65, 0x38, 0x39, 0x83, 0x55, 0xDD, 0x4C,
            0x3B, 0x17, 0xAA, 0x87, 0x33, 0x82, 0xB0, 0xF2,
            0x4D, 0x61, 0x29, 0x49, 0x3D, 0x8A, 0xAD, 0x60
        ];
        expect_eq(bytesEq32(k1[0..32], expected_k[0..32]), true);
    });

    test("2. @cap/rfc6979 — Single-bit private key change produces completely different k", () => {
        let sk1: [32]u8 = undefined;
        let sk2: [32]u8 = undefined;
        let h: [32]u8 = undefined;
        let i: usize = 0;
        while (i < 32) {
            sk1[i] = (i + 1) as u8;
            sk2[i] = (i + 1) as u8;
            h[i] = (0xA0 + i) as u8;
            i = i + 1;
        }
        sk2[31] = sk2[31] ^ 0x01; // flip 1 bit

        let q: [32]u8 = undefined;
        makeP256Order(q[0..32]);

        let k1: [32]u8 = undefined;
        let k2: [32]u8 = undefined;
        expect_eq(generateK(sk1[0..32], h[0..32], q[0..32], k1[0..32]), true);
        expect_eq(generateK(sk2[0..32], h[0..32], q[0..32], k2[0..32]), true);
        expect_eq(bytesEq32(k1[0..32], k2[0..32]), false);
    });

    test("3. @cap/rfc6979 — Different message hash produces different k", () => {
        let sk: [32]u8 = undefined;
        let h1: [32]u8 = undefined;
        let h2: [32]u8 = undefined;
        let i: usize = 0;
        while (i < 32) {
            sk[i] = 0x55;
            h1[i] = i as u8;
            h2[i] = i as u8;
            i = i + 1;
        }
        h2[0] = 0xFF;

        let q: [32]u8 = undefined;
        makeP256Order(q[0..32]);

        let k1: [32]u8 = undefined;
        let k2: [32]u8 = undefined;
        expect_eq(generateK(sk[0..32], h1[0..32], q[0..32], k1[0..32]), true);
        expect_eq(generateK(sk[0..32], h2[0..32], q[0..32], k2[0..32]), true);
        expect_eq(bytesEq32(k1[0..32], k2[0..32]), false);
    });

    test("4. @cap/rfc6979 — Extra data (RFC 6979 Section 3.6) alters DRBG output", () => {
        let sk: [32]u8 = undefined;
        let h: [32]u8 = undefined;
        let i: usize = 0;
        while (i < 32) {
            sk[i] = 0x11;
            h[i] = 0x22;
            i = i + 1;
        }
        let ex0: [1]u8 = [0];
        let ex1: [4]u8 = [0xDE, 0xAD, 0xBE, 0xEF];
        let ex2: [4]u8 = [0xDE, 0xAD, 0xBE, 0xEE];

        let d0 = HmacDrbg256.init(sk[0..32], h[0..32], ex0[0..0]);
        let d1 = HmacDrbg256.init(sk[0..32], h[0..32], ex1[0..4]);
        let d2 = HmacDrbg256.init(sk[0..32], h[0..32], ex2[0..4]);

        let o0: [32]u8 = undefined;
        let o1: [32]u8 = undefined;
        let o2: [32]u8 = undefined;
        expect_eq(d0.nextBytes(o0[0..32]), 32 as usize);
        expect_eq(d1.nextBytes(o1[0..32]), 32 as usize);
        expect_eq(d2.nextBytes(o2[0..32]), 32 as usize);

        expect_eq(bytesEq32(o0[0..32], o1[0..32]), false);
        expect_eq(bytesEq32(o1[0..32], o2[0..32]), false);
    });

    test("5. @cap/rfc6979 — Multi-round nextBytes advances (K, V) without repetition", () => {
        let sk: [32]u8 = undefined;
        let h: [32]u8 = undefined;
        let i: usize = 0;
        while (i < 32) {
            sk[i] = 0x33;
            h[i] = 0x44;
            i = i + 1;
        }
        let empty: [1]u8 = [0];
        let drbg = HmacDrbg256.init(sk[0..32], h[0..32], empty[0..0]);

        let b1: [32]u8 = undefined;
        let b2: [32]u8 = undefined;
        let b3: [32]u8 = undefined;
        expect_eq(drbg.nextBytes(b1[0..32]), 32 as usize);
        expect_eq(drbg.nextBytes(b2[0..32]), 32 as usize);
        expect_eq(drbg.nextBytes(b3[0..32]), 32 as usize);

        expect_eq(bytesEq32(b1[0..32], b2[0..32]), false);
        expect_eq(bytesEq32(b2[0..32], b3[0..32]), false);
        expect_eq(bytesEq32(b1[0..32], b3[0..32]), false);
    });

    test("6. @cap/rfc6979 — generateK satisfies 1 <= k < P-256 order & rejects invalid lengths", () => {
        let sk: [32]u8 = undefined;
        let h: [32]u8 = undefined;
        let i: usize = 0;
        while (i < 32) {
            sk[i] = 0x77;
            h[i] = 0xFF; // >= P-256 order, exercises bits2octets reduction!
            i = i + 1;
        }
        let q: [32]u8 = undefined;
        makeP256Order(q[0..32]);

        let k: [32]u8 = undefined;
        expect_eq(generateK(sk[0..32], h[0..32], q[0..32], k[0..32]), true);
        expect_eq(isNonZero32(k[0..32]), true);
        expect_eq(cmpBe32(k[0..32], q[0..32]) < 0, true);

        // Invalid lengths & zero order safely return false
        expect_eq(generateK(sk[0..16], h[0..32], q[0..32], k[0..32]), false);
        expect_eq(generateK(sk[0..32], h[0..16], q[0..32], k[0..32]), false);
        expect_eq(generateK(sk[0..32], h[0..32], q[0..16], k[0..32]), false);
        expect_eq(generateK(sk[0..32], h[0..32], q[0..32], k[0..16]), false);

        let zero_q: [32]u8 = undefined;
        i = 0;
        while (i < 32) {
            zero_q[i] = 0;
            i = i + 1;
        }
        expect_eq(generateK(sk[0..32], h[0..32], zero_q[0..32], k[0..32]), false);
    });
}
