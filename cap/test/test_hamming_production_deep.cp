// Source: cp/cap/test/test_hamming_production_deep.cp (Production deep tests for cap:hamming)

import std from "std";
import { test, expect_eq } from "cap:test";
import {
    EccStatus,
    Hamming4Decode,
    Secded4Decode,
    Secded64Decode,
    hamming74Encode,
    hamming74Decode,
    secded84Encode,
    secded84Decode,
    secded84EncodeBytes,
    secded84DecodeBytes,
    secded7264Encode,
    secded7264Decode,
} from "cap:codec";

fn slicesEq(a: []const u8, b: []const u8): bool {
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

test("1. Hamming(7,4) exhaustive round-trip and all 16 nibbles x 7 single-bit flips", () => {
    let nibble: u8 = 0;
    while (nibble < 16) {
        const code: u8 = hamming74Encode(nibble);
        // Bit 7 must always be 0.
        expect_eq(code & 0x80, 0 as u8);

        // Error-free decode.
        const clean: Hamming4Decode = hamming74Decode(code);
        expect_eq(clean.data, nibble);
        expect_eq(clean.corrected, false);
        expect_eq(clean.error_bit, 0 as u8);

        // Exhaustively flip each of the 7 bits (positions 1..7).
        let bit: u8 = 0;
        while (bit < 7) {
            const bit_u3: u3 = bit as u3;
            const corrupted: u8 = code ^ ((1 as u8) << bit_u3);
            const res: Hamming4Decode = hamming74Decode(corrupted);
            expect_eq(res.data, nibble);
            expect_eq(res.corrected, true);
            expect_eq(res.error_bit, bit + 1);
            bit += 1;
        }

        // High nibble bits in input must be ignored cleanly.
        expect_eq(hamming74Encode(nibble | 0xF0), code);
        nibble += 1;
    }
});

test("2. SECDED(8,4) exhaustive 16 nibbles x 8 single-bit corrections and all 28 double-bit detections", () => {
    let nibble: u8 = 0;
    while (nibble < 16) {
        const code: u8 = secded84Encode(nibble);

        // Error-free decode.
        const clean: Secded4Decode = secded84Decode(code);
        expect_eq(clean.data, nibble);
        expect_eq(clean.status == EccStatus.ok, true);
        expect_eq(clean.error_bit, 0 as u8);

        // All 8 single-bit flips (positions 1..8) must be corrected precisely.
        let b1: u8 = 0;
        while (b1 < 8) {
            const b1_u3: u3 = b1 as u3;
            const corrupted: u8 = code ^ ((1 as u8) << b1_u3);
            const res: Secded4Decode = secded84Decode(corrupted);
            expect_eq(res.data, nibble);
            expect_eq(res.status == EccStatus.corrected, true);
            expect_eq(res.error_bit, b1 + 1);
            b1 += 1;
        }

        // All C(8,2) = 28 double-bit flips must be detected as double_error.
        let i: u8 = 0;
        while (i < 8) {
            let j: u8 = i + 1;
            while (j < 8) {
                const i_u3: u3 = i as u3;
                const j_u3: u3 = j as u3;
                const mask: u8 = ((1 as u8) << i_u3) | ((1 as u8) << j_u3);
                const dec2: Secded4Decode = secded84Decode(code ^ mask);
                expect_eq(dec2.status == EccStatus.double_error, true);
                expect_eq(dec2.error_bit, 0 as u8);
                j += 1;
            }
            i += 1;
        }

        nibble += 1;
    }
});

test("3. SECDED(8,4) byte-stream encode/decode with single-bit noise healing and double-bit rejection", () => {
    const payload: []const u8 = "CapHamming-ECC-2026!\x00\xFF\x5A\xA5";
    let enc_arr: [64]u8 = std.mem.zeroes([64]u8);
    let enc: []u8 = enc_arr[0..];
    let out_arr: [32]u8 = std.mem.zeroes([32]u8);
    let out: []u8 = out_arr[0..];

    if (enc_len := secded84EncodeBytes(payload, *enc)) {
        expect_eq(enc_len, payload.len * 2);

        // Clean decode.
        if (dec_len := secded84DecodeBytes(enc_arr[0..enc_len], *out)) {
            expect_eq(dec_len, payload.len);
            expect_eq(slicesEq(out_arr[0..dec_len], payload), true);
        } else {
            expect_eq(true, false);
        }

        // Inject one single-bit flip into EVERY encoded byte; all must heal seamlessly.
        let k: usize = 0;
        while (k < enc_len) {
            const bit_u3: u3 = (k & 7) as u3;
            enc_arr[k] = enc_arr[k] ^ ((1 as u8) << bit_u3);
            k += 1;
        }
        if (dec_len2 := secded84DecodeBytes(enc_arr[0..enc_len], *out)) {
            expect_eq(dec_len2, payload.len);
            expect_eq(slicesEq(out_arr[0..dec_len2], payload), true);
        } else {
            expect_eq(true, false);
        }

        // Inject a second bit flip into a single codeword byte -> double-bit error -> must return null.
        const second_bit: u3 = ((5 + 1) & 7) as u3;
        enc_arr[5] = enc_arr[5] ^ ((1 as u8) << second_bit);
        if (_ := secded84DecodeBytes(enc_arr[0..enc_len], *out)) {
            expect_eq(true, false);
        }
    } else {
        expect_eq(true, false);
    }

    // Edge cases: empty slice, odd-length decode, insufficient dst buffers.
    let empty_src: [0]u8 = undefined;
    if (n0 := secded84EncodeBytes(empty_src[0..0], *enc)) {
        expect_eq(n0, 0 as usize);
    } else {
        expect_eq(true, false);
    }
    if (d0 := secded84DecodeBytes(empty_src[0..0], *out)) {
        expect_eq(d0, 0 as usize);
    } else {
        expect_eq(true, false);
    }

    let tiny_arr: [3]u8 = std.mem.zeroes([3]u8);
    let tiny: []u8 = tiny_arr[0..];
    if (_ := secded84EncodeBytes(payload, *tiny)) {
        expect_eq(true, false);
    }
    if (_ := secded84DecodeBytes(enc_arr[0..3], *out)) {
        expect_eq(true, false);
    }
    if (_ := secded84DecodeBytes(enc_arr[0..10], *tiny)) {
        expect_eq(true, false);
    }
});

test("4. SECDED(72,64) server DRAM ECC exhaustive all 72 single-bit flips (64 data + 8 ECC)", () => {
    const vectors: [5]u64 = [
        0x0000000000000000,
        0xFFFFFFFFFFFFFFFF,
        0x0123456789ABCDEF,
        0xDEADBEEFCAFEBABE,
        0x55555555AAAAAAAA,
    ];

    let v_idx: usize = 0;
    while (v_idx < 5) {
        const word: u64 = vectors[v_idx];
        const ecc: u8 = secded7264Encode(word);

        // Clean check.
        const clean: Secded64Decode = secded7264Decode(word, ecc);
        expect_eq(clean.data, word);
        expect_eq(clean.ecc, ecc);
        expect_eq(clean.status == EccStatus.ok, true);
        expect_eq(clean.error_bit, 0 as u8);

        // Flip every one of the 64 data bits.
        let d_bit: u8 = 0;
        while (d_bit < 64) {
            const shift: u6 = d_bit as u6;
            const corrupted: u64 = word ^ ((1 as u64) << shift);
            const res: Secded64Decode = secded7264Decode(corrupted, ecc);
            expect_eq(res.data, word);
            expect_eq(res.ecc, ecc);
            expect_eq(res.status == EccStatus.corrected, true);
            expect_eq(res.error_bit >= 3 && res.error_bit <= 71, true);
            d_bit += 1;
        }

        // Flip every one of the 8 ECC check bits.
        const expected_ecc_pos: [8]u8 = [1, 2, 4, 8, 16, 32, 64, 72];
        let e_bit: u8 = 0;
        while (e_bit < 8) {
            const shift_e: u3 = e_bit as u3;
            const corrupted_ecc: u8 = ecc ^ ((1 as u8) << shift_e);
            const dec_e: Secded64Decode = secded7264Decode(word, corrupted_ecc);
            expect_eq(dec_e.data, word);
            expect_eq(dec_e.ecc, ecc);
            expect_eq(dec_e.status == EccStatus.corrected, true);
            expect_eq(dec_e.error_bit, expected_ecc_pos[e_bit as usize]);
            e_bit += 1;
        }

        v_idx += 1;
    }
});

test("5. SECDED(72,64) exhaustive double-bit error detection across all C(72,2) = 2556 bit pairs", () => {
    const word: u64 = 0xFEDCBA9876543210;
    const ecc: u8 = secded7264Encode(word);

    // 1) All C(64,2) = 2016 data+data double-bit flips.
    let i: u8 = 0;
    while (i < 64) {
        let j: u8 = i + 1;
        while (j < 64) {
            const si: u6 = i as u6;
            const sj: u6 = j as u6;
            const bad_data: u64 = word ^ ((1 as u64) << si) ^ ((1 as u64) << sj);
            const res: Secded64Decode = secded7264Decode(bad_data, ecc);
            expect_eq(res.status == EccStatus.double_error, true);
            expect_eq(res.error_bit, 0 as u8);
            j += 1;
        }
        i += 1;
    }

    // 2) All 64 * 8 = 512 data+ecc double-bit flips.
    let d: u8 = 0;
    while (d < 64) {
        let e: u8 = 0;
        while (e < 8) {
            const sd: u6 = d as u6;
            const se: u3 = e as u3;
            const res: Secded64Decode = secded7264Decode(
                word ^ ((1 as u64) << sd),
                ecc ^ ((1 as u8) << se),
            );
            expect_eq(res.status == EccStatus.double_error, true);
            expect_eq(res.error_bit, 0 as u8);
            e += 1;
        }
        d += 1;
    }

    // 3) All C(8,2) = 28 ecc+ecc double-bit flips.
    let e1: u8 = 0;
    while (e1 < 8) {
        let e2: u8 = e1 + 1;
        while (e2 < 8) {
            const s1: u3 = e1 as u3;
            const s2: u3 = e2 as u3;
            const bad_ecc: u8 = ecc ^ ((1 as u8) << s1) ^ ((1 as u8) << s2);
            const res: Secded64Decode = secded7264Decode(word, bad_ecc);
            expect_eq(res.status == EccStatus.double_error, true);
            expect_eq(res.error_bit, 0 as u8);
            e2 += 1;
        }
        e1 += 1;
    }
});
