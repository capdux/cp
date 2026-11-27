import { test, expect_eq } from "cap:test";
import {
    isMappedToNothing, isNonAsciiSpace, isSaslProhibited,
    saslprep, nameprepAscii
} from "../../packages/stringprep/src/index.cp";

fn eqBuf(buf: []u8, len: usize, expected: string): bool {
    if (len != expected.len) { return false; }
    let i: usize = 0;
    while (i < len) {
        if (buf[i] != expected[i]) { return false; }
        i = i + 1;
    }
    return true;
}

fn main() {
    test("RFC 4013 SASLprep strips soft hyphen U+00AD", () => {
        // "I\u00ADX" -> "IX" (RFC 4013 Section 3 example 1)
        let out: [32]u8 = [0; 32];
        let r = saslprep("I\xc2\xadX", out[0..32]);
        expect_eq(r.ok, true);
        expect_eq(eqBuf(out[0..32], r.len, "IX"), true);
    });

    test("SASLprep maps non-ASCII spaces (NBSP U+00A0 & ideographic U+3000) to ASCII 0x20", () => {
        let out: [32]u8 = [0; 32];
        let r = saslprep("a\xc2\xa0b\xe3\x80\x80c", out[0..32]);
        expect_eq(r.ok, true);
        expect_eq(eqBuf(out[0..32], r.len, "a b c"), true);
    });

    test("SASLprep preserves case & printable ASCII", () => {
        let out: [32]u8 = [0; 32];
        let r = saslprep("User_Pass123!", out[0..32]);
        expect_eq(r.ok, true);
        expect_eq(eqBuf(out[0..32], r.len, "User_Pass123!"), true);
    });

    test("SASLprep rejects ASCII control chars & prohibited codepoints", () => {
        let out: [32]u8 = [0; 32];
        expect_eq(saslprep("bad\x07bell", out[0..32]).ok, false);
        expect_eq(saslprep("bad\x7fdel", out[0..32]).ok, false);
        expect_eq(isSaslProhibited(0x0007), true);
        expect_eq(isSaslProhibited(0xE000), true);
        expect_eq(isSaslProhibited(0xFFFD), true);
    });

    test("nameprepAscii case-folds ASCII & strips Table B.1", () => {
        let out: [32]u8 = [0; 32];
        let r = nameprepAscii("Ex\xc2\xadAmPlE.COM", out[0..32]);
        expect_eq(r.ok, true);
        expect_eq(eqBuf(out[0..32], r.len, "example.com"), true);
    });

    test("table classifiers & small buffer safety", () => {
        expect_eq(isMappedToNothing(0x200B), true);
        expect_eq(isNonAsciiSpace(0x00A0), true);
        let tiny: [2]u8 = [0; 2];
        expect_eq(saslprep("hello", tiny[0..2]).ok, false);
        let r_empty = saslprep("", tiny[0..2]);
        expect_eq(r_empty.ok, true);
        expect_eq(r_empty.len, 0 as usize);
    });
}
