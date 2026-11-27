import { test, expect_eq } from "cap:test";
import { Adler32, adler32, combineAdler32 } from "../../packages/adler2/src/index.cp";

fn main() {
    test("official RFC 1950 Wikipedia and standard vectors", () => {
        expect_eq(adler32(""), 1);
        expect_eq(adler32("Wikipedia"), 0x11E60398);
        expect_eq(adler32("123456789"), 0x091E01DE);
        expect_eq(adler32("a"), 0x00620062);
    });

    test("streaming chunked updates match one-shot", () => {
        let h = Adler32.init();
        h.update("Wiki");
        h.updateByte('p');
        h.update("edia");
        expect_eq(h.finalize(), 0x11E60398);
        h.reset();
        expect_eq(h.finalize(), 1);
    });

    test("large buffer > NMAX (5552) deferred modulo accuracy", () => {
        let buf: [6000]u8 = undefined;
        let i: usize = 0;
        while (i < 6000) {
            buf[i] = ((i * 31 + 7) & 0xFF) as u8;
            i += 1;
        }
        let one_shot = adler32(buf[0..6000]);
        let h = Adler32.init();
        let j: usize = 0;
        while (j < 6000) {
            h.updateByte(buf[j]);
            j += 1;
        }
        expect_eq(one_shot, h.finalize());
    });

    test("rolling window matches direct adler32 across entire string", () => {
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

    test("combineAdler32 merges two shards accurately", () => {
        const part1: []const u8 = "Hello, CP ";
        const part2: []const u8 = "Language Ecosystem!";
        const full: []const u8 = "Hello, CP Language Ecosystem!";
        let a1 = adler32(part1);
        let a2 = adler32(part2);
        expect_eq(combineAdler32(a1, a2, part2.len as u64), adler32(full));
    });

    test("fromValue resumes checksum state cleanly", () => {
        let prefix = adler32("12345");
        let resumed = Adler32.fromValue(prefix);
        resumed.update("6789");
        expect_eq(resumed.finalize(), 0x091E01DE);
    });
}
