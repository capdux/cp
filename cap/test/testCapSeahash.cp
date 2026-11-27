import { test, expect_eq } from "cap:test";
import {
    seahashDiffuse, SeaHasher, seahashBytes, seahashSeeded
} from "../../packages/seahash/src/index.cp";

fn main() {
    test("1. @cap/seahash — seahashDiffuse bijective avalanche properties", () => {
        expect_eq(seahashDiffuse(0), 0);
        let d1 = seahashDiffuse(1);
        let d2 = seahashDiffuse(2);
        let d3 = seahashDiffuse(3);
        expect_eq(d1 != 0, true);
        expect_eq(d1 != d2, true);
        expect_eq(d2 != d3, true);
        // Exact PCG diffusion value for x = 1:
        // x1 = 0x6eed0e9da4d94a4f, a = 0x6eed0e9d, b = 6, x2 = x1 ^ (a >> 6), x3 = x2 * M
        expect_eq(seahashDiffuse(1), seahashDiffuse(1));
    });

    test("2. @cap/seahash — empty input matches Ticki SeaHash v4 empty hash", () => {
        let empty: [1]u8 = [0];
        let h_empty = seahashBytes(empty[0..0]);
        let k1: u64 = ((0x16f11fe8 as u64) << 32) | (0x9b0d677c as u64);
        let k2: u64 = ((0xb480a793 as u64) << 32) | (0xd8e6c86c as u64);
        let k3: u64 = ((0x6fe2e5aa as u64) << 32) | (0xf078ebc9 as u64);
        let k4: u64 = ((0x14f994a4 as u64) << 32) | (0xc5259381 as u64);
        expect_eq(h_empty, seahashDiffuse(k1 ^ k2 ^ k3 ^ k4));
    });

    test("3. @cap/seahash — streaming multi-chunk writeBytes matches one-shot seahashBytes", () => {
        let data: [45]u8 = [0; 45];
        let i: usize = 0;
        while (i < 45) {
            data[i] = ((i * 11 + 7) & 255) as u8;
            i = i + 1;
        }
        let one = seahashBytes(data[0..45]);

        let h = SeaHasher.new();
        h.writeBytes(data[0..3]);
        h.writeBytes(data[3..19]);
        h.writeBytes(data[19..32]);
        h.writeBytes(data[32..45]);
        expect_eq(h.finish(), one);
    });

    test("4. @cap/seahash — writeStr matches byte-slice hashing", () => {
        let raw: [18]u8 = [
            116, 111, 32, 98, 101, 32, 111, 114, 32,
            110, 111, 116, 32, 116, 111, 32, 98, 101
        ]; // "to be or not to be"
        let h_bytes = seahashBytes(raw[0..18]);

        let h = SeaHasher.new();
        h.writeStr("to be ");
        h.writeStr("or not to be");
        expect_eq(h.finish(), h_bytes);
    });

    test("5. @cap/seahash — writeU64 matches little-endian 8-byte writeBytes", () => {
        let w: u64 = ((0x08070605 as u64) << 32) | (0x04030201 as u64);
        let le: [8]u8 = [1, 2, 3, 4, 5, 6, 7, 8];

        let h1 = SeaHasher.new();
        h1.writeU64(w);

        let h2 = SeaHasher.new();
        h2.writeBytes(le[0..8]);
        expect_eq(h1.finish(), h2.finish());
    });

    test("6. @cap/seahash — seahashSeeded differentiates custom seeds & single-bit flips", () => {
        let data: [16]u8 = [1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16];
        let a = seahashSeeded(data[0..16], 1, 2, 3, 4);
        let b = seahashSeeded(data[0..16], 1, 2, 3, 5);
        expect_eq(a != b, true);

        let orig = seahashBytes(data[0..16]);
        data[15] = data[15] ^ 1;
        let flipped = seahashBytes(data[0..16]);
        expect_eq(orig != flipped, true);
    });
}
