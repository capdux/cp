// cap/test/testCapPoly1305.cp
// Comprehensive unit tests for @cap/poly1305 (RFC 8439 Section 2.5.2 official vector)

import { test, expect_eq } from "cap:test";
import {
    POLY1305_KEY_LEN,
    POLY1305_TAG_LEN,
    Poly1305,
    poly1305Mac,
    poly1305Verify
} from "../../packages/poly1305/src/index.cp";

fn bytesEq(a: []u8, b: []u8): bool {
    if (a.len != b.len) {
        return false;
    }
    let i: usize = 0;
    while (i < a.len) {
        if (a[i] != b[i]) {
            return false;
        }
        i = i + 1;
    }
    return true;
}

fn fillRfcKey(out: []u8) {
    let k: [32]u8 = [
        0x85, 0xd6, 0xbe, 0x78, 0x57, 0x55, 0x6d, 0x33,
        0x7f, 0x44, 0x52, 0xfe, 0x42, 0xd5, 0x06, 0xa8,
        0x01, 0x03, 0x80, 0x8a, 0xfb, 0x0d, 0xb2, 0xfd,
        0x4a, 0xbf, 0xf6, 0xaf, 0x41, 0x49, 0xf5, 0x1b
    ];
    let i: usize = 0;
    while (i < 32) {
        out[i] = k[i];
        i = i + 1;
    }
}

fn main() {
    test("1. @cap/poly1305 — RFC 8439 Section 2.5.2 official test vector", () => {
        let key: [32]u8 = undefined;
        fillRfcKey(key[0..32]);

        let msg_str = "Cryptographic Forum Research Group";
        let msg: [34]u8 = undefined;
        let i: usize = 0;
        while (i < 34) {
            msg[i] = msg_str[i];
            i = i + 1;
        }

        let tag: [16]u8 = undefined;
        expect_eq(poly1305Mac(key[0..32], msg[0..34], tag[0..16]), true);

        let expected: [16]u8 = [
            0xa8, 0x06, 0x1d, 0xc1, 0x30, 0x51, 0x36, 0xc6,
            0xc2, 0x2b, 0x8b, 0xaf, 0x0c, 0x01, 0x27, 0xa9
        ];
        expect_eq(bytesEq(tag[0..16], expected[0..16]), true);
    });

    test("2. @cap/poly1305 — Streaming multi-chunk update matches one-shot poly1305Mac", () => {
        let key: [32]u8 = undefined;
        fillRfcKey(key[0..32]);

        let msg_str = "Cryptographic Forum Research Group";
        let msg: [34]u8 = undefined;
        let i: usize = 0;
        while (i < 34) {
            msg[i] = msg_str[i];
            i = i + 1;
        }

        let tag_oneshot: [16]u8 = undefined;
        expect_eq(poly1305Mac(key[0..32], msg[0..34], tag_oneshot[0..16]), true);

        // Feed in irregular chunks: 1, 5, 11, 16, 1 bytes
        let st = Poly1305.init(key[0..32]);
        st.update(msg[0..1]);
        st.update(msg[1..6]);
        st.update(msg[6..17]);
        st.update(msg[17..33]);
        st.update(msg[33..34]);
        let tag_stream: [16]u8 = undefined;
        st.finalize(tag_stream[0..16]);

        expect_eq(bytesEq(tag_oneshot[0..16], tag_stream[0..16]), true);
    });

    test("3. @cap/poly1305 — poly1305Verify accepts valid tag & rejects single-bit tamper", () => {
        let key: [32]u8 = undefined;
        fillRfcKey(key[0..32]);

        let msg_str = "Cryptographic Forum Research Group";
        let msg: [34]u8 = undefined;
        let i: usize = 0;
        while (i < 34) {
            msg[i] = msg_str[i];
            i = i + 1;
        }

        let tag: [16]u8 = [
            0xa8, 0x06, 0x1d, 0xc1, 0x30, 0x51, 0x36, 0xc6,
            0xc2, 0x2b, 0x8b, 0xaf, 0x0c, 0x01, 0x27, 0xa9
        ];
        expect_eq(poly1305Verify(key[0..32], msg[0..34], tag[0..16]), true);

        // Flip 1 bit in tag -> reject
        tag[15] = tag[15] ^ 0x01;
        expect_eq(poly1305Verify(key[0..32], msg[0..34], tag[0..16]), false);

        // Restore tag, flip 1 bit in message -> reject
        tag[15] = tag[15] ^ 0x01;
        msg[0] = msg[0] ^ 0x01;
        expect_eq(poly1305Verify(key[0..32], msg[0..34], tag[0..16]), false);
    });

    test("4. @cap/poly1305 — Empty message tag equals s (key[16..32])", () => {
        let key: [32]u8 = undefined;
        fillRfcKey(key[0..32]);

        let empty: [1]u8 = [0];
        let tag: [16]u8 = undefined;
        expect_eq(poly1305Mac(key[0..32], empty[0..0], tag[0..16]), true);
        // For empty message, accumulator h = 0, so tag = (0 + s) mod 2^128 = key[16..32]!
        expect_eq(bytesEq(tag[0..16], key[16..32]), true);
        expect_eq(poly1305Verify(key[0..32], empty[0..0], key[16..32]), true);
    });

    test("5. @cap/poly1305 — Exact 16-byte & multi-block 64-byte boundaries", () => {
        let key: [32]u8 = undefined;
        let i: usize = 0;
        while (i < 32) {
            key[i] = (0xA0 + i) as u8;
            i = i + 1;
        }

        let msg: [64]u8 = undefined;
        i = 0;
        while (i < 64) {
            msg[i] = (i * 3 + 7) as u8;
            i = i + 1;
        }

        let t16: [16]u8 = undefined;
        let t64_a: [16]u8 = undefined;
        let t64_b: [16]u8 = undefined;
        expect_eq(poly1305Mac(key[0..32], msg[0..16], t16[0..16]), true);
        expect_eq(poly1305Mac(key[0..32], msg[0..64], t64_a[0..16]), true);

        // Stream byte-by-byte across all 4 16-byte blocks
        let st = Poly1305.init(key[0..32]);
        i = 0;
        while (i < 64) {
            st.update(msg[i..(i + 1)]);
            i = i + 1;
        }
        st.finalize(t64_b[0..16]);

        expect_eq(bytesEq(t64_a[0..16], t64_b[0..16]), true);
        expect_eq(bytesEq(t16[0..16], t64_a[0..16]), false);
    });

    test("6. @cap/poly1305 — Invalid key & tag buffer lengths safely return false", () => {
        expect_eq(POLY1305_KEY_LEN, 32 as usize);
        expect_eq(POLY1305_TAG_LEN, 16 as usize);

        let key: [32]u8 = undefined;
        let msg: [4]u8 = [1, 2, 3, 4];
        let tag: [16]u8 = undefined;

        expect_eq(poly1305Mac(key[0..31], msg[0..4], tag[0..16]), false);
        expect_eq(poly1305Mac(key[0..32], msg[0..4], tag[0..15]), false);
        expect_eq(poly1305Verify(key[0..16], msg[0..4], tag[0..16]), false);
        expect_eq(poly1305Verify(key[0..32], msg[0..4], tag[0..15]), false);
    });
}
