import { test, expect_eq } from "cap:test";
import { HkdfSha256, hkdfSha256 } from "../../packages/hkdf/src/index.cp";

fn hkHexByte(hi: u8): u8 {
    if (hi < 10) { return 48 + hi; }
    return 87 + hi;
}

fn hkToHex(data: []u8, out: []u8): usize {
    let i: usize = 0;
    while (i < data.len && i * 2 + 1 < out.len) {
        out[i * 2] = hkHexByte((data[i] >> 4) & 15);
        out[i * 2 + 1] = hkHexByte(data[i] & 15);
        i = i + 1;
    }
    return i * 2;
}

fn main() {
    test("1. @cap/hkdf-crate — RFC 5869 Test Case 1 PRK & 42-byte OKM", () => {
        let ikm: [22]u8 = [11; 22]; // 0x0b * 22
        let salt: [13]u8 = [0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12];
        let info: [10]u8 = [240, 241, 242, 243, 244, 245, 246, 247, 248, 249];
        let hk = HkdfSha256.extract(salt[0..13], ikm[0..22]);

        let prk_hex: [64]u8 = [0; 64];
        * = hkToHex(hk.prk[0..32], prk_hex[0..64]);
        expect_eq(prk_hex[0..64] as string, "077709362c2e32df0ddc3f0dc47bba6390b6c73bb50f9c3122ec844ad7c2b3e5");

        let okm: [42]u8 = [0; 42];
        expect_eq(hk.expand(info[0..10], okm[0..42]), true);
        let okm_hex: [84]u8 = [0; 84];
        * = hkToHex(okm[0..42], okm_hex[0..84]);
        expect_eq(okm_hex[0..84] as string, "3cb25f25faacd57a90434f64d0362f2a2d2d0a90cf1a5a4c5db02d56ecc4c5bf34007208d5b887185865");
    });

    test("2. @cap/hkdf-crate — RFC 5869 Test Case 3 (empty salt & empty info)", () => {
        let ikm: [22]u8 = [11; 22];
        let empty: [1]u8 = [0];
        let okm: [42]u8 = [0; 42];
        expect_eq(hkdfSha256(empty[0..0], ikm[0..22], empty[0..0], okm[0..42]), true);
        let okm_hex: [84]u8 = [0; 84];
        * = hkToHex(okm[0..42], okm_hex[0..84]);
        expect_eq(okm_hex[0..84] as string, "8da4e775a563c18f715f802a063c5a31b8a11f5c5ee1879ec3454e5f3c738d2d9d201395faa4b61a96c8");
    });

    test("3. @cap/hkdf-crate — fromPrk rejects short PRK and accepts 32-byte PRK", () => {
        let short_prk: [16]u8 = [1; 16];
        expect_eq(HkdfSha256.fromPrk(short_prk[0..16]) == null, true);

        let full_prk: [32]u8 = [7; 32];
        let hk_opt = HkdfSha256.fromPrk(full_prk[0..32]);
        expect_eq(hk_opt != null, true);
    });

    test("4. @cap/hkdf-crate — domain separation: different info produces different keys", () => {
        let ikm: [8]u8 = [1, 2, 3, 4, 5, 6, 7, 8];
        let salt: [4]u8 = [9, 9, 9, 9];
        let info1: [3]u8 = [101, 110, 99]; // "enc"
        let info2: [3]u8 = [109, 97, 99];  // "mac"
        let k1: [32]u8 = [0; 32];
        let k2: [32]u8 = [0; 32];
        * = hkdfSha256(salt[0..4], ikm[0..8], info1[0..3], k1[0..32]);
        * = hkdfSha256(salt[0..4], ikm[0..8], info2[0..3], k2[0..32]);
        expect_eq(k1[0] == k2[0] && k1[1] == k2[1], false);
    });

    test("5. @cap/hkdf-crate — zero-length OKM succeeds as no-op", () => {
        let ikm: [4]u8 = [1, 2, 3, 4];
        let hk = HkdfSha256.extract(ikm[0..0], ikm[0..4]);
        let okm: [1]u8 = [99];
        expect_eq(hk.expand(ikm[0..0], okm[0..0]), true);
        expect_eq(okm[0], 99);
    });

    test("6. @cap/hkdf-crate — multi-block expansion (>64 bytes) is prefix-consistent", () => {
        let ikm: [4]u8 = [5, 6, 7, 8];
        let hk = HkdfSha256.extract(ikm[0..0], ikm[0..4]);
        let k32: [32]u8 = [0; 32];
        let k80: [80]u8 = [0; 80];
        * = hk.expand(ikm[0..4], k32[0..32]);
        * = hk.expand(ikm[0..4], k80[0..80]);
        expect_eq(k32[0], k80[0]);
        expect_eq(k32[31], k80[31]);
    });
}
