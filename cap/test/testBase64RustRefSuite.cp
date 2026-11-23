// Reference-hardened test suite against marshallpierce/rust-base64 (MIT/Apache-2.0) & RFC 4648
// Covers:
//   1. RFC 4648 §10 official test vectors & all 4 variants (Standard, StandardNoPad, UrlSafe, UrlSafeNoPad)
//   2. rust-base64 RequireCanonical strict checks: NonCanonicalPaddingBits (AB==, ABC=), len%4==1,
//      interspersed '=', excessive '=', alphabet confusion (+/ vs -_), overflow-safe encodedLenChecked
//   3. 16-lane @Vector(16, u8) SIMD alphabet preflight & 12B<->16B codec 0-diff parity vs scalar (0..257 bytes)
//   4. Deterministic PRNG Fuzz (>= 1,600 random binary round-trips + malformed Base64 inputs)

import { test, expect_eq } from "cap:test";
import {
    Base64Variant,
    Base64Error,
} from "../../lib/base64/types";
import {
    encodedLen,
    encodedLenChecked,
    decodedLenEstimate,
    exactDecodedLen,
    encodeInto,
    encodeToSliceScalar,
    decodeInto,
    decodeToSliceScalar,
    simdValidateBlock16,
    simdValidateBlock16Scalar,
    simdValidateBase64Alphabet,
} from "../../lib/base64/codec";
const std = @import("std");

fn nextRng(state: *u64): u64 {
    let x = state.*;
    x ^= x << 13;
    x ^= x >> 7;
    x ^= x << 17;
    state.* = x;
    return x;
}

fn bytesEq(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) {
        if (a[i] != b[i]) return false;
        i += 1;
    }
    return true;
}

fn tryDecodeSimdLen(dst: []u8, src: []const u8, v: Base64Variant): usize? {
    const res = decodeInto(dst, src, v) catch return null;
    return res.len;
}

fn tryDecodeScalarLen(dst: []u8, src: []const u8, v: Base64Variant): usize? {
    const res = decodeToSliceScalar(dst, src, v) catch return null;
    return res.len;
}

