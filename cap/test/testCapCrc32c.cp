import { test, expect_eq } from "cap:test";
import {
    CRC32C_POLY_REVERSED,
    crc32c,
    crc32cAppend,
    crc32cStr,
    crc32cCombine,
    maskCrc32c,
    unmaskCrc32c,
    Crc32cHasher
} from "../../packages/crc32c/src/index.cp";

fn main() {
    test("1. @cap/crc32c — RFC 3720 standard check vector 123456789 and empty input", () => {
        expect_eq(CRC32C_POLY_REVERSED, 2197175160);
        expect_eq(crc32cStr(""), 0);
        expect_eq(crc32cStr("123456789"), 3808858755); // 0xE3069283

        let empty: [1]u8 = [0];
        expect_eq(crc32c(empty[0..0]), 0);

        let digits: [9]u8 = [49, 50, 51, 52, 53, 54, 55, 56, 57];
        expect_eq(crc32c(digits[0..9]), 3808858755);
    });

    test("2. @cap/crc32c — RFC 3720 32-byte known test vectors (zeros, 0xFF, ascending, descending)", () => {
        // 32 bytes of 0x00 -> 0x8A9136AA (2324772522)
        let zeros: [32]u8 = [0; 32];
        expect_eq(crc32c(zeros[0..32]), 2324772522);

        // 32 bytes of 0xFF -> 0x62A8AB43 (1655221059)
        let ones: [32]u8 = [255; 32];
        expect_eq(crc32c(ones[0..32]), 1655221059);

        // 32 bytes 0x00..0x1F -> 0x46DD794E (1188919630)
        let asc: [32]u8 = [0; 32];
        let i: usize = 0;
        while (i < 32) {
            asc[i] = i as u8;
            i = i + 1;
        }
        expect_eq(crc32c(asc[0..32]), 1188919630);

        // 32 bytes 0x1F..0x00 -> 0x113FDB5C (289397596)
        let desc: [32]u8 = [0; 32];
        i = 0;
        while (i < 32) {
            desc[i] = (31 - i) as u8;
            i = i + 1;
        }
        expect_eq(crc32c(desc[0..32]), 289397596);
    });

    test("3. @cap/crc32c — crc32cAppend streaming matches one-shot crc32c", () => {
        let digits: [9]u8 = [49, 50, 51, 52, 53, 54, 55, 56, 57];
        let c: u32 = 0;
        c = crc32cAppend(c, digits[0..3]);
        c = crc32cAppend(c, digits[3..6]);
        c = crc32cAppend(c, digits[6..9]);
        expect_eq(c, 3808858755);

        // Append empty slice leaves CRC unchanged
        expect_eq(crc32cAppend(c, digits[0..0]), 3808858755);
    });

    test("4. @cap/crc32c — crc32cCombine GF(2) merges independent chunks accurately", () => {
        let buf: [64]u8 = [0; 64];
        let i: usize = 0;
        while (i < 64) {
            buf[i] = ((i * 37 + 13) & 255) as u8;
            i = i + 1;
        }
        let full = crc32c(buf[0..64]);
        let part1 = crc32c(buf[0..19]);
        let part2 = crc32c(buf[19..64]);
        expect_eq(crc32cCombine(part1, part2, 45), full);

        // len2 == 0 returns crc1
        expect_eq(crc32cCombine(part1, 0, 0), part1);
    });

    test("5. @cap/crc32c — maskCrc32c and unmaskCrc32c round-trip including wraparound edges", () => {
        let v0: u32 = 0;
        let v1: u32 = 3808858755; // 0xE3069283
        let v2: u32 = 4294967295; // 0xFFFFFFFF
        let v3: u32 = 305419896;  // 0x12345678
        let v4: u32 = 2726488792; // 0xA282EAD8

        expect_eq(unmaskCrc32c(maskCrc32c(v0)), v0);
        expect_eq(unmaskCrc32c(maskCrc32c(v1)), v1);
        expect_eq(unmaskCrc32c(maskCrc32c(v2)), v2);
        expect_eq(unmaskCrc32c(maskCrc32c(v3)), v3);
        expect_eq(unmaskCrc32c(maskCrc32c(v4)), v4);

        // mask(0) == 0xA282EAD8
        expect_eq(maskCrc32c(0), 2726488792);
        // mask(x) != x
        expect_eq(maskCrc32c(v1) != v1, true);
    });

    test("6. @cap/crc32c — Crc32cHasher streaming, reset, and combine", () => {
        let h1 = Crc32cHasher.new();
        h1.updateStr("1234");
        let h2 = Crc32cHasher.new();
        h2.updateStr("56789");
        h1.combine(h2);
        expect_eq(h1.finalize(), 3808858755);
        expect_eq(h1.amount, 9);

        h1.reset();
        expect_eq(h1.finalize(), 0);
        expect_eq(h1.amount, 0);
    });
}
