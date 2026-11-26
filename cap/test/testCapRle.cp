// cap/test/testCapRle.cp
// Comprehensive unit tests for @cap/rle (PackBits RLE + Delta / Delta-of-Delta)

import { test, expect_eq } from "cap:test";
import {
    encodePackBits,
    decodePackBits,
    encodeDeltaI64,
    decodeDeltaI64,
    encodeDeltaOfDeltaI64,
    decodeDeltaOfDeltaI64,
} from "../../packages/@cap/rle/src/index.cp";

fn main() {
    test("1. @cap/rle — PackBits high-repetition compression ratio & exact decode", () => {
        let raw: [200]u8 = undefined;
        let i: usize = 0;
        while (i < 200) {
            raw[i] = 'A' as u8;
            i += 1;
        }
        let enc_buf: [64]u8 = undefined;
        let dec_buf: [256]u8 = undefined;

        const enc = encodePackBits(raw[0..200], enc_buf[0..64]).?;
        // 200 identical bytes split into 128 + 72 -> only 4 compressed bytes!
        expect_eq(enc.len, 4 as usize);

        const decoded = decodePackBits(enc, dec_buf[0..256]).?;
        expect_eq(decoded.len, 200 as usize);
        expect_eq(decoded[0], 'A' as u8);
        expect_eq(decoded[199], 'A' as u8);
    });

    test("2. @cap/rle — PackBits mixed literal + repeat runs round-trip", () => {
        const msg = "abcDDDDDDxyzEEEE12345FFFFF";
        let enc_buf: [128]u8 = undefined;
        let dec_buf: [128]u8 = undefined;

        const enc = encodePackBits(msg, enc_buf[0..128]).?;
        expect_eq(enc.len < msg.len, true);
        expect_eq(decodePackBits(enc, dec_buf[0..128]).?, msg);
    });

    test("3. @cap/rle — PackBits non-repeating bytes & NOP (128) byte handling", () => {
        const unique = "0123456789abcdefghijklmnopqrstuvwxyz";
        let enc_buf: [128]u8 = undefined;
        let dec_buf: [128]u8 = undefined;

        const enc = encodePackBits(unique, enc_buf[0..128]).?;
        expect_eq(decodePackBits(enc, dec_buf[0..128]).?, unique);

        // Stream containing NOP (0x80 = 128) between runs
        let with_nop: [6]u8 = [0x80, 0xFE, 'X', 0x80, 0x00, 'Y'];
        expect_eq(decodePackBits(with_nop[0..6], dec_buf[0..128]).?, "XXXY");
    });

    test("4. @cap/rle — PackBits rejects truncated streams & buffer overflow", () => {
        let dec_buf: [32]u8 = undefined;

        // Literal control byte 5 claims 6 bytes, only 2 follow
        let bad_lit: [3]u8 = [5, 'a', 'b'];
        expect_eq(decodePackBits(bad_lit[0..3], dec_buf[0..32]) == null, true);

        // Repeat control byte 254 missing payload byte
        let bad_rep: [1]u8 = [254];
        expect_eq(decodePackBits(bad_rep[0..1], dec_buf[0..32]) == null, true);

        // Output buffer too small
        let rep10: [2]u8 = [247, 'Z']; // 257 - 247 = 10 'Z's
        let tiny: [4]u8 = undefined;
        expect_eq(decodePackBits(rep10[0..2], tiny[0..4]) == null, true);
    });

    test("5. @cap/rle — Delta first-order encoding & lossless prefix-sum decoding", () => {
        let vals: [5]i64 = [100, 105, 102, 110, 95];
        let deltas: [5]i64 = undefined;
        let restored: [5]i64 = undefined;

        expect_eq(encodeDeltaI64(vals[0..5], deltas[0..5]), 5 as usize);
        expect_eq(deltas[0], 100 as i64);
        expect_eq(deltas[1], 5 as i64);
        expect_eq(deltas[2], -3 as i64);
        expect_eq(deltas[3], 8 as i64);
        expect_eq(deltas[4], -15 as i64);

        expect_eq(decodeDeltaI64(deltas[0..5], restored[0..5]), 5 as usize);
        expect_eq(restored[0], 100 as i64);
        expect_eq(restored[2], 102 as i64);
        expect_eq(restored[4], 95 as i64);
    });

    test("6. @cap/rle — Delta-of-Delta constant-interval & jittered timestamps round-trip", () => {
        // Constant 60s interval -> all 0s from index 2 onward
        let steady: [5]i64 = [1700000000, 1700000060, 1700000120, 1700000180, 1700000240];
        let dod: [5]i64 = undefined;
        let out: [5]i64 = undefined;

        expect_eq(encodeDeltaOfDeltaI64(steady[0..5], dod[0..5]), 5 as usize);
        expect_eq(dod[0], 1700000000 as i64);
        expect_eq(dod[1], 60 as i64);
        expect_eq(dod[2], 0 as i64);
        expect_eq(dod[3], 0 as i64);
        expect_eq(dod[4], 0 as i64);

        expect_eq(decodeDeltaOfDeltaI64(dod[0..5], out[0..5]), 5 as usize);
        expect_eq(out[4], 1700000240 as i64);

        // Jittered timestamps (+60, +62, +58, +61 -> dod: +2, -4, +3)
        let jitter: [5]i64 = [1000, 1060, 1122, 1180, 1241];
        expect_eq(encodeDeltaOfDeltaI64(jitter[0..5], dod[0..5]), 5 as usize);
        expect_eq(dod[2], 2 as i64);
        expect_eq(dod[3], -4 as i64);
        expect_eq(dod[4], 3 as i64);

        expect_eq(decodeDeltaOfDeltaI64(dod[0..5], out[0..5]), 5 as usize);
        expect_eq(out[0], 1000 as i64);
        expect_eq(out[2], 1122 as i64);
        expect_eq(out[4], 1241 as i64);
    });
}
