// cap/test/testCapBase58.cp
// Standard Bitcoin/IPFS/Flickr Base58 & Base58Check official test vectors.

const std = @import("std");
import { test, expect_eq } from "cap:test";
import {
    ALPHABET_FLICKR,
    Base58Error,
    encode,
    decode,
    encodeWithAlphabet,
    decodeWithAlphabet,
    encodeCheck,
    decodeCheck,
} from "../../packages/@cap/base58/src/index.cp";

fn expectChecksumMismatch(bad: []const u8): bool {
    let out: [64]u8 = [0; 64];
    * = decodeCheck(bad, out[0..]) catch |e| {
        return e == Base58Error.ChecksumMismatch;
    };
    return false;
}

fn expectInvalidChar(bad: []const u8): bool {
    let out: [64]u8 = [0; 64];
    * = decode(bad, out[0..]) catch |e| {
        return e == Base58Error.InvalidCharacter;
    };
    return false;
}

fn main() {
    test("1. @cap/base58 — Official Bitcoin Test Vectors & Leading Zeros", () => {
        let enc: [128]u8 = [0; 128];
        let dec_buf: [128]u8 = [0; 128];

        const e1 = encode("Hello World!", enc[0..]) catch unreachable;
        expect_eq(std.mem.eql(u8, e1, "2NEpo7TZRRrLZSi2U"), true);
        const d1 = decode(e1, dec_buf[0..]) catch unreachable;
        expect_eq(std.mem.eql(u8, d1, "Hello World!"), true);

        const e2 = encode("The quick brown fox jumps over the lazy dog.", enc[0..]) catch unreachable;
        expect_eq(std.mem.eql(u8, e2, "USm3fpXnKG5EUBx2ndxBDMPVciP5hGey2Jh4NDv6gmeo1LkMeiKrLJUUBk6Z"), true);
        const d2 = decode(e2, dec_buf[0..]) catch unreachable;
        expect_eq(std.mem.eql(u8, d2, "The quick brown fox jumps over the lazy dog."), true);

        // Leading zero bytes -> leading '1's
        const zeros: [5]u8 = [0, 0, 0, 0x28, 0x7f];
        const ez = encode(zeros[0..], enc[0..]) catch unreachable;
        expect_eq(ez[0], '1');
        expect_eq(ez[1], '1');
        expect_eq(ez[2], '1');
        const dz = decode(ez, dec_buf[0..]) catch unreachable;
        expect_eq(std.mem.eql(u8, dz, zeros[0..]), true);
    });

    test("2. @cap/base58 — Base58Check Official Bitcoin Address Vector", () => {
        // Standard Bitcoin Wiki Base58Check example
        const payload: [21]u8 = [
            0x00, 0x01, 0x09, 0x66, 0x77, 0x60, 0x06, 0x95, 0x3D, 0x55, 0x67,
            0x43, 0x9E, 0x5E, 0x39, 0xF8, 0x6A, 0x0D, 0x27, 0x3B, 0xEE,
        ];
        let addr_buf: [64]u8 = [0; 64];
        const addr = encodeCheck(payload[0..], addr_buf[0..]) catch unreachable;
        expect_eq(std.mem.eql(u8, addr, "16UwLL9Risc3QfPqBUvKofHmBQ7wMtjvM"), true);

        let out: [32]u8 = [0; 32];
        const verified = decodeCheck(addr, out[0..]) catch unreachable;
        expect_eq(std.mem.eql(u8, verified, payload[0..]), true);

        // Tampered last character must fail checksum verification
        expect_eq(expectChecksumMismatch("16UwLL9Risc3QfPqBUvKofHmBQ7wMtjvN"), true);
    });

    test("3. @cap/base58 — Flickr Alphabet & Invalid Character Rejection", () => {
        let enc: [64]u8 = [0; 64];
        let dec_buf: [64]u8 = [0; 64];
        const ef = encodeWithAlphabet("CP-Lang", ALPHABET_FLICKR, enc[0..]) catch unreachable;
        const df = decodeWithAlphabet(ef, ALPHABET_FLICKR, dec_buf[0..]) catch unreachable;
        expect_eq(std.mem.eql(u8, df, "CP-Lang"), true);

        // '0', 'O', 'I', 'l' are excluded from Base58
        expect_eq(expectInvalidChar("0OIl"), true);
    });
}
