// cap/test/testCapBs58.cp
// Comprehensive unit tests for @cap/bs58 (Rust bs58 / Bitcoin / Ripple / Flickr parity)

import { test, expect_eq } from "cap:test";
import {
    Base58Alphabet,
    encodeBase58,
    decodeBase58,
    encodeBase58With,
    decodeBase58With,
    isValidBase58,
} from "../../packages/bs58/src/index.cp";

fn main() {
    test("1. @cap/bs58 — Bitcoin official standard test vectors", () => {
        let enc_buf: [256]u8 = undefined;
        let dec_buf: [256]u8 = undefined;

        const v1 = encodeBase58("Hello World!", enc_buf[0..256]).?;
        expect_eq(v1, "2NEpo7TZRRrLZSi2U");
        expect_eq(decodeBase58(v1, dec_buf[0..256]).?, "Hello World!");

        const pangram = "The quick brown fox jumps over the lazy dog.";
        const v2 = encodeBase58(pangram, enc_buf[0..256]).?;
        expect_eq(v2, "USm3fpXnKG5EUBx2ndxBDMPVciP5hGey2Jh4NDv6gmeo1LkMeiKrLJUUBk6Z");
        expect_eq(decodeBase58(v2, dec_buf[0..256]).?, pangram);
    });

    test("2. @cap/bs58 — leading zero byte preservation (\\x00\\x00\\x28\\x7f\\xb4\\xcd -> 11233QC4)", () => {
        let raw: [6]u8 = [0x00, 0x00, 0x28, 0x7f, 0xb4, 0xcd];
        let enc_buf: [64]u8 = undefined;
        let dec_buf: [64]u8 = undefined;

        const enc = encodeBase58(raw[0..6], enc_buf[0..64]).?;
        expect_eq(enc, "11233QC4");

        const decoded = decodeBase58(enc, dec_buf[0..64]).?;
        expect_eq(decoded.len, 6 as usize);
        expect_eq(decoded[0], 0x00 as u8);
        expect_eq(decoded[1], 0x00 as u8);
        expect_eq(decoded[2], 0x28 as u8);
        expect_eq(decoded[3], 0x7f as u8);
        expect_eq(decoded[4], 0xb4 as u8);
        expect_eq(decoded[5], 0xcd as u8);
    });

    test("3. @cap/bs58 — all-zero input maps cleanly to repeated alphabet[0]", () => {
        let zeros: [4]u8 = [0, 0, 0, 0];
        let enc_buf: [32]u8 = undefined;
        let dec_buf: [32]u8 = undefined;

        const enc_btc = encodeBase58(zeros[0..4], enc_buf[0..32]).?;
        expect_eq(enc_btc, "1111");
        const dec_btc = decodeBase58(enc_btc, dec_buf[0..32]).?;
        expect_eq(dec_btc.len, 4 as usize);
        expect_eq(dec_btc[0], 0 as u8);
        expect_eq(dec_btc[3], 0 as u8);

        // Ripple alphabet[0] is 'r'
        const enc_xrp = encodeBase58With(zeros[0..4], Base58Alphabet.Ripple, enc_buf[0..32]).?;
        expect_eq(enc_xrp, "rrrr");
    });

    test("4. @cap/bs58 — Ripple & Flickr alphabets round-trip", () => {
        let enc_buf: [128]u8 = undefined;
        let dec_buf: [128]u8 = undefined;
        const msg = "CapDux Distributed Node ID #42";

        const xrp = encodeBase58With(msg, Base58Alphabet.Ripple, enc_buf[0..128]).?;
        expect_eq(isValidBase58(xrp, Base58Alphabet.Ripple), true);
        expect_eq(decodeBase58With(xrp, Base58Alphabet.Ripple, dec_buf[0..128]).?, msg);

        const flk = encodeBase58With(msg, Base58Alphabet.Flickr, enc_buf[0..128]).?;
        expect_eq(isValidBase58(flk, Base58Alphabet.Flickr), true);
        expect_eq(decodeBase58With(flk, Base58Alphabet.Flickr, dec_buf[0..128]).?, msg);
    });

    test("5. @cap/bs58 — empty input & buffer-too-small guard", () => {
        let buf: [16]u8 = undefined;
        expect_eq(encodeBase58("", buf[0..16]).?, "");
        expect_eq(decodeBase58("", buf[0..16]).?, "");
        expect_eq(isValidBase58("", Base58Alphabet.Bitcoin), true);

        let tiny: [4]u8 = undefined;
        expect_eq(encodeBase58("Hello World!", tiny[0..4]) == null, true);
        expect_eq(decodeBase58("2NEpo7TZRRrLZSi2U", tiny[0..4]) == null, true);
    });

    test("6. @cap/bs58 — strict rejection of ambiguous / invalid chars (0, O, I, l)", () => {
        let dec_buf: [64]u8 = undefined;
        expect_eq(isValidBase58("2NEpo7TZRRrLZSi2U", Base58Alphabet.Bitcoin), true);
        expect_eq(isValidBase58("0abc", Base58Alphabet.Bitcoin), false);
        expect_eq(isValidBase58("Oabc", Base58Alphabet.Bitcoin), false);
        expect_eq(isValidBase58("Iabc", Base58Alphabet.Bitcoin), false);
        expect_eq(isValidBase58("labc", Base58Alphabet.Bitcoin), false);

        expect_eq(decodeBase58("0abc", dec_buf[0..64]) == null, true);
        expect_eq(decodeBase58("Oabc", dec_buf[0..64]) == null, true);
        expect_eq(decodeBase58("Iabc", dec_buf[0..64]) == null, true);
        expect_eq(decodeBase58("labc", dec_buf[0..64]) == null, true);
        expect_eq(decodeBase58("abc+def", dec_buf[0..64]) == null, true);
    });
}
