// cap/test/testCapUuencode.cp
// Comprehensive unit tests for @cap/uuencode (POSIX uuencode/uudecode)

import { test, expect_eq } from "cap:test";
import {
    encodeLine,
    decodeLine,
    encode,
    decode,
} from "../../packages/@cap/uuencode/src/index.cp";

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
    test("1. @cap/uuencode — Wikipedia classic vector 'Cat' -> '#0V%T'", () => {
        let enc: [16]u8 = undefined;
        const n: usize = encodeLine("Cat", enc[0..16]);
        expect_eq(n, 5 as usize);
        expect_eq(sliceEq(enc[0..5], "#0V%T"), true);

        let dec_buf: [16]u8 = undefined;
        const dn: i64 = decodeLine("#0V%T", dec_buf[0..16]);
        expect_eq(dn, 3 as i64);
        expect_eq(sliceEq(dec_buf[0..3], "Cat"), true);
    });

    test("2. @cap/uuencode — 100-byte multi-line round-trip across 45-byte chunks", () => {
        let src: [100]u8 = undefined;
        let i: usize = 0;
        while (i < 100) {
            src[i] = ((i * 7 + 13) % 256) as u8;
            i += 1;
        }
        let enc: [256]u8 = undefined;
        let dec_buf: [128]u8 = undefined;
        const en: usize = encode(src[0..100], enc[0..256]);
        expect_eq(en > 100, true);

        const dn: i64 = decode(enc[0..en], dec_buf[0..128]);
        expect_eq(dn, 100 as i64);
        expect_eq(sliceEq(dec_buf[0..100], src[0..100]), true);
    });

    test("3. @cap/uuencode — full 'begin 644 test.bin ... end' frame decoding", () => {
        const frame = "begin 644 test.bin\n#0V%T\n`\nend\n";
        let out: [32]u8 = undefined;
        const n: i64 = decode(frame, out[0..32]);
        expect_eq(n, 3 as i64);
        expect_eq(sliceEq(out[0..3], "Cat"), true);
    });

    test("4. @cap/uuencode — compatible with legacy space ' ' and modern backtick '`' zero", () => {
        let out: [8]u8 = undefined;
        // 3 zero bytes encoded with legacy spaces vs modern backticks
        expect_eq(decodeLine("#    ", out[0..8]), 3 as i64);
        expect_eq(out[0] == 0 && out[1] == 0 && out[2] == 0, true);

        expect_eq(decodeLine("#````", out[0..8]), 3 as i64);
        expect_eq(out[0] == 0 && out[1] == 0 && out[2] == 0, true);

        expect_eq(decodeLine(" ", out[0..8]), 0 as i64);
        expect_eq(decodeLine("`", out[0..8]), 0 as i64);
    });

    test("5. @cap/uuencode — binary all-zero & all-0xFF round-trip", () => {
        let zeros: [16]u8 = undefined;
        let ones: [16]u8 = undefined;
        let i: usize = 0;
        while (i < 16) {
            zeros[i] = 0x00;
            ones[i] = 0xFF;
            i += 1;
        }
        let enc: [64]u8 = undefined;
        let dec_buf: [32]u8 = undefined;

        const n0: usize = encode(zeros[0..16], enc[0..64]);
        expect_eq(decode(enc[0..n0], dec_buf[0..32]), 16 as i64);
        expect_eq(sliceEq(dec_buf[0..16], zeros[0..16]), true);

        const n1: usize = encode(ones[0..16], enc[0..64]);
        expect_eq(decode(enc[0..n1], dec_buf[0..32]), 16 as i64);
        expect_eq(sliceEq(dec_buf[0..16], ones[0..16]), true);
    });

    test("6. @cap/uuencode — rejects malformed lines & tiny buffers", () => {
        let out: [16]u8 = undefined;
        expect_eq(decodeLine("", out[0..16]), -1 as i64);
        // Truncated data line (claims 3 bytes, only 2 chars after length)
        expect_eq(decodeLine("#0V", out[0..16]), -1 as i64);
        // Out-of-range character 0x7F
        const bad_char: [5]u8 = ['#' as u8, 0x7F, '0' as u8, '0' as u8, '0' as u8];
        expect_eq(decodeLine(bad_char[0..5], out[0..16]), -1 as i64);

        let tiny: [2]u8 = undefined;
        expect_eq(encodeLine("Cat", tiny[0..2]), 0 as usize);
        expect_eq(decodeLine("#0V%T", tiny[0..2]), -1 as i64);
        expect_eq(encode("Cat", tiny[0..2]), 0 as usize);
    });
}
