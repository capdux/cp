import { test, expect_eq } from "cap:test";
import { MAX_BLOCK, bwtForward, bwtInverse, mtfEncode, mtfDecode } from "../../packages/bwt/src/index.cp";

fn main() {
    test("1. @cap/bwt — Classic BANANA -> NNBAAA with primary_index = 3", () => {
        const src: [6]u8 = [66, 65, 78, 65, 78, 65]; // "BANANA"
        let bwt_buf: [6]u8 = undefined;
        let restored: [6]u8 = undefined;

        const res = bwtForward(src[0..6], bwt_buf[0..6]);
        expect_eq(res.ok, true);
        expect_eq(res.len, 6 as usize);
        expect_eq(res.primary_index, 3 as usize);
        // "NNBAAA" = [78, 78, 66, 65, 65, 65]
        expect_eq(bwt_buf[0], 78 as u8);
        expect_eq(bwt_buf[1], 78 as u8);
        expect_eq(bwt_buf[2], 66 as u8);
        expect_eq(bwt_buf[3], 65 as u8);
        expect_eq(bwt_buf[4], 65 as u8);
        expect_eq(bwt_buf[5], 65 as u8);

        expect_eq(bwtInverse(bwt_buf[0..6], res.primary_index, restored[0..6]), true);
        let i: usize = 0;
        while (i < 6) {
            expect_eq(restored[i], src[i]);
            i += 1;
        }
    });

    test("2. @cap/bwt — mississippi & pseudo-random binary stream roundtrip", () => {
        // "mississippi"
        const word: [11]u8 = [109, 105, 115, 115, 105, 115, 115, 105, 112, 112, 105];
        let bwt1: [11]u8 = undefined;
        let dec1: [11]u8 = undefined;
        const r1 = bwtForward(word[0..11], bwt1[0..11]);
        expect_eq(r1.ok, true);
        expect_eq(bwtInverse(bwt1[0..11], r1.primary_index, dec1[0..11]), true);
        let i: usize = 0;
        while (i < 11) {
            expect_eq(dec1[i], word[i]);
            i += 1;
        }

        // 128-byte deterministic pseudo-random binary payload (all byte values 0..255)
        let bin: [128]u8 = undefined;
        let state: u32 = 0x12345678;
        let j: usize = 0;
        while (j < 128) {
            state = state *% 1664525 +% 1013904223;
            bin[j] = ((state >> 24) & 0xFF) as u8;
            j += 1;
        }
        let bwt2: [128]u8 = undefined;
        let dec2: [128]u8 = undefined;
        const r2 = bwtForward(bin[0..128], bwt2[0..128]);
        expect_eq(r2.ok, true);
        expect_eq(r2.len, 128 as usize);
        expect_eq(bwtInverse(bwt2[0..128], r2.primary_index, dec2[0..128]), true);
        let k: usize = 0;
        while (k < 128) {
            expect_eq(dec2[k], bin[k]);
            k += 1;
        }
    });

    test("3. @cap/bwt — Empty & single-byte edge cases", () => {
        let empty_src: [1]u8 = [0];
        let empty_dst: [1]u8 = undefined;
        const r0 = bwtForward(empty_src[0..0], empty_dst[0..0]);
        expect_eq(r0.ok, true);
        expect_eq(r0.len, 0 as usize);
        expect_eq(r0.primary_index, 0 as usize);
        expect_eq(bwtInverse(empty_src[0..0], 0, empty_dst[0..0]), true);

        const single_src: [1]u8 = [0xAB];
        let single_bwt: [1]u8 = undefined;
        let single_dec: [1]u8 = undefined;
        const r1 = bwtForward(single_src[0..1], single_bwt[0..1]);
        expect_eq(r1.ok, true);
        expect_eq(r1.primary_index, 0 as usize);
        expect_eq(single_bwt[0], 0xAB as u8);
        expect_eq(bwtInverse(single_bwt[0..1], 0, single_dec[0..1]), true);
        expect_eq(single_dec[0], 0xAB as u8);
    });

    test("4. @cap/bwt — MTF run clustering & exact decode", () => {
        // "NNBAAA" -> repeated bytes produce 0s in MTF output
        const input: [6]u8 = [78, 78, 66, 65, 65, 65];
        let encoded: [6]u8 = undefined;
        let decoded: [6]u8 = undefined;

        expect_eq(mtfEncode(input[0..6], encoded[0..6]), true);
        expect_eq(encoded[0], 78 as u8); // 'N' at 78
        expect_eq(encoded[1], 0 as u8);  // second 'N' at front (0)
        expect_eq(encoded[2], 67 as u8); // 'B' shifted by 1 ('N' moved before 'B')
        expect_eq(encoded[3], 67 as u8); // 'A' shifted by 2 ('N','B' moved before 'A')
        expect_eq(encoded[4], 0 as u8);  // second 'A' at front (0)
        expect_eq(encoded[5], 0 as u8);  // third 'A' at front (0)

        expect_eq(mtfDecode(encoded[0..6], decoded[0..6]), true);
        let i: usize = 0;
        while (i < 6) {
            expect_eq(decoded[i], input[i]);
            i += 1;
        }
    });

    test("5. @cap/bwt — Full BWT + MTF cascaded pipeline roundtrip", () => {
        // Repetitive text: "abracadabra!abracadabra!abracadabra!" (36 bytes)
        const pattern: [12]u8 = [97, 98, 114, 97, 99, 97, 100, 97, 98, 114, 97, 33];
        let src: [36]u8 = undefined;
        let p: usize = 0;
        while (p < 36) {
            src[p] = pattern[p % 12];
            p += 1;
        }

        let bwt_out: [36]u8 = undefined;
        let mtf_out: [36]u8 = undefined;
        let mtf_dec: [36]u8 = undefined;
        let final_out: [36]u8 = undefined;

        const bres = bwtForward(src[0..36], bwt_out[0..36]);
        expect_eq(bres.ok, true);
        expect_eq(mtfEncode(bwt_out[0..36], mtf_out[0..36]), true);

        // Verify high zero density after BWT + MTF
        let zero_count: usize = 0;
        let z: usize = 0;
        while (z < 36) {
            if (mtf_out[z] == 0) {
                zero_count += 1;
            }
            z += 1;
        }
        expect_eq(zero_count >= 20, true);

        // Reverse pipeline: MTF decode -> BWT inverse
        expect_eq(mtfDecode(mtf_out[0..36], mtf_dec[0..36]), true);
        expect_eq(bwtInverse(mtf_dec[0..36], bres.primary_index, final_out[0..36]), true);
        let i: usize = 0;
        while (i < 36) {
            expect_eq(final_out[i], src[i]);
            i += 1;
        }
    });

    test("6. @cap/bwt — Oversized > 512 block, undersized dst & invalid primary_index rejection", () => {
        expect_eq(MAX_BLOCK, 512 as usize);
        let big: [513]u8 = undefined;
        let big_dst: [513]u8 = undefined;
        let i: usize = 0;
        while (i < 513) {
            big[i] = (i & 0xFF) as u8;
            i += 1;
        }
        const over = bwtForward(big[0..513], big_dst[0..513]);
        expect_eq(over.ok, false);
        expect_eq(bwtInverse(big[0..513], 0, big_dst[0..513]), false);

        // Undersized destination
        const small: [4]u8 = [1, 2, 3, 4];
        let tiny_dst: [2]u8 = undefined;
        const under = bwtForward(small[0..4], tiny_dst[0..2]);
        expect_eq(under.ok, false);
        expect_eq(mtfEncode(small[0..4], tiny_dst[0..2]), false);
        expect_eq(mtfDecode(small[0..4], tiny_dst[0..2]), false);

        // Invalid primary_index (>= len)
        let ok_dst: [4]u8 = undefined;
        expect_eq(bwtInverse(small[0..4], 4, ok_dst[0..4]), false);
        expect_eq(bwtInverse(small[0..4], 99, ok_dst[0..4]), false);
    });
}
