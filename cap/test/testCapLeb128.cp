import { test, expect_eq } from "cap:test";
import {
    UlebResult,
    SlebResult,
    encodeUnsigned,
    decodeUnsigned,
    encodeSigned,
    decodeSigned,
    unsignedEncodedLen,
} from "../../packages/@cap/leb128/src/index.cp";

fn main() {
    test("1. @cap/leb128 — DWARF/WASM reference ULEB128 (624485 -> 0xE5 0x8E 0x26)", () => {
        let buf: [16]u8 = undefined;
        let n: usize = encodeUnsigned(624485, buf[0..]);
        expect_eq(n, 3);
        expect_eq(buf[0], 0xe5);
        expect_eq(buf[1], 0x8e);
        expect_eq(buf[2], 0x26);
        expect_eq(unsignedEncodedLen(624485), 3);

        let r: UlebResult = decodeUnsigned(buf[0..n], 0);
        expect_eq(r.valid, true);
        expect_eq(r.value, 624485);
        expect_eq(r.consumed, 3);
    });

    test("2. @cap/leb128 — DWARF/WASM reference SLEB128 (-123456 -> 0xC0 0xBB 0x78)", () => {
        let buf: [16]u8 = undefined;
        let n: usize = encodeSigned(-123456, buf[0..]);
        expect_eq(n, 3);
        expect_eq(buf[0], 0xc0);
        expect_eq(buf[1], 0xbb);
        expect_eq(buf[2], 0x78);

        let r: SlebResult = decodeSigned(buf[0..n], 0);
        expect_eq(r.valid, true);
        expect_eq(r.value, -123456);
        expect_eq(r.consumed, 3);
    });

    test("3. @cap/leb128 — unsigned boundary values (0, 127, 128, u64::MAX)", () => {
        let max_u64: u64 = ((0xffffffff as u64) << 32) | (0xffffffff as u64);
        let vals: [4]u64 = undefined;
        vals[0] = 0;
        vals[1] = 127;
        vals[2] = 128;
        vals[3] = max_u64;

        let i: usize = 0;
        while (i < 4) {
            let buf: [16]u8 = undefined;
            let n: usize = encodeUnsigned(vals[i], buf[0..]);
            expect_eq(n, unsignedEncodedLen(vals[i]));
            let r: UlebResult = decodeUnsigned(buf[0..n], 0);
            expect_eq(r.valid, true);
            expect_eq(r.value, vals[i]);
            expect_eq(r.consumed, n);
            i += 1;
        }
    });

    test("4. @cap/leb128 — signed boundary values (0, -1, 63, -64, 64, -65, i64 extremes)", () => {
        let vals: [7]i64 = undefined;
        vals[0] = 0;
        vals[1] = -1;
        vals[2] = 63;
        vals[3] = -64;
        vals[4] = 64;
        vals[5] = -65;
        vals[6] = -1000000000000;

        let i: usize = 0;
        while (i < 7) {
            let buf: [16]u8 = undefined;
            let n: usize = encodeSigned(vals[i], buf[0..]);
            expect_eq(n > 0, true);
            let r: SlebResult = decodeSigned(buf[0..n], 0);
            expect_eq(r.valid, true);
            expect_eq(r.value, vals[i]);
            expect_eq(r.consumed, n);
            i += 1;
        }
    });

    test("5. @cap/leb128 — truncated & 10-byte overflow rejection", () => {
        let trunc: [2]u8 = undefined;
        trunc[0] = 0x80;
        trunc[1] = 0x80;
        expect_eq(decodeUnsigned(trunc[0..2], 0).valid, false);
        expect_eq(decodeSigned(trunc[0..2], 0).valid, false);

        // 10th byte = 0x02 overflows u64
        let ovf: [10]u8 = undefined;
        let k: usize = 0;
        while (k < 9) { ovf[k] = 0x80; k += 1; }
        ovf[9] = 0x02;
        expect_eq(decodeUnsigned(ovf[0..10], 0).valid, false);
        expect_eq(decodeSigned(ovf[0..10], 0).valid, false);
    });

    test("6. @cap/leb128 — small buffer protection", () => {
        let tiny: [2]u8 = undefined;
        expect_eq(encodeUnsigned(624485, tiny[0..]), 0);
        expect_eq(encodeSigned(-123456, tiny[0..]), 0);
    });
}