fn main() {
    test("1. RFC 4648 Section 10 test vectors & all 4 variants", () => {
        let enc_buf: [64]u8 = undefined;
        let dec_buf: [64]u8 = undefined;

        const inputs: [7][]const u8 = [ "", "f", "fo", "foo", "foob", "fooba", "foobar" ];
        const expected_std: [7][]const u8 = [ "", "Zg==", "Zm8=", "Zm9v", "Zm9vYg==", "Zm9vYmE=", "Zm9vYmFy" ];
        const expected_nopad: [7][]const u8 = [ "", "Zg", "Zm8", "Zm9v", "Zm9vYg", "Zm9vYmE", "Zm9vYmFy" ];

        let i: usize = 0;
        while (i < 7) {
            const e1: []u8 = encodeInto(&enc_buf, inputs[i], .Standard) catch unreachable;
            expect_eq(bytesEq(e1, expected_std[i]), true);
            const d1: []u8 = decodeInto(&dec_buf, e1, .Standard) catch unreachable;
            expect_eq(bytesEq(d1, inputs[i]), true);

            const e2: []u8 = encodeInto(&enc_buf, inputs[i], .StandardNoPad) catch unreachable;
            expect_eq(bytesEq(e2, expected_nopad[i]), true);
            const d2: []u8 = decodeInto(&dec_buf, e2, .StandardNoPad) catch unreachable;
            expect_eq(bytesEq(d2, inputs[i]), true);
            i += 1;
        }

        // Standard (+/) vs URL-Safe (-_) differentiation on bytes 0xFB, 0xFF, 0xFE -> "+//+" vs "-__-"
        const bin3: [3]u8 = [ 0xFB, 0xFF, 0xFE ];
        const e_std: []u8 = encodeInto(&enc_buf, &bin3, .Standard) catch unreachable;
        expect_eq(bytesEq(e_std, "+//+"), true);
        const e_url: []u8 = encodeInto(&enc_buf, &bin3, .UrlSafe) catch unreachable;
        expect_eq(bytesEq(e_url, "-__-"), true);
        const d_url: []u8 = decodeInto(&dec_buf, e_url, .UrlSafe) catch unreachable;
        expect_eq(bytesEq(d_url, &bin3), true);

        const e_url_np: []u8 = encodeInto(&enc_buf, bin3[0..2], .UrlSafeNoPad) catch unreachable;
        expect_eq(bytesEq(e_url_np, "-_8"), true);
        const d_url_np: []u8 = decodeInto(&dec_buf, e_url_np, .UrlSafeNoPad) catch unreachable;
        expect_eq(bytesEq(d_url_np, bin3[0..2]), true);
    });

    test("2. Strict RequireCanonical decoding & overflow-safe length checks", () => {
        let dec_buf: [64]u8 = undefined;

        // Non-canonical trailing bits: 'A'=0, 'B'=1 (lower 4 bits != 0 for 2-char quantum)
        let err_ab = false;
        * = decodeInto(&dec_buf, "AB==", .Standard) catch { err_ab = true; };
        expect_eq(err_ab, true);

        let err_ab_np = false;
        * = decodeInto(&dec_buf, "AB", .StandardNoPad) catch { err_ab_np = true; };
        expect_eq(err_ab_np, true);

        // 'A'=0, 'B'=1, 'C'=2 (lower 2 bits of 'C'=0b10 != 0 for 3-char quantum)
        let err_abc = false;
        * = decodeInto(&dec_buf, "ABC=", .Standard) catch { err_abc = true; };
        expect_eq(err_abc, true);

        // len % 4 == 1 rejected across all variants
        let err_len1 = false;
        * = decodeInto(&dec_buf, "A", .StandardNoPad) catch { err_len1 = true; };
        expect_eq(err_len1, true);
        let err_len5 = false;
        * = decodeInto(&dec_buf, "AAAAA", .UrlSafeNoPad) catch { err_len5 = true; };
        expect_eq(err_len5, true);

        // Interspersed '=' and excessive '='
        let err_mid_pad = false;
        * = decodeInto(&dec_buf, "AA=A", .Standard) catch { err_mid_pad = true; };
        expect_eq(err_mid_pad, true);
        let err_mid_pad2 = false;
        * = decodeInto(&dec_buf, "Zg==Zm9v", .Standard) catch { err_mid_pad2 = true; };
        expect_eq(err_mid_pad2, true);
        let err_all_pad = false;
        * = exactDecodedLen("====", .Standard) catch { err_all_pad = true; };
        expect_eq(err_all_pad, true);

        // Alphabet confusion (+/ in UrlSafe, -_ in Standard)
        let err_mix1 = false;
        * = decodeInto(&dec_buf, "+//+", .UrlSafe) catch { err_mix1 = true; };
        expect_eq(err_mix1, true);
        let err_mix2 = false;
        * = decodeInto(&dec_buf, "-__-", .Standard) catch { err_mix2 = true; };
        expect_eq(err_mix2, true);

        // Overflow-safe encodedLenChecked
        expect_eq(encodedLenChecked(std.math.maxInt(usize), .Standard) == null, true);
        expect_eq(encodedLenChecked(std.math.maxInt(usize), .UrlSafeNoPad) == null, true);
        expect_eq(encodedLen(6, .Standard), 8 as usize);
        expect_eq(decodedLenEstimate(8), 6 as usize);
    });

    test("3. 16-lane SIMD alphabet check & 12B<->16B codec 0-diff parity vs scalar (0..257 bytes)", () => {
        let raw: [257]u8 = undefined;
        let simd_enc: [400]u8 = undefined;
        let scal_enc: [400]u8 = undefined;
        let simd_dec: [257]u8 = undefined;
        let scal_dec: [257]u8 = undefined;

        let idx: usize = 0;
        while (idx < 257) {
            raw[idx] = ((idx * 131 + 17) & 0xFF) as u8;
            idx += 1;
        }

        const variants: [4]Base64Variant = [ .Standard, .StandardNoPad, .UrlSafe, .UrlSafeNoPad ];
        let vi: usize = 0;
        while (vi < 4) {
            const v = variants[vi];
            const is_url = (vi >= 2);
            let len: usize = 0;
            while (len <= 257) {
                const e_simd: []u8 = encodeInto(&simd_enc, raw[0..len], v) catch unreachable;
                const e_scal: []u8 = encodeToSliceScalar(&scal_enc, raw[0..len], v) catch unreachable;
                expect_eq(bytesEq(e_simd, e_scal), true);

                if (vi == 1 || vi == 3) {
                    expect_eq(simdValidateBase64Alphabet(e_simd, is_url), true);
                }

                const d_simd: []u8 = decodeInto(&simd_dec, e_simd, v) catch unreachable;
                const d_scal: []u8 = decodeToSliceScalar(&scal_dec, e_simd, v) catch unreachable;
                expect_eq(bytesEq(d_simd, raw[0..len]), true);
                expect_eq(bytesEq(d_simd, d_scal), true);
                len += 1;
            }
            vi += 1;
        }

        // Single-byte corruption parity for simdValidateBlock16 vs scalar
        let blk: [16]u8 = undefined;
        let p: usize = 0;
        while (p < 16) {
            let k: usize = 0;
            while (k < 16) {
                blk[k] = 65 as u8; // 'A'
                k += 1;
            }
            blk[p] = 61 as u8; // '=' invalid in unpadded block
            expect_eq(simdValidateBlock16(&blk, false), simdValidateBlock16Scalar(&blk, false));
            expect_eq(simdValidateBlock16(&blk, true), simdValidateBlock16Scalar(&blk, true));
            blk[p] = 43 as u8; // '+' valid in Standard, invalid in UrlSafe
            expect_eq(simdValidateBlock16(&blk, false), simdValidateBlock16Scalar(&blk, false));
            expect_eq(simdValidateBlock16(&blk, true), simdValidateBlock16Scalar(&blk, true));
            blk[p] = 45 as u8; // '-' invalid in Standard, valid in UrlSafe
            expect_eq(simdValidateBlock16(&blk, false), simdValidateBlock16Scalar(&blk, false));
            expect_eq(simdValidateBlock16(&blk, true), simdValidateBlock16Scalar(&blk, true));
            p += 1;
        }
    });

    test("4. Deterministic PRNG Fuzz (>= 1,600 random round-trips + malformed Base64 inputs)", () => {
        let seed: u64 = 0x5EED_B640_2026_0040;
        let raw_buf: [128]u8 = undefined;
        let enc_simd: [200]u8 = undefined;
        let enc_scal: [200]u8 = undefined;
        let dec_simd: [160]u8 = undefined;
        let dec_scal: [160]u8 = undefined;

        const variants: [4]Base64Variant = [ .Standard, .StandardNoPad, .UrlSafe, .UrlSafeNoPad ];

        // 4A. 800 random binary round-trips across all 4 variants with SIMD vs scalar 0-diff
        let iter: usize = 0;
        while (iter < 800) {
            const slen: usize = (nextRng(&seed) % 128) as usize;
            let k: usize = 0;
            while (k < slen) {
                raw_buf[k] = (nextRng(&seed) & 0xFF) as u8;
                k += 1;
            }
            const v = variants[iter % 4];
            const es: []u8 = encodeInto(&enc_simd, raw_buf[0..slen], v) catch unreachable;
            const ec: []u8 = encodeToSliceScalar(&enc_scal, raw_buf[0..slen], v) catch unreachable;
            expect_eq(bytesEq(es, ec), true);

            const ds: []u8 = decodeInto(&dec_simd, es, v) catch unreachable;
            const dc: []u8 = decodeToSliceScalar(&dec_scal, es, v) catch unreachable;
            expect_eq(bytesEq(ds, raw_buf[0..slen]), true);
            expect_eq(bytesEq(ds, dc), true);
            iter += 1;
        }

        // 4B. 800 malformed Base64 fuzz inputs: SIMD decoder and scalar decoder must agree 100%
        const fuzz_alpha: []const u8 = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/-_= \t\r\n\x00\xFF!";
        let f_iter: usize = 0;
        while (f_iter < 800) {
            const flen: usize = ((nextRng(&seed) % 96) + 1) as usize;
            let k2: usize = 0;
            while (k2 < flen) {
                const idx: usize = (nextRng(&seed) % (fuzz_alpha.len as u64)) as usize;
                raw_buf[k2] = fuzz_alpha[idx];
                k2 += 1;
            }
            const sample = raw_buf[0..flen];
            const fv = variants[f_iter % 4];

            const res_simd: usize? = tryDecodeSimdLen(&dec_simd, sample, fv);
            const res_scal: usize? = tryDecodeScalarLen(&dec_scal, sample, fv);
            expect_eq(res_simd != null, res_scal != null);
            if (ls := res_simd) {
                if (lc := res_scal) {
                    expect_eq(bytesEq(dec_simd[0..ls], dec_scal[0..lc]), true);
                }
            }
            f_iter += 1;
        }
    });
}
