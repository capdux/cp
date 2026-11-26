// cap/test/testCapTinyKeccak.cp
// Comprehensive unit tests for @cap/tiny-keccak (SHA3-256 & Keccak-256)

import { test, expect_eq } from "cap:test";
import {
    KECCAK256_DIGEST_LEN,
    KECCAK256_RATE,
    Sha3_256,
    Keccak256,
    sha3_256Hex,
    keccak256Hex,
} from "../../packages/@cap/tiny-keccak/src/index.cp";

fn kckTestHexMatches(actual: []u8, expected: string): bool {
    if (actual.len != expected.len) {
        return false;
    }
    let i: usize = 0;
    while (i < expected.len) {
        if (actual[i] != expected[i]) {
            return false;
        }
        i += 1;
    }
    return true;
}

fn main() {
    test("1. @cap/tiny-keccak — Official SHA3-256 & Keccak-256 standard vectors", () => {
        expect_eq(KECCAK256_DIGEST_LEN, 32 as usize);
        expect_eq(KECCAK256_RATE, 136 as usize);

        let hex: [64]u8 = undefined;

        expect_eq(sha3_256Hex("", hex[0..64]), 64 as usize);
        expect_eq(
            kckTestHexMatches(hex[0..64], "a7ffc6f8bf1ed76651c14756a061d662f580ff4de43b49fa82d80a4b80f8434a"),
            true
        );

        expect_eq(sha3_256Hex("abc", hex[0..64]), 64 as usize);
        expect_eq(
            kckTestHexMatches(hex[0..64], "3a985da74fe225b2045c172d6bd390bd855f086e3e9d525b46bfe24511431532"),
            true
        );

        expect_eq(keccak256Hex("", hex[0..64]), 64 as usize);
        expect_eq(
            kckTestHexMatches(hex[0..64], "c5d2460186f7233c927e7db2dcc703c0e500b653ca82273b7bfad8045d85a470"),
            true
        );

        expect_eq(keccak256Hex("abc", hex[0..64]), 64 as usize);
        expect_eq(
            kckTestHexMatches(hex[0..64], "4e03657aea45a94fc7d47ba826c8d667c0d1e6e33a64a036ec44f58fa12d6c45"),
            true
        );
    });

    test("2. @cap/tiny-keccak — FIPS 202 448-bit & Ethereum ERC-20 selector vectors", () => {
        let hex: [64]u8 = undefined;

        // FIPS 202 SHA3-256 56-byte vector
        expect_eq(
            sha3_256Hex("abcdbcdecdefdefgefghfghighijhijkijkljklmklmnlmnomnopnopq", hex[0..64]),
            64 as usize
        );
        expect_eq(
            kckTestHexMatches(hex[0..64], "41c0dba2a9d6240849100376a8235e2c82e1b9998a999e21db32dd97496d3376"),
            true
        );

        // Ethereum ERC-20 transfer(address,uint256) selector == 0xa9059cbb
        expect_eq(keccak256Hex("transfer(address,uint256)", hex[0..64]), 64 as usize);
        expect_eq(kckTestHexMatches(hex[0..8], "a9059cbb"), true);
    });

    test("3. @cap/tiny-keccak — Streaming chunked updateStr & single-byte update", () => {
        let hex: [64]u8 = undefined;

        let s3 = Sha3_256.new();
        s3.updateStr("abcdbcde");
        s3.updateStr("cdefdefgefghfghighij");
        s3.updateStr("hijkijkljklmklmnlmnomnopnopq");
        expect_eq(s3.finalizeHex(hex[0..64]), 64 as usize);
        expect_eq(
            kckTestHexMatches(hex[0..64], "41c0dba2a9d6240849100376a8235e2c82e1b9998a999e21db32dd97496d3376"),
            true
        );

        let k2 = Keccak256.new();
        let b0: [1]u8 = ['a' as u8];
        let b1: [1]u8 = ['b' as u8];
        let b2: [1]u8 = ['c' as u8];
        k2.update(b0[0..1]);
        k2.update(b1[0..1]);
        k2.update(b2[0..1]);
        expect_eq(k2.finalizeHex(hex[0..64]), 64 as usize);
        expect_eq(
            kckTestHexMatches(hex[0..64], "4e03657aea45a94fc7d47ba826c8d667c0d1e6e33a64a036ec44f58fa12d6c45"),
            true
        );
    });

    test("4. @cap/tiny-keccak — 136-byte rate boundary padding (135, 136, 137, 272 bytes)", () => {
        let data: [272]u8 = undefined;
        let i: usize = 0;
        while (i < (272 as usize)) {
            data[i] = ('a' as u8) + ((i % (26 as usize)) as u8);
            i += 1;
        }

        let d1: [32]u8 = undefined;
        let d2: [32]u8 = undefined;

        // 135 bytes: domain_sep and 0x80 merge on byte 135
        let s135a = Sha3_256.new();
        s135a.update(data[0..135]);
        s135a.finalize(d1[0..32]);

        let s135b = Sha3_256.new();
        s135b.update(data[0..70]);
        s135b.update(data[70..135]);
        s135b.finalize(d2[0..32]);
        expect_eq(d1[0], d2[0]);
        expect_eq(d1[31], d2[31]);

        // 136 bytes: exact rate absorb + extra padded block
        let k136a = Keccak256.new();
        k136a.update(data[0..136]);
        k136a.finalize(d1[0..32]);

        let k136b = Keccak256.new();
        k136b.update(data[0..100]);
        k136b.update(data[100..136]);
        k136b.finalize(d2[0..32]);
        expect_eq(d1[0], d2[0]);
        expect_eq(d1[31], d2[31]);

        // 272 bytes: two exact blocks
        let s272a = Sha3_256.new();
        s272a.update(data[0..272]);
        s272a.finalize(d1[0..32]);

        let s272b = Sha3_256.new();
        s272b.update(data[0..137]);
        s272b.update(data[137..272]);
        s272b.finalize(d2[0..32]);
        expect_eq(d1[0], d2[0]);
        expect_eq(d1[31], d2[31]);
    });

    test("5. @cap/tiny-keccak — Raw 32-byte digest & domain separation distinctness", () => {
        let s_out: [32]u8 = undefined;
        let k_out: [32]u8 = undefined;

        let s = Sha3_256.new();
        s.updateStr("abc");
        s.finalize(s_out[0..32]);

        let k = Keccak256.new();
        k.updateStr("abc");
        k.finalize(k_out[0..32]);

        // SHA3-256("abc") starts with 3a 98 5d a7 ... ends with 15 32
        expect_eq(s_out[0], 0x3A as u8);
        expect_eq(s_out[1], 0x98 as u8);
        expect_eq(s_out[30], 0x15 as u8);
        expect_eq(s_out[31], 0x32 as u8);

        // Keccak-256("abc") starts with 4e 03 65 7a ... ends with 6c 45
        expect_eq(k_out[0], 0x4E as u8);
        expect_eq(k_out[1], 0x03 as u8);
        expect_eq(k_out[30], 0x6C as u8);
        expect_eq(k_out[31], 0x45 as u8);
        expect_eq(s_out[0] != k_out[0], true);
    });

    test("6. @cap/tiny-keccak — Short output buffer safety", () => {
        let small: [32]u8 = undefined;
        expect_eq(sha3_256Hex("abc", small[0..32]), 0 as usize);
        expect_eq(keccak256Hex("abc", small[0..32]), 0 as usize);

        let tiny: [8]u8 = [1, 2, 3, 4, 5, 6, 7, 8];
        let s = Sha3_256.new();
        s.updateStr("abc");
        s.finalize(tiny[0..8]);
        expect_eq(tiny[0], 1 as u8);
        expect_eq(tiny[7], 8 as u8);
    });
}
