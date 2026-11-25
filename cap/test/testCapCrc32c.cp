// cap/test/testCapCrc32c.cp
// RFC 3720 / iSCSI official test vectors, streaming hasher, GF(2) combine, and LevelDB/TFRecord mask/unmask.

import { test, expect_eq } from "cap:test";
import {
    crc32c,
    crc32cAppend,
    crc32cCombine,
    mask,
    unmask,
    Crc32cHasher,
} from "../../packages/@cap/crc32c/src/index.cp";

fn main() {
    test("1. @cap/crc32c — RFC 3720 Official Test Vectors", () => {
        expect_eq(crc32c(""), 0x00000000 as u32);
        expect_eq(crc32c("123456789"), 0xE3069283 as u32);

        // 32 bytes of zeros -> 0x8A9136AA
        const zeros: [32]u8 = [0; 32];
        expect_eq(crc32c(zeros[0..]), 0x8A9136AA as u32);

        // 32 bytes of 0xFF -> 0x62A8AB43
        const ones: [32]u8 = [0xFF; 32];
        expect_eq(crc32c(ones[0..]), 0x62A8AB43 as u32);

        // 32 bytes ascending 0x00..0x1F -> 0x46DD794E
        let asc: [32]u8 = [0; 32];
        let i: usize = 0;
        while (i < 32) : (i += 1) {
            asc[i] = i as u8;
        }
        expect_eq(crc32c(asc[0..]), 0x46DD794E as u32);

        // 32 bytes descending 0x1F..0x00 -> 0x113FDB5C
        let desc: [32]u8 = [0; 32];
        let j: usize = 0;
        while (j < 32) : (j += 1) {
            desc[j] = (31 - j) as u8;
        }
        expect_eq(crc32c(desc[0..]), 0x113FDB5C as u32);
    });

    test("2. @cap/crc32c — Streaming Hasher, Append & Reset", () => {
        let h = Crc32cHasher.init();
        h.update("123");
        const mid = h.finalize();
        expect_eq(mid, crc32c("123"));
        h.update("456789");
        expect_eq(h.finalize(), 0xE3069283 as u32);
        expect_eq(h.bytesHashed(), 9 as u64);

        const app = crc32cAppend(crc32c("1234"), "56789");
        expect_eq(app, 0xE3069283 as u32);

        h.reset();
        expect_eq(h.bytesHashed(), 0 as u64);
        expect_eq(h.finalize(), 0 as u32);
    });

    test("3. @cap/crc32c — GF(2) Parallel Combine & LevelDB/TFRecord Mask/Unmask", () => {
        const full = "The quick brown fox jumps over the lazy dog";
        const c_full = crc32c(full);
        const c1 = crc32c(full[0..19]);
        const c2 = crc32c(full[19..]);
        expect_eq(crc32cCombine(c1, c2, (full.len - 19) as u64), c_full);
        expect_eq(crc32cCombine(c_full, 0, 0), c_full);

        const m = mask(c_full);
        expect_eq(m != c_full, true);
        expect_eq(unmask(m), c_full);

        let h = Crc32cHasher.init();
        h.update(full);
        expect_eq(h.finalizeMasked(), m);
    });
}
