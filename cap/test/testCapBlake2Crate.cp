// cap/test/testCapBlake2Crate.cp
// Comprehensive unit tests for @cap/blake2-crate (RFC 7693 BLAKE2s-256)

import { test, expect_eq } from "cap:test";
import {
    BLAKE2S_DIGEST_LEN,
    BLAKE2S_BLOCK_LEN,
    Blake2s256,
    blake2s256Bytes,
    blake2s256Hex,
} from "../../packages/@cap/blake2-crate/src/index.cp";

fn bl2sTestHexMatches(actual: []u8, expected: string): bool {
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
    test("1. @cap/blake2-crate — RFC 7693 official unkeyed vectors", () => {
        expect_eq(BLAKE2S_DIGEST_LEN, 32 as usize);
        expect_eq(BLAKE2S_BLOCK_LEN, 64 as usize);

        let hex: [64]u8 = undefined;

        expect_eq(blake2s256Hex("", hex[0..64]), 64 as usize);
        expect_eq(
            bl2sTestHexMatches(hex[0..64], "69217a3079908094e11121d042354a7c1f55b6482ca1a51e1b250dfd1ed0eef9"),
            true
        );

        expect_eq(blake2s256Hex("abc", hex[0..64]), 64 as usize);
        expect_eq(
            bl2sTestHexMatches(hex[0..64], "508c5e8c327c14e2e1a72ba34eeb452f37458b209ed63a294d999b4c86675982"),
            true
        );
    });

    test("2. @cap/blake2-crate — Keyed MAC mode (empty & non-empty message)", () => {
        let key: [32]u8 = undefined;
        let i: usize = 0;
        while (i < (32 as usize)) {
            key[i] = i as u8;
            i += 1;
        }

        let hex: [64]u8 = undefined;

        let h0 = Blake2s256.newKeyed(key[0..32]).?;
        expect_eq(h0.finalizeHex(hex[0..64]), 64 as usize);
        expect_eq(
            bl2sTestHexMatches(hex[0..64], "48a8997da407876b3d79c0d92325ad3b89cbb754d86ab71aee047ad345fd2c49"),
            true
        );

        let h1 = Blake2s256.newKeyed(key[0..32]).?;
        h1.updateStr("abc");
        expect_eq(h1.finalizeHex(hex[0..64]), 64 as usize);
        expect_eq(
            bl2sTestHexMatches(hex[0..64], "a281f725754969a702f6fe36fc591b7def866e4b70173ece402fc01c064d6b65"),
            true
        );
    });

    test("3. @cap/blake2-crate — Exact 64-byte & 128-byte block boundary vectors", () => {
        let data: [128]u8 = undefined;
        let i: usize = 0;
        while (i < (128 as usize)) {
            data[i] = ('a' as u8) + ((i % (26 as usize)) as u8);
            i += 1;
        }

        let hex: [64]u8 = undefined;

        let h64 = Blake2s256.new();
        h64.update(data[0..64]);
        expect_eq(h64.finalizeHex(hex[0..64]), 64 as usize);
        expect_eq(
            bl2sTestHexMatches(hex[0..64], "9b7edfe7981d0971e3bf1486b943dd23f27cf80e2f3693834768df4df51f7561"),
            true
        );

        let h128 = Blake2s256.new();
        h128.update(data[0..128]);
        expect_eq(h128.finalizeHex(hex[0..64]), 64 as usize);
        expect_eq(
            bl2sTestHexMatches(hex[0..64], "07b3b30461c70e17ba3a770be023c9942483ecd0d103d4b3b202b3e533d1b685"),
            true
        );
    });

    test("4. @cap/blake2-crate — Streaming chunked update matches monolithic", () => {
        let data: [128]u8 = undefined;
        let i: usize = 0;
        while (i < (128 as usize)) {
            data[i] = ('a' as u8) + ((i % (26 as usize)) as u8);
            i += 1;
        }

        let hex: [64]u8 = undefined;
        let h = Blake2s256.new();
        h.update(data[0..1]);
        h.update(data[1..33]);
        h.update(data[33..64]);
        h.update(data[64..95]);
        h.update(data[95..128]);
        expect_eq(h.finalizeHex(hex[0..64]), 64 as usize);
        expect_eq(
            bl2sTestHexMatches(hex[0..64], "07b3b30461c70e17ba3a770be023c9942483ecd0d103d4b3b202b3e533d1b685"),
            true
        );
    });

    test("5. @cap/blake2-crate — Raw 32-byte digest via blake2s256Bytes", () => {
        let raw_abc: [3]u8 = ['a' as u8, 'b' as u8, 'c' as u8];
        let out: [32]u8 = undefined;
        blake2s256Bytes(raw_abc[0..3], out[0..32]);

        // 508c5e8c ... 6675982
        expect_eq(out[0], 0x50 as u8);
        expect_eq(out[1], 0x8C as u8);
        expect_eq(out[2], 0x5E as u8);
        expect_eq(out[3], 0x8C as u8);
        expect_eq(out[30], 0x59 as u8);
        expect_eq(out[31], 0x82 as u8);
    });

    test("6. @cap/blake2-crate — Invalid key (>32B), empty key & short buffer safety", () => {
        let too_long: [33]u8 = undefined;
        expect_eq(Blake2s256.newKeyed(too_long[0..33]) == null, true);

        // 0-length key behaves identically to unkeyed new()
        let empty_key: [1]u8 = [0 as u8];
        let h = Blake2s256.newKeyed(empty_key[0..0]).?;
        h.updateStr("abc");
        let hex: [64]u8 = undefined;
        expect_eq(h.finalizeHex(hex[0..64]), 64 as usize);
        expect_eq(
            bl2sTestHexMatches(hex[0..64], "508c5e8c327c14e2e1a72ba34eeb452f37458b209ed63a294d999b4c86675982"),
            true
        );

        let small: [16]u8 = undefined;
        expect_eq(blake2s256Hex("abc", small[0..16]), 0 as usize);
    });
}
