import { test, expect_eq } from "cap:test";
import { newBufferList } from "../../packages/@cap/bl/src/index.cp";

fn main() {
    test("1. @cap/bl — appends multiple chunks and indexes seamlessly across boundaries", () => {
        let bl = newBufferList();
        expect_eq(bl.isEmpty(), true);
        expect_eq(bl.append("hello "), true);
        expect_eq(bl.append("world"), true);
        expect_eq(bl.len(), 11 as usize);
        expect_eq(bl.get(0) ?? 0, 104 as u8);  // 'h'
        expect_eq(bl.get(6) ?? 0, 119 as u8);  // 'w'
        expect_eq(bl.get(11) == null, true);
    });

    test("2. @cap/bl — copyRange extracts slice spanning multiple chunks", () => {
        let bl = newBufferList();
        bl.append("foo");
        bl.append("bar");
        bl.append("baz");
        let out: [16]u8 = [0; 16];
        const n = bl.copyRange(2, 7, out[0..16]);
        expect_eq(n, 5 as usize);
        expect_eq(out[0..n] as string, "obarb");
    });

    test("3. @cap/bl — consume drops full and partial leading chunks", () => {
        let bl = newBufferList();
        bl.append("abcd");
        bl.append("efgh");
        bl.append("ijkl");
        bl.consume(6); // drops "abcd" + "ef"
        expect_eq(bl.len(), 6 as usize);
        let out: [16]u8 = [0; 16];
        const n = bl.copyRange(0, 6, out[0..16]);
        expect_eq(out[0..n] as string, "ghijkl");
        bl.consume(100);
        expect_eq(bl.isEmpty(), true);
    });

    test("4. @cap/bl — reads U16/U32 big-endian and little-endian across chunk boundary", () => {
        let bl = newBufferList();
        const c1: [2]u8 = [0x12, 0x34];
        const c2: [2]u8 = [0x56, 0x78];
        bl.append(c1[0..2]);
        bl.append(c2[0..2]);
        expect_eq(bl.readU16BE(1) ?? 0, 0x3456 as u16);
        expect_eq(bl.readU16LE(1) ?? 0, 0x5634 as u16);
        expect_eq(bl.readU32BE(0) ?? 0, 0x12345678 as u32);
        expect_eq(bl.readU32LE(0) ?? 0, 0x78563412 as u32);
        expect_eq(bl.readU32BE(1) == null, true);
    });

    test("5. @cap/bl — indexOf locates needle spanning chunk boundaries", () => {
        let bl = newBufferList();
        bl.append("HTTP/1.1 200 OK\r");
        bl.append("\nContent-Length: 5\r\n\r\nhello");
        expect_eq(bl.indexOf("\r\n\r\n", 0) ?? 0, 34 as usize);
        expect_eq(bl.indexOf("200", 0) ?? 0, 9 as usize);
        expect_eq(bl.indexOf("missing", 0) == null, true);
    });

    test("6. @cap/bl — ignores empty chunk appends", () => {
        let bl = newBufferList();
        expect_eq(bl.append(""), true);
        expect_eq(bl.chunk_count, 0 as usize);
        expect_eq(bl.append("ok"), true);
        expect_eq(bl.chunk_count, 1 as usize);
    });
}
