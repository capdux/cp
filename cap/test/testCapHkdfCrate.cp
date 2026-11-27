import { test, expect_eq } from "cap:test";
import {
    HkdfSha256,
    HkdfSha384,
    HkdfSha512,
    HkdfHashAlg,
    HKDF_SHA256_HASH_LEN,
    HKDF_SHA384_HASH_LEN,
    HKDF_SHA512_HASH_LEN,
    hkdfSha256,
    hkdfSha384,
    hkdfSha512,
    hkdfExtract,
    hkdfExpand,
    hkdfDerive,
} from "../../packages/hkdf/src/index.cp";

fn hkBytesEq(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) { return false; }
    let i: usize = 0;
    while (i < a.len) {
        if (a[i] != b[i]) { return false; }
        i = i + 1;
    }
    return true;
}

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

    test("7. @cap/hkdf-crate — expandMultiInfo matches single concatenated info", () => {
        let ikm: [8]u8 = [1, 2, 3, 4, 5, 6, 7, 8];
        let salt: [4]u8 = [10, 20, 30, 40];
        let hk = HkdfSha256.extract(salt[0..4], ikm[0..8]);
        let full_info: [9]u8 = ['a', 'b', 'c', '-', 'd', 'e', 'f', 'g', 'h'];
        let parts: [3]string = ["abc", "-", "defgh"];
        let out1: [48]u8 = undefined;
        let out2: [48]u8 = undefined;
        expect_eq(hk.expand(full_info[0..9], out1[0..48]), true);
        expect_eq(hk.expandMultiInfo(parts[0..3], out2[0..48]), true);
        expect_eq(hkBytesEq(out1[0..48], out2[0..48]), true);
        expect_eq(hk.verifyPrk(hk.prk[0..32]), true);
    });

    test("8. @cap/hkdf-crate — TLS 1.3 expandLabel & boundary checks", () => {
        let ikm: [16]u8 = [0xaa; 16];
        let salt: [16]u8 = [0xbb; 16];
        let hk = HkdfSha256.extract(salt[0..16], ikm[0..16]);
        let key1: [32]u8 = undefined;
        let key2: [32]u8 = undefined;
        expect_eq(hk.expandLabel("key", "ctx", key1[0..32]), true);
        expect_eq(hk.expandLabel("iv", "ctx", key2[0..32]), true);
        expect_eq(hkBytesEq(key1[0..32], key2[0..32]), false);
    });

    test("9. @cap/hkdf-crate — HkdfSha384 & HkdfSha512 extract/expand/derive", () => {
        let ikm = "input-keying-material";
        let salt = "salt-bytes";
        let info = "app-context";
        let okm384_a: [64]u8 = undefined;
        let okm384_b: [64]u8 = undefined;
        let h384 = HkdfSha384.extract(salt, ikm);
        expect_eq(h384.expand(info, okm384_a[0..64]), true);
        expect_eq(hkdfSha384(salt, ikm, info, okm384_b[0..64]), true);
        expect_eq(hkBytesEq(okm384_a[0..64], okm384_b[0..64]), true);
        expect_eq(HkdfSha384.fromPrk(h384.prk[0..47]) == null, true);
        expect_eq(HkdfSha384.fromPrk(h384.prk[0..48]) != null, true);

        let okm512_a: [64]u8 = undefined;
        let okm512_b: [64]u8 = undefined;
        let h512 = HkdfSha512.extract(salt, ikm);
        expect_eq(h512.expand(info, okm512_a[0..64]), true);
        expect_eq(hkdfSha512(salt, ikm, info, okm512_b[0..64]), true);
        expect_eq(hkBytesEq(okm512_a[0..64], okm512_b[0..64]), true);
        expect_eq(HkdfSha512.fromPrk(h512.prk[0..63]) == null, true);
    });

    test("10. @cap/hkdf-crate — Unified HkdfHashAlg extract/expand/derive & overflow guards", () => {
        expect_eq(HkdfHashAlg.Sha256.hashLen(), HKDF_SHA256_HASH_LEN);
        expect_eq(HkdfHashAlg.Sha384.hashLen(), HKDF_SHA384_HASH_LEN);
        expect_eq(HkdfHashAlg.Sha512.hashLen(), HKDF_SHA512_HASH_LEN);

        let prk: [64]u8 = undefined;
        expect_eq(hkdfExtract(HkdfHashAlg.Sha256, "s", "k", prk[0..16]), 0 as usize);
        expect_eq(hkdfExtract(HkdfHashAlg.Sha256, "s", "k", prk[0..64]), 32 as usize);
        expect_eq(hkdfExtract(HkdfHashAlg.Sha384, "s", "k", prk[0..64]), 48 as usize);
        expect_eq(hkdfExtract(HkdfHashAlg.Sha512, "s", "k", prk[0..64]), 64 as usize);

        let out1: [42]u8 = undefined;
        let out2: [42]u8 = undefined;
        expect_eq(hkdfExpand(HkdfHashAlg.Sha512, prk[0..64], "i", out1[0..42]), true);
        expect_eq(hkdfDerive(HkdfHashAlg.Sha512, "s", "k", "i", out2[0..42]), true);
        expect_eq(hkBytesEq(out1[0..42], out2[0..42]), true);

        // Short PRK rejected
        expect_eq(hkdfExpand(HkdfHashAlg.Sha512, prk[0..32], "i", out1[0..42]), false);
    });
}
