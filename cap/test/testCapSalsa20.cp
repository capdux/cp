import { test, expect_eq } from "cap:test";
import {
    salsa20Block,
    salsa20Apply,
} from "../../packages/salsa20/src/index.cp";

fn main() {
    test("1. @cap/salsa20 — matches official eSTREAM Set 1 Vector 0 (1-bit key, zero nonce)", () => {
        // eSTREAM verified vector: key = [0x80, 0..0], nonce = [0..0], counter = 0
        // Expected first 8 bytes: E3 BE 8F DD 8B EC A2 E3
        let key: [32]u8 = [0; 32];
        key[0] = 0x80;
        const nonce: [8]u8 = [0; 8];
        let blk: [64]u8 = [0; 64];
        salsa20Block(key[0..32], nonce[0..8], 0, blk[0..64]);
        expect_eq(blk[0], 0xE3 as u8);
        expect_eq(blk[1], 0xBE as u8);
        expect_eq(blk[2], 0x8F as u8);
        expect_eq(blk[3], 0xDD as u8);
        expect_eq(blk[4], 0x8B as u8);
        expect_eq(blk[5], 0xEC as u8);
        expect_eq(blk[6], 0xA2 as u8);
        expect_eq(blk[7], 0xE3 as u8);
    });

    test("2. @cap/salsa20 — encrypts and decrypts multi-block payload cleanly", () => {
        let key: [32]u8 = [0; 32];
        let i: usize = 0;
        while (i < 32) { key[i] = (i * 7 + 3) as u8; i += 1; }
        const nonce: [8]u8 = [1, 2, 3, 4, 5, 6, 7, 8];
        const msg = "The quick brown fox jumps over the lazy dog! Salsa20/20 multi-block stream test 1234567890.";
        let ct: [128]u8 = [0; 128];
        let pt: [128]u8 = [0; 128];

        const n1 = salsa20Apply(key[0..32], nonce[0..8], 0, msg, ct[0..128]);
        expect_eq(n1, msg.len);
        expect_eq(ct[0] != msg[0] || ct[1] != msg[1], true);

        const n2 = salsa20Apply(key[0..32], nonce[0..8], 0, ct[0..n1], pt[0..128]);
        expect_eq(n2, msg.len);
        expect_eq(pt[0..n2] as string, msg);
    });

    test("3. @cap/salsa20 — counter seek produces identical keystream as block 1", () => {
        let key: [32]u8 = [0x42; 32];
        const nonce: [8]u8 = [9, 8, 7, 6, 5, 4, 3, 2];
        const zeros: [128]u8 = [0; 128];
        let stream: [128]u8 = [0; 128];
        * = salsa20Apply(key[0..32], nonce[0..8], 0, zeros[0..128], stream[0..128]);

        let blk1: [64]u8 = [0; 64];
        salsa20Block(key[0..32], nonce[0..8], 1, blk1[0..64]);
        expect_eq(stream[64], blk1[0]);
        expect_eq(stream[127], blk1[63]);
    });

    test("4. @cap/salsa20 — different nonces produce completely different keystreams", () => {
        const key: [32]u8 = [0x11; 32];
        const n1: [8]u8 = [0, 0, 0, 0, 0, 0, 0, 1];
        const n2: [8]u8 = [0, 0, 0, 0, 0, 0, 0, 2];
        let b1: [64]u8 = [0; 64];
        let b2: [64]u8 = [0; 64];
        salsa20Block(key[0..32], n1[0..8], 0, b1[0..64]);
        salsa20Block(key[0..32], n2[0..8], 0, b2[0..64]);
        expect_eq(b1[0] != b2[0] || b1[1] != b2[1], true);
    });

    test("5. @cap/salsa20 — handles empty input safely", () => {
        const key: [32]u8 = [0; 32];
        const nonce: [8]u8 = [0; 8];
        let out: [16]u8 = [0; 16];
        expect_eq(salsa20Apply(key[0..32], nonce[0..8], 0, "", out[0..16]), 0 as usize);
    });

    test("6. @cap/salsa20 — rejects short key or short nonce safely", () => {
        const short_k: [16]u8 = [0; 16];
        const nonce: [8]u8 = [0; 8];
        let out: [16]u8 = [0; 16];
        expect_eq(salsa20Apply(short_k[0..16], nonce[0..8], 0, "hi", out[0..16]), 0 as usize);
    });
}
