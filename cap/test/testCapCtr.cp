import { test, expect_eq } from "cap:test";
import {
    Aes128Ctr128Be,
    Aes128Ctr128Le,
    Aes256Ctr128Be,
    Aes256Ctr128Le,
} from "../../packages/ctr/src/index.cp";

fn main() {
    test("1. @cap/ctr — NIST SP 800-38A F.5.1 AES-128-CTR official 64-byte vector", () => {
        const key: [16]u8 = [
            0x2b as u8, 0x7e as u8, 0x15 as u8, 0x16 as u8,
            0x28 as u8, 0xae as u8, 0xd2 as u8, 0xa6 as u8,
            0xab as u8, 0xf7 as u8, 0x15 as u8, 0x88 as u8,
            0x09 as u8, 0xcf as u8, 0x4f as u8, 0x3c as u8,
        ];
        const iv: [16]u8 = [
            0xf0 as u8, 0xf1 as u8, 0xf2 as u8, 0xf3 as u8,
            0xf4 as u8, 0xf5 as u8, 0xf6 as u8, 0xf7 as u8,
            0xf8 as u8, 0xf9 as u8, 0xfa as u8, 0xfb as u8,
            0xfc as u8, 0xfd as u8, 0xfe as u8, 0xff as u8,
        ];
        let buf: [64]u8 = [
            0x6b as u8, 0xc1 as u8, 0xbe as u8, 0xe2 as u8,
            0x2e as u8, 0x40 as u8, 0x9f as u8, 0x96 as u8,
            0xe9 as u8, 0x3d as u8, 0x7e as u8, 0x11 as u8,
            0x73 as u8, 0x93 as u8, 0x17 as u8, 0x2a as u8,
            0xae as u8, 0x2d as u8, 0x8a as u8, 0x57 as u8,
            0x1e as u8, 0x03 as u8, 0xac as u8, 0x9c as u8,
            0x9e as u8, 0xb7 as u8, 0x6f as u8, 0xac as u8,
            0x45 as u8, 0xaf as u8, 0x8e as u8, 0x51 as u8,
            0x30 as u8, 0xc8 as u8, 0x1c as u8, 0x46 as u8,
            0xa3 as u8, 0x5c as u8, 0xe4 as u8, 0x11 as u8,
            0xe5 as u8, 0xfb as u8, 0xc1 as u8, 0x19 as u8,
            0x1a as u8, 0x0a as u8, 0x52 as u8, 0xef as u8,
            0xf6 as u8, 0x9f as u8, 0x24 as u8, 0x45 as u8,
            0xdf as u8, 0x4f as u8, 0x9b as u8, 0x17 as u8,
            0xad as u8, 0x2b as u8, 0x41 as u8, 0x7b as u8,
            0xe6 as u8, 0x6c as u8, 0x37 as u8, 0x10 as u8,
        ];
        const expected: [64]u8 = [
            0x87 as u8, 0x4d as u8, 0x61 as u8, 0x91 as u8,
            0xb6 as u8, 0x20 as u8, 0xe3 as u8, 0x26 as u8,
            0x1b as u8, 0xef as u8, 0x68 as u8, 0x64 as u8,
            0x99 as u8, 0x0d as u8, 0xb6 as u8, 0xce as u8,
            0x98 as u8, 0x06 as u8, 0xf6 as u8, 0x6b as u8,
            0x79 as u8, 0x70 as u8, 0xfd as u8, 0xff as u8,
            0x86 as u8, 0x17 as u8, 0x18 as u8, 0x7b as u8,
            0xb9 as u8, 0xff as u8, 0xfd as u8, 0xff as u8,
            0x5a as u8, 0xe4 as u8, 0xdf as u8, 0x3e as u8,
            0xdb as u8, 0xd5 as u8, 0xd3 as u8, 0x5e as u8,
            0x5b as u8, 0x4f as u8, 0x09 as u8, 0x02 as u8,
            0x0d as u8, 0xb0 as u8, 0x3e as u8, 0xab as u8,
            0x1e as u8, 0x03 as u8, 0x1d as u8, 0xda as u8,
            0x2f as u8, 0xbe as u8, 0x03 as u8, 0xd1 as u8,
            0x79 as u8, 0x21 as u8, 0x70 as u8, 0xa0 as u8,
            0xf3 as u8, 0x00 as u8, 0x9c as u8, 0xee as u8,
        ];

        let c: Aes128Ctr128Be = Aes128Ctr128Be.new(key, iv);
        c.applyKeystream(buf[0..64]);
        let i: usize = 0;
        while (i < 64) {
            expect_eq(buf[i], expected[i]);
            i += 1;
        }
        expect_eq(c.currentPos(), 64 as u64);
    });

    test("2. @cap/ctr — NIST SP 800-38A F.5.5 AES-256-CTR official 64-byte vector", () => {
        const key: [32]u8 = [
            0x60 as u8, 0x3d as u8, 0xeb as u8, 0x10 as u8,
            0x15 as u8, 0xca as u8, 0x71 as u8, 0xbe as u8,
            0x2b as u8, 0x73 as u8, 0xae as u8, 0xf0 as u8,
            0x85 as u8, 0x7d as u8, 0x77 as u8, 0x81 as u8,
            0x1f as u8, 0x35 as u8, 0x2c as u8, 0x07 as u8,
            0x3b as u8, 0x61 as u8, 0x08 as u8, 0xd7 as u8,
            0x2d as u8, 0x98 as u8, 0x10 as u8, 0xa3 as u8,
            0x09 as u8, 0x14 as u8, 0xdf as u8, 0xf4 as u8,
        ];
        const iv: [16]u8 = [
            0xf0 as u8, 0xf1 as u8, 0xf2 as u8, 0xf3 as u8,
            0xf4 as u8, 0xf5 as u8, 0xf6 as u8, 0xf7 as u8,
            0xf8 as u8, 0xf9 as u8, 0xfa as u8, 0xfb as u8,
            0xfc as u8, 0xfd as u8, 0xfe as u8, 0xff as u8,
        ];
        let buf: [64]u8 = [
            0x6b as u8, 0xc1 as u8, 0xbe as u8, 0xe2 as u8,
            0x2e as u8, 0x40 as u8, 0x9f as u8, 0x96 as u8,
            0xe9 as u8, 0x3d as u8, 0x7e as u8, 0x11 as u8,
            0x73 as u8, 0x93 as u8, 0x17 as u8, 0x2a as u8,
            0xae as u8, 0x2d as u8, 0x8a as u8, 0x57 as u8,
            0x1e as u8, 0x03 as u8, 0xac as u8, 0x9c as u8,
            0x9e as u8, 0xb7 as u8, 0x6f as u8, 0xac as u8,
            0x45 as u8, 0xaf as u8, 0x8e as u8, 0x51 as u8,
            0x30 as u8, 0xc8 as u8, 0x1c as u8, 0x46 as u8,
            0xa3 as u8, 0x5c as u8, 0xe4 as u8, 0x11 as u8,
            0xe5 as u8, 0xfb as u8, 0xc1 as u8, 0x19 as u8,
            0x1a as u8, 0x0a as u8, 0x52 as u8, 0xef as u8,
            0xf6 as u8, 0x9f as u8, 0x24 as u8, 0x45 as u8,
            0xdf as u8, 0x4f as u8, 0x9b as u8, 0x17 as u8,
            0xad as u8, 0x2b as u8, 0x41 as u8, 0x7b as u8,
            0xe6 as u8, 0x6c as u8, 0x37 as u8, 0x10 as u8,
        ];
        const expected: [64]u8 = [
            0x60 as u8, 0x1e as u8, 0xc3 as u8, 0x13 as u8,
            0x77 as u8, 0x57 as u8, 0x89 as u8, 0xa5 as u8,
            0xb7 as u8, 0xa7 as u8, 0xf5 as u8, 0x04 as u8,
            0xbb as u8, 0xf3 as u8, 0xd2 as u8, 0x28 as u8,
            0xf4 as u8, 0x43 as u8, 0xe3 as u8, 0xca as u8,
            0x4d as u8, 0x62 as u8, 0xb5 as u8, 0x9a as u8,
            0xca as u8, 0x84 as u8, 0xe9 as u8, 0x90 as u8,
            0xca as u8, 0xca as u8, 0xf5 as u8, 0xc5 as u8,
            0x2b as u8, 0x09 as u8, 0x30 as u8, 0xda as u8,
            0xa2 as u8, 0x3d as u8, 0xe9 as u8, 0x4c as u8,
            0xe8 as u8, 0x70 as u8, 0x17 as u8, 0xba as u8,
            0x2d as u8, 0x84 as u8, 0x98 as u8, 0x8d as u8,
            0xdf as u8, 0xc9 as u8, 0xc5 as u8, 0x8d as u8,
            0xb6 as u8, 0x7a as u8, 0xad as u8, 0xa6 as u8,
            0x13 as u8, 0xc2 as u8, 0xdd as u8, 0x08 as u8,
            0x45 as u8, 0x79 as u8, 0x41 as u8, 0xa6 as u8,
        ];

        let c: Aes256Ctr128Be = Aes256Ctr128Be.new(key, iv);
        c.applyKeystream(buf[0..64]);
        let i: usize = 0;
        while (i < 64) {
            expect_eq(buf[i], expected[i]);
            i += 1;
        }
    });

    test("3. @cap/ctr — chunked streaming across non-16-byte boundaries matches one-shot", () => {
        const key: [16]u8 = [
            0x2b as u8, 0x7e as u8, 0x15 as u8, 0x16 as u8,
            0x28 as u8, 0xae as u8, 0xd2 as u8, 0xa6 as u8,
            0xab as u8, 0xf7 as u8, 0x15 as u8, 0x88 as u8,
            0x09 as u8, 0xcf as u8, 0x4f as u8, 0x3c as u8,
        ];
        const iv: [16]u8 = [
            0xf0 as u8, 0xf1 as u8, 0xf2 as u8, 0xf3 as u8,
            0xf4 as u8, 0xf5 as u8, 0xf6 as u8, 0xf7 as u8,
            0xf8 as u8, 0xf9 as u8, 0xfa as u8, 0xfb as u8,
            0xfc as u8, 0xfd as u8, 0xfe as u8, 0xff as u8,
        ];
        let oneshot: [40]u8 = [0x42 as u8; 40];
        let chunked: [40]u8 = [0x42 as u8; 40];

        let c1: Aes128Ctr128Be = Aes128Ctr128Be.new(key, iv);
        c1.applyKeystream(oneshot[0..40]);

        let c2: Aes128Ctr128Be = Aes128Ctr128Be.new(key, iv);
        c2.applyKeystream(chunked[0..5]);
        expect_eq(c2.currentPos(), 5 as u64);
        c2.applyKeystream(chunked[5..12]);
        expect_eq(c2.currentPos(), 12 as u64);
        c2.applyKeystream(chunked[12..29]);
        expect_eq(c2.currentPos(), 29 as u64);
        c2.applyKeystream(chunked[29..40]);
        expect_eq(c2.currentPos(), 40 as u64);

        let i: usize = 0;
        while (i < 40) {
            expect_eq(chunked[i], oneshot[i]);
            i += 1;
        }
    });

    test("4. @cap/ctr — O(1) seek() to arbitrary byte offset matches sequential keystream", () => {
        const key: [16]u8 = [
            0x2b as u8, 0x7e as u8, 0x15 as u8, 0x16 as u8,
            0x28 as u8, 0xae as u8, 0xd2 as u8, 0xa6 as u8,
            0xab as u8, 0xf7 as u8, 0x15 as u8, 0x88 as u8,
            0x09 as u8, 0xcf as u8, 0x4f as u8, 0x3c as u8,
        ];
        const iv: [16]u8 = [
            0xf0 as u8, 0xf1 as u8, 0xf2 as u8, 0xf3 as u8,
            0xf4 as u8, 0xf5 as u8, 0xf6 as u8, 0xf7 as u8,
            0xf8 as u8, 0xf9 as u8, 0xfa as u8, 0xfb as u8,
            0xfc as u8, 0xfd as u8, 0xfe as u8, 0xff as u8,
        ];
        let full: [48]u8 = [0xAA as u8; 48];
        let c_full: Aes128Ctr128Be = Aes128Ctr128Be.new(key, iv);
        c_full.applyKeystream(full[0..48]);

        // Seek directly to byte 21 (block 1, offset 5) and decrypt 20 bytes
        let slice_dec: [20]u8 = [0 as u8; 20];
        let k: usize = 0;
        while (k < 20) {
            slice_dec[k] = full[21 + k];
            k += 1;
        }
        let c_seek: Aes128Ctr128Be = Aes128Ctr128Be.new(key, iv);
        c_seek.seek(21);
        expect_eq(c_seek.currentPos(), 21 as u64);
        c_seek.applyKeystream(slice_dec[0..20]);
        expect_eq(c_seek.currentPos(), 41 as u64);

        k = 0;
        while (k < 20) {
            expect_eq(slice_dec[k], 0xAA as u8);
            k += 1;
        }
    });

    test("5. @cap/ctr — Aes128Ctr128Le & Aes256Ctr128Le encrypt/decrypt roundtrip + seek", () => {
        const key128: [16]u8 = [0x11 as u8; 16];
        const key256: [32]u8 = [0x22 as u8; 32];
        const iv: [16]u8 = [
            0xFF as u8, 0xFF as u8, 0x00 as u8, 0x00 as u8,
            0x01 as u8, 0x02 as u8, 0x03 as u8, 0x04 as u8,
            0x05 as u8, 0x06 as u8, 0x07 as u8, 0x08 as u8,
            0x09 as u8, 0x0A as u8, 0x0B as u8, 0x0C as u8,
        ];

        let msg1: [35]u8 = [0x55 as u8; 35];
        let c128: Aes128Ctr128Le = Aes128Ctr128Le.new(key128, iv);
        c128.applyKeystream(msg1[0..35]);
        // Must differ from plaintext
        expect_eq(msg1[0] != (0x55 as u8) || msg1[1] != (0x55 as u8), true);
        c128.seek(0);
        c128.applyKeystream(msg1[0..35]);
        expect_eq(msg1[0], 0x55 as u8);
        expect_eq(msg1[34], 0x55 as u8);

        let msg2: [35]u8 = [0x77 as u8; 35];
        let c256: Aes256Ctr128Le = Aes256Ctr128Le.new(key256, iv);
        c256.applyKeystream(msg2[0..35]);
        c256.seek(0);
        c256.applyKeystream(msg2[0..35]);
        expect_eq(msg2[0], 0x77 as u8);
        expect_eq(msg2[34], 0x77 as u8);
    });

    test("6. @cap/ctr — BE vs LE produce identical block 0 and distinct block 1", () => {
        const key: [16]u8 = [0x33 as u8; 16];
        const iv: [16]u8 = [0x00 as u8; 16];
        let be_out: [32]u8 = [0 as u8; 32];
        let le_out: [32]u8 = [0 as u8; 32];

        let c_be: Aes128Ctr128Be = Aes128Ctr128Be.new(key, iv);
        let c_le: Aes128Ctr128Le = Aes128Ctr128Le.new(key, iv);
        c_be.applyKeystream(be_out[0..32]);
        c_le.applyKeystream(le_out[0..32]);

        // Block 0 uses identical IV
        let i: usize = 0;
        while (i < 16) {
            expect_eq(be_out[i], le_out[i]);
            i += 1;
        }
        // Block 1 differs (iv[15]=1 vs iv[0]=1)
        let diffs: usize = 0;
        while (i < 32) {
            if (be_out[i] != le_out[i]) diffs += 1;
            i += 1;
        }
        expect_eq(diffs > 0, true);
    });
}
