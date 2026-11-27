import { test, expect_eq } from "cap:test";
import {
    ADLER32_NMAX,
    Adler32,
    adler32,
    adler32Slice,
    adler32Str,
    combineAdler32,
    adler32Combine,
} from "../../packages/adler2/src/index.cp";

fn main() {
    test("1. adler2 — official RFC 1950 Wikipedia and standard vectors", () => {
        expect_eq(adler32(""), 1 as u32);
        expect_eq(adler32("Wikipedia"), 0x11E60398 as u32);
        expect_eq(adler32("123456789"), 0x091E01DE as u32);
        expect_eq(adler32("a"), 0x00620062 as u32);
        expect_eq(adler32Slice("Wikipedia"), 0x11E60398 as u32);
        expect_eq(adler32Str("123456789"), 0x091E01DE as u32);
    });

    test("2. adler2 — streaming chunked updates match one-shot (init/update/finalize)", () => {
        let h = Adler32.init();
        h.update("Wiki");
        h.updateByte('p');
        h.update("edia");
        expect_eq(h.finalize(), 0x11E60398 as u32);
        h.reset();
        expect_eq(h.finalize(), 1 as u32);
    });

    test("3. adler2 — Rust adler2 crate API (new/fromChecksum/writeByte/writeSlice/checksum)", () => {
        let h = Adler32.new();
        expect_eq(h.checksum(), 1 as u32);
        h.writeSlice("Wiki");
        h.writeByte('p');
        h.writeStr("edia");
        expect_eq(h.checksum(), 0x11E60398 as u32);

        const prefix = adler32Str("12345");
        let resumed = Adler32.fromChecksum(prefix);
        resumed.writeSlice("6789");
        expect_eq(resumed.checksum(), 0x091E01DE as u32);
    });

    test("4. adler2 — large buffer > NMAX (5552) deferred modulo accuracy", () => {
        expect_eq(ADLER32_NMAX, 5552 as usize);
        let buf: [6000]u8 = undefined;
        let i: usize = 0;
        while (i < 6000) {
            buf[i] = ((i * 31 + 7) & 0xFF) as u8;
            i += 1;
        }
        const one_shot = adler32(buf[0..6000]);
        let h = Adler32.new();
        let j: usize = 0;
        while (j < 6000) {
            h.writeByte(buf[j]);
            j += 1;
        }
        expect_eq(one_shot, h.checksum());
    });

    test("5. adler2 — rolling window matches direct adler32 across entire string", () => {
        const text: []const u8 = "The quick brown fox jumps over the lazy dog!";
        const win: usize = 8;
        let h = Adler32.init();
        h.update(text[0..win]);
        expect_eq(h.finalize(), adler32(text[0..win]));
        let i: usize = 0;
        while (i + win < text.len) {
            h.roll(win, text[i], text[i + win]);
            expect_eq(h.finalize(), adler32(text[i + 1 .. i + 1 + win]));
            i += 1;
        }
    });

    test("6. adler2 — combineAdler32 & adler32Combine merge shards accurately", () => {
        const part1: []const u8 = "Hello, CP ";
        const part2: []const u8 = "Language Ecosystem!";
        const full: []const u8 = "Hello, CP Language Ecosystem!";
        const a1 = adler32(part1);
        const a2 = adler32(part2);
        expect_eq(combineAdler32(a1, a2, part2.len as u64), adler32(full));
        expect_eq(adler32Combine(a1, a2, part2.len as u64), adler32(full));
        // Combining with empty second shard preserves first checksum
        expect_eq(adler32Combine(a1, 1 as u32, 0 as u64), a1);
    });

    test("7. adler2 — fromValue resumes checksum state cleanly", () => {
        const prefix = adler32("12345");
        let resumed = Adler32.fromValue(prefix);
        resumed.update("6789");
        expect_eq(resumed.finalize(), 0x091E01DE as u32);
    });
}
