// cap/test/testCapCrc32.cp
// Comprehensive unit tests for @cap/crc32

import { test, expect_eq } from "cap:test";
import {
    Crc32Hasher,
    Crc32cHasher,
    crc32Ieee,
    crc32c,
    adler32,
    combineCrc32Ieee,
} from "../../packages/@cap/crc32/src/index.cp";

fn main() {
    test("1. @cap/crc32 — IEEE 802.3 CRC-32 official test vectors", () => {
        expect_eq(crc32Ieee(""), 0x00000000 as u32);
        expect_eq(crc32Ieee("123456789"), 0xCBF43926 as u32);
        expect_eq(crc32Ieee("The quick brown fox jumps over the lazy dog"), 0x414FA339 as u32);
    });

    test("2. @cap/crc32 — Castagnoli CRC-32C official test vectors", () => {
        expect_eq(crc32c(""), 0x00000000 as u32);
        expect_eq(crc32c("123456789"), 0xE3069283 as u32);
        expect_eq(crc32c("The quick brown fox jumps over the lazy dog"), 0x22620404 as u32);
    });

    test("3. @cap/crc32 — RFC 1950 Adler-32 official test vectors", () => {
        expect_eq(adler32(""), 0x00000001 as u32);
        expect_eq(adler32("Wikipedia"), 0x11E60398 as u32);
        expect_eq(adler32("123456789"), 0x091E01DE as u32);
    });

    test("4. @cap/crc32 — Streaming Crc32Hasher chunked updates & reset", () => {
        let h = Crc32Hasher.init();
        h.update("123");
        h.update("456");
        h.update("789");
        expect_eq(h.finalize(), 0xCBF43926 as u32);

        h.reset();
        expect_eq(h.finalize(), 0x00000000 as u32);
        h.update("The quick brown fox ");
        h.update("jumps over the lazy dog");
        expect_eq(h.finalize(), 0x414FA339 as u32);
    });

    test("5. @cap/crc32 — Streaming Crc32cHasher chunked updates & reset", () => {
        let h = Crc32cHasher.init();
        h.update("12");
        h.update("34567");
        h.update("89");
        expect_eq(h.finalize(), 0xE3069283 as u32);

        h.reset();
        h.update("The quick brown fox jumps over ");
        h.update("the lazy dog");
        expect_eq(h.finalize(), 0x22620404 as u32);
    });

    test("6. @cap/crc32 — combineCrc32Ieee O(log N) parallel chunk merging", () => {
        const partA = crc32Ieee("1234");
        const partB = crc32Ieee("56789");
        expect_eq(combineCrc32Ieee(partA, partB, 5 as usize), 0xCBF43926 as u32);
        expect_eq(combineCrc32Ieee(partA, 0 as u32, 0 as usize), partA);

        const p1 = crc32Ieee("The quick brown ");
        const p2 = crc32Ieee("fox jumps over ");
        const p3 = crc32Ieee("the lazy dog");
        const m12 = combineCrc32Ieee(p1, p2, 15 as usize);
        const m123 = combineCrc32Ieee(m12, p3, 12 as usize);
        expect_eq(m123, 0x414FA339 as u32);
    });
}
