import { test, expect_eq } from "cap:test";
import {
    DecoderEncoding,
    StringDecoder,
} from "../../packages/@cap/string-decoder/src/index.cp";

fn main() {
    test("1. @cap/string-decoder — reassembles 3-byte UTF-8 Euro sign split across 3 chunks", () => {
        let sd = StringDecoder.utf8();
        let out: [32]u8 = undefined;
        let c1: [1]u8 = [0xE2];
        let c2: [1]u8 = [0x82];
        let c3: [1]u8 = [0xAC];

        expect_eq(sd.write(c1[0..1], out[0..32]), 0 as usize);
        expect_eq(sd.pendingBytes(), 1 as usize);
        expect_eq(sd.write(c2[0..1], out[0..32]), 0 as usize);
        expect_eq(sd.pendingBytes(), 2 as usize);
        let n = sd.write(c3[0..1], out[0..32]);
        expect_eq(out[0..n], "€");
        expect_eq(sd.pendingBytes(), 0 as usize);
    });

    test("2. @cap/string-decoder — reassembles 4-byte emoji split across 2 chunks", () => {
        let sd = StringDecoder.utf8();
        let out: [32]u8 = undefined;
        let part1: [3]u8 = [0x41, 0xF0, 0x9F]; // 'A' + first 2 bytes of 🚀 (F0 9F 9A 80)
        let part2: [3]u8 = [0x9A, 0x80, 0x21]; // last 2 bytes + '!'

        let n1 = sd.write(part1[0..3], out[0..32]);
        expect_eq(out[0..n1], "A");
        let n2 = sd.write(part2[0..3], out[0..32]);
        expect_eq(out[0..n2], "🚀!");
    });

    test("3. @cap/string-decoder — end() flushes incomplete trailing UTF-8 as U+FFFD", () => {
        let sd = StringDecoder.utf8();
        let out: [32]u8 = undefined;
        let partial: [2]u8 = [0xE4, 0xB8]; // incomplete '中'
        expect_eq(sd.write(partial[0..2], out[0..32]), 0 as usize);
        let n = sd.end(out[0..32]);
        expect_eq(n, 3 as usize);
        expect_eq(out[0], 0xEF as u8);
        expect_eq(out[1], 0xBF as u8);
        expect_eq(out[2], 0xBD as u8);
    });

    test("4. @cap/string-decoder — broken continuation emits replacement and keeps valid ASCII", () => {
        let sd = StringDecoder.utf8();
        let out: [32]u8 = undefined;
        let c1: [1]u8 = [0xC3];
        let c2: [2]u8 = [0x41, 0x42]; // 'A','B' instead of 0x80..0xBF
        expect_eq(sd.write(c1[0..1], out[0..32]), 0 as usize);
        let n = sd.write(c2[0..2], out[0..32]);
        expect_eq(n, 5 as usize); // U+FFFD (3B) + "AB" (2B)
        expect_eq(out[3], 65 as u8);
        expect_eq(out[4], 66 as u8);
    });

    test("5. @cap/string-decoder — Latin1 converts bytes 0x80..0xFF to UTF-8", () => {
        let sd = StringDecoder.new(.Latin1);
        let out: [32]u8 = undefined;
        let data: [2]u8 = [0x41, 0xE9]; // 'A', 'é'
        let n = sd.write(data[0..2], out[0..32]);
        expect_eq(out[0..n], "Aé");
    });

    test("6. @cap/string-decoder — UTF-16LE reassembles odd byte and surrogate pair across chunks", () => {
        let sd = StringDecoder.new(.Utf16Le);
        let out: [32]u8 = undefined;
        // '😀' U+1F600 = D83D DE00 in UTF-16LE: 3D D8 00 DE
        let c1: [3]u8 = [0x3D, 0xD8, 0x00];
        let c2: [1]u8 = [0xDE];
        expect_eq(sd.write(c1[0..3], out[0..32]), 0 as usize);
        let n = sd.write(c2[0..1], out[0..32]);
        expect_eq(out[0..n], "😀");
        expect_eq(sd.end(out[0..32]), 0 as usize);
    });
}
