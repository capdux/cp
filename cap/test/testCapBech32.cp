// cap/test/testCapBech32.cp
// Comprehensive unit tests for @cap/bech32

import { test, expect_eq } from "cap:test";
import {
    Bech32Variant,
    Bech32Decoded,
    to5BitWords,
    from5BitWords,
    encodeBech32,
    decodeBech32,
    verifyBech32,
} from "../../packages/bech32/src/index.cp";

fn main() {
    test("1. @cap/bech32 — BIP-173 Bech32 official test vectors (encode, decode, uppercase)", () => {
        let out: [128]u8 = undefined;
        let hrp_buf: [64]u8 = undefined;
        let data_buf: [64]u8 = undefined;

        // BIP-173 empty payload vector: hrp="a" -> "a12uel5l"
        const enc_a = encodeBech32("a", "", Bech32Variant.Bech32, out[0..128]).?;
        expect_eq(enc_a, "a12uel5l");
        expect_eq(verifyBech32("a12uel5l"), true);
        expect_eq(verifyBech32("A12UEL5L"), true);

        const d_upper = decodeBech32("A12UEL5L", hrp_buf[0..64], data_buf[0..64]);
        expect_eq(d_upper.valid, true);
        expect_eq(d_upper.variant == Bech32Variant.Bech32, true);
        expect_eq(d_upper.hrp, "a");
        expect_eq(d_upper.data, "");

        // Official BIP-173 vector: abcdef1qpzry9x8gf2tvdw0s3jn54khce6mua7lmqqqxw
        expect_eq(verifyBech32("abcdef1qpzry9x8gf2tvdw0s3jn54khce6mua7lmqqqxw"), true);
    });

    test("2. @cap/bech32 — BIP-350 Bech32m official test vectors", () => {
        let out: [128]u8 = undefined;
        let hrp_buf: [64]u8 = undefined;
        let data_buf: [64]u8 = undefined;

        // BIP-350 empty payload vector: hrp="a" -> "a1lqfn3a"
        const enc_m = encodeBech32("a", "", Bech32Variant.Bech32m, out[0..128]).?;
        expect_eq(enc_m, "a1lqfn3a");
        expect_eq(verifyBech32("A1LQFN3A"), true);

        const d_m = decodeBech32("A1LQFN3A", hrp_buf[0..64], data_buf[0..64]);
        expect_eq(d_m.valid, true);
        expect_eq(d_m.variant == Bech32Variant.Bech32m, true);
        expect_eq(d_m.hrp, "a");

        // Official BIP-350 vector: abcdef1l7aum6echk45nj3s0wdvt2fg8x9yrzpqzd3ryx
        expect_eq(verifyBech32("abcdef1l7aum6echk45nj3s0wdvt2fg8x9yrzpqzd3ryx"), true);
    });

    test("3. @cap/bech32 — 8-bit <-> 5-bit word conversion round-trip & strict padding check", () => {
        let words: [64]u8 = undefined;
        let bytes: [64]u8 = undefined;

        const payload = "age-secret-key-payload";
        const wlen = to5BitWords(payload, words[0..64]).?;
        const blen = from5BitWords(words[0..wlen], bytes[0..64]).?;
        expect_eq(bytes[0..blen], payload);

        // Non-zero trailing padding bits must be rejected
        let bad_words: [2]u8 = [0, 1]; // 10 bits => 8 bits + 2 non-zero pad bits (0b01)
        expect_eq(from5BitWords(bad_words[0..2], bytes[0..64]) == null, true);
    });

    test("4. @cap/bech32 — Nostr (npub) & Age (age) payload round-trip in Bech32 & Bech32m", () => {
        let enc: [128]u8 = undefined;
        let hrp_buf: [32]u8 = undefined;
        let data_buf: [64]u8 = undefined;

        let pubkey: [16]u8 = [0x01, 0x23, 0x45, 0x67, 0x89, 0xab, 0xcd, 0xef, 0xfe, 0xdc, 0xba, 0x98, 0x76, 0x54, 0x32, 0x10];

        const npub = encodeBech32("npub", pubkey[0..16], Bech32Variant.Bech32, enc[0..128]).?;
        const d1 = decodeBech32(npub, hrp_buf[0..32], data_buf[0..64]);
        expect_eq(d1.valid, true);
        expect_eq(d1.variant == Bech32Variant.Bech32, true);
        expect_eq(d1.hrp, "npub");
        expect_eq(d1.data, pubkey[0..16]);

        const age = encodeBech32("age", pubkey[0..16], Bech32Variant.Bech32m, enc[0..128]).?;
        const d2 = decodeBech32(age, hrp_buf[0..32], data_buf[0..64]);
        expect_eq(d2.valid, true);
        expect_eq(d2.variant == Bech32Variant.Bech32m, true);
        expect_eq(d2.hrp, "age");
        expect_eq(d2.data, pubkey[0..16]);
    });

    test("5. @cap/bech32 — Single-character tamper detection by BCH checksum", () => {
        let enc: [128]u8 = undefined;
        let hrp_buf: [32]u8 = undefined;
        let data_buf: [64]u8 = undefined;

        const valid_str = encodeBech32("cap", "hello-world", Bech32Variant.Bech32m, enc[0..128]).?;
        expect_eq(verifyBech32(valid_str), true);

        // Flip 1 character in data section
        enc[5] = if (enc[5] == 'q') 'p' as u8 else 'q' as u8;
        expect_eq(verifyBech32(enc[0..valid_str.len]), false);
        expect_eq(decodeBech32(enc[0..valid_str.len], hrp_buf[0..32], data_buf[0..64]).valid, false);
    });

    test("6. @cap/bech32 — Mixed case, invalid HRP, missing separator & bad chars rejected", () => {
        let out: [64]u8 = undefined;
        expect_eq(verifyBech32("A12uel5l"), false); // mixed case
        expect_eq(verifyBech32("a1Lqfn3a"), false); // mixed case
        expect_eq(verifyBech32("12uel5l"), false);  // empty HRP
        expect_eq(verifyBech32("noseparator"), false);
        expect_eq(verifyBech32("a12uel5"), false);  // checksum < 6 chars
        expect_eq(encodeBech32("", "hi", Bech32Variant.Bech32, out[0..64]) == null, true);
    });
}
