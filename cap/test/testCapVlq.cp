// cap/test/testCapVlq.cp
// Comprehensive unit tests for @cap/vlq (Source Map v3 Base64 VLQ)

import { test, expect_eq } from "cap:test";
import {
    DecodeResult,
    encodeOne,
    decodeOne,
    encodeSlice,
    decodeSlice,
} from "../../packages/@cap/vlq/src/index.cp";

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
    test("1. @cap/vlq — official Source Map v3 reference vectors", () => {
        let buf: [16]u8 = undefined;

        expect_eq(sliceEq(buf[0..encodeOne(0, buf[0..16])], "A"), true);
        expect_eq(sliceEq(buf[0..encodeOne(-1, buf[0..16])], "D"), true);
        expect_eq(sliceEq(buf[0..encodeOne(1, buf[0..16])], "C"), true);
        expect_eq(sliceEq(buf[0..encodeOne(16, buf[0..16])], "gB"), true);
        expect_eq(sliceEq(buf[0..encodeOne(123, buf[0..16])], "2H"), true);
        expect_eq(sliceEq(buf[0..encodeOne(-123, buf[0..16])], "3H"), true);

        expect_eq(decodeOne("2H", 0).value, 123 as i64);
        expect_eq(decodeOne("3H", 0).value, -123 as i64);
        expect_eq(decodeOne("gB", 0).value, 16 as i64);
    });

    test("2. @cap/vlq — SourceMap segments [0,0,0,0] -> 'AAAA' & [1,2,-3,4] round-trip", () => {
        let buf: [32]u8 = undefined;
        const zeros: [4]i64 = [0, 0, 0, 0];
        const n0: usize = encodeSlice(zeros[0..4], buf[0..32]);
        expect_eq(sliceEq(buf[0..n0], "AAAA"), true);

        const seg: [4]i64 = [1, 2, -3, 4];
        const n1: usize = encodeSlice(seg[0..4], buf[0..32]);
        expect_eq(n1 > 0, true);

        let dec_vals: [8]i64 = undefined;
        const cnt: i64 = decodeSlice(buf[0..n1], dec_vals[0..8]);
        expect_eq(cnt, 4 as i64);
        expect_eq(dec_vals[0], 1 as i64);
        expect_eq(dec_vals[1], 2 as i64);
        expect_eq(dec_vals[2], -3 as i64);
        expect_eq(dec_vals[3], 4 as i64);
    });

    test("3. @cap/vlq — large +/-1000000 values round-trip", () => {
        let buf: [32]u8 = undefined;
        const big: [2]i64 = [1000000, -1000000];
        const n: usize = encodeSlice(big[0..2], buf[0..32]);
        expect_eq(n > 0, true);

        let out: [4]i64 = undefined;
        expect_eq(decodeSlice(buf[0..n], out[0..4]), 2 as i64);
        expect_eq(out[0], 1000000 as i64);
        expect_eq(out[1], -1000000 as i64);
    });

    test("4. @cap/vlq — streaming sequential decodeOne across concatenated values", () => {
        const stream = "A2H3HgB"; // 0, 123, -123, 16
        let pos: usize = 0;

        const r0: DecodeResult = decodeOne(stream, pos);
        expect_eq(r0.valid && r0.value == 0, true);
        pos += r0.consumed;

        const r1: DecodeResult = decodeOne(stream, pos);
        expect_eq(r1.valid && r1.value == 123, true);
        pos += r1.consumed;

        const r2: DecodeResult = decodeOne(stream, pos);
        expect_eq(r2.valid && r2.value == -123, true);
        pos += r2.consumed;

        const r3: DecodeResult = decodeOne(stream, pos);
        expect_eq(r3.valid && r3.value == 16, true);
        pos += r3.consumed;
        expect_eq(pos, stream.len);
    });

    test("5. @cap/vlq — rejects invalid Base64, truncated continuation & shift overflow", () => {
        // Invalid character '!'
        expect_eq(decodeOne("!", 0).valid, false);
        // Truncated continuation ('g' has continuation bit 0x20 set with no following char)
        expect_eq(decodeOne("g", 0).valid, false);
        // 14 continuation chars overflow 64-bit shift
        expect_eq(decodeOne("ggggggggggggggA", 0).valid, false);

        let out: [4]i64 = undefined;
        expect_eq(decodeSlice("A!", out[0..4]), -1 as i64);
    });

    test("6. @cap/vlq — empty input & tiny buffer protection", () => {
        let out: [4]i64 = undefined;
        expect_eq(decodeSlice("", out[0..4]), 0 as i64);
        expect_eq(decodeOne("", 0).valid, false);

        // Output slice too small
        let one: [1]i64 = undefined;
        expect_eq(decodeSlice("AAAA", one[0..1]), -1 as i64);

        // Encode buffer too small
        let tiny: [1]u8 = undefined;
        expect_eq(encodeOne(123, tiny[0..1]), 0 as usize);
        const vals: [2]i64 = [0, 0];
        expect_eq(encodeSlice(vals[0..2], tiny[0..1]), 0 as usize);
    });
}
