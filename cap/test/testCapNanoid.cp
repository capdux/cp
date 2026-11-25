// cap/test/testCapNanoid.cp
// Deep unit tests for @cap/nanoid (standard 21-char ID, custom alphabets, unbiased bitmask rejection sampling,
// deterministic generation, entropy calculation, and error handling).

const std = @import("std");
import { test, expect_eq } from "cap:test";
import {
    DEFAULT_SIZE,
    URL_ALPHABET,
    ALPHABET_HEX_LOWER,
    ALPHABET_NUMBERS,
    ALPHABET_NOLOOKEALIKE,
    NanoidError,
    calcBitmask,
    entropyBits,
    generateFromBytes,
    nanoidCustom,
    nanoidLen,
    nanoid,
    isValidCustom,
    isValidNanoid,
} from "../../packages/@cap/nanoid/src/index.cp";

fn checkInvalidAlphabet(): bool {
    let out: [4]u8 = [0; 4];
    * = nanoidCustom("a", 4, out[0..]) catch |e| {
        return e == NanoidError.InvalidAlphabet;
    };
    return false;
}

fn checkBufferTooSmall(): bool {
    let out: [4]u8 = [0; 4];
    * = nanoidLen(10, out[0..]) catch |e| {
        return e == NanoidError.BufferTooSmall;
    };
    return false;
}

fn checkEntropyExhausted(): bool {
    let out: [4]u8 = [0; 4];
    const tiny_rng: [2]u8 = [0x0F, 0x0E];
    * = generateFromBytes(ALPHABET_NUMBERS, 1, tiny_rng[0..], out[0..]) catch |e| {
        return e == NanoidError.EntropyExhausted;
    };
    return false;
}

fn main() {
    test("1. @cap/nanoid — Standard 21-char ID & Uniqueness", () => {
        const a = nanoid();
        const b = nanoid();
        expect_eq(a.len, DEFAULT_SIZE);
        expect_eq(isValidNanoid(a[0..]), true);
        expect_eq(isValidNanoid(b[0..]), true);
        expect_eq(std.mem.eql(u8, a[0..], b[0..]), false);

        expect_eq(isValidNanoid("too-short"), false);
        expect_eq(isValidNanoid("invalid!char$in21len!"), false);
    });

    test("2. @cap/nanoid — Custom Length & Preset Alphabets", () => {
        let buf32: [32]u8 = [0; 32];
        const s32 = nanoidLen(32, buf32[0..]) catch unreachable;
        expect_eq(s32.len, 32 as usize);
        expect_eq(isValidCustom(s32, URL_ALPHABET, 32), true);

        let num_buf: [12]u8 = [0; 12];
        const digits = nanoidCustom(ALPHABET_NUMBERS, 12, num_buf[0..]) catch unreachable;
        expect_eq(isValidCustom(digits, ALPHABET_NUMBERS, 12), true);

        let code_buf: [10]u8 = [0; 10];
        const code = nanoidCustom(ALPHABET_NOLOOKEALIKE, 10, code_buf[0..]) catch unreachable;
        expect_eq(isValidCustom(code, ALPHABET_NOLOOKEALIKE, 10), true);
    });

    test("3. @cap/nanoid — Unbiased Bitmask & Deterministic Rejection Sampling", () => {
        expect_eq(calcBitmask(2), 1 as u8);
        expect_eq(calcBitmask(10), 15 as u8);
        expect_eq(calcBitmask(16), 15 as u8);
        expect_eq(calcBitmask(64), 63 as u8);

        // Alphabet of size 10 -> mask = 15 (0x0F). Bytes with low nibble >= 10 (0x0A..0x0F) must be rejected.
        const raw: [8]u8 = [0x0A, 0x0F, 0x13, 0x20, 0x0B, 0x39, 0x05, 0x02];
        let out: [4]u8 = [0; 4];
        const det = generateFromBytes(ALPHABET_NUMBERS, 4, raw[0..], out[0..]) catch unreachable;
        // Accepted indices: 3, 0, 9, 5 -> "3095"
        expect_eq(std.mem.eql(u8, det, "3095"), true);

        const bits: f64 = entropyBits(64, 21);
        expect_eq(bits > 125.9 && bits < 126.1, true);
    });

    test("4. @cap/nanoid — Error Boundaries", () => {
        expect_eq(checkInvalidAlphabet(), true);
        expect_eq(checkBufferTooSmall(), true);
        expect_eq(checkEntropyExhausted(), true);
    });
}
