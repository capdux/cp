import { test, expect_eq } from "cap:test";
import { SmartBuffer } from "../../packages/@cap/smart-buffer/src/index.cp";

fn main() {
    test("1. @cap/smart-buffer — write and read u8/u16BE/u16LE", () => {
        let sb = SmartBuffer.new();
        sb.writeU8(0x7F);
        sb.writeU16BE(0x1234);
        sb.writeU16LE(0xABCD);
        expect_eq(sb.len(), 5);
        expect_eq(sb.remaining(), 5);
        expect_eq(sb.readU8(), 0x7F);
        expect_eq(sb.readU16BE(), 0x1234);
        expect_eq(sb.readU16LE(), 0xABCD);
        expect_eq(sb.remaining(), 0);
    });

    test("2. @cap/smart-buffer — write and read u32BE/u32LE round-trip", () => {
        let sb = SmartBuffer.new();
        sb.writeU32BE(0xDEADBEEF);
        sb.writeU32LE(0x01020304);
        expect_eq(sb.len(), 8);
        expect_eq(sb.readU32BE(), 0xDEADBEEF);
        expect_eq(sb.readU32LE(), 0x01020304);
    });

    test("3. @cap/smart-buffer — writeString, writeStringNT, and readString", () => {
        let sb = SmartBuffer.new();
        sb.writeString("hello");
        sb.writeStringNT("cp");
        expect_eq(sb.len(), 8);
        expect_eq(sb.readString(5), "hello");
        expect_eq(sb.readString(2), "cp");
        expect_eq(sb.readU8(), 0);
        expect_eq(sb.remaining(), 0);
    });

    test("4. @cap/smart-buffer — fromSlice initializes buffer with existing bytes", () => {
        const raw: [6]u8 = [0x00, 0x2A, 0x43, 0x50, 0x21, 0x00];
        let sb = SmartBuffer.fromSlice(raw[0..6]);
        expect_eq(sb.len(), 6);
        expect_eq(sb.readU16BE(), 42);
        expect_eq(sb.readString(3), "CP!");
        expect_eq(sb.readU8(), 0);
    });

    test("5. @cap/smart-buffer — rewind resets read cursor without altering length", () => {
        let sb = SmartBuffer.new();
        sb.writeU16BE(0xCAFE);
        expect_eq(sb.readU16BE(), 0xCAFE);
        expect_eq(sb.remaining(), 0);
        sb.rewind();
        expect_eq(sb.remaining(), 2);
        expect_eq(sb.readU8(), 0xCA);
        expect_eq(sb.readU8(), 0xFE);
    });

    test("6. @cap/smart-buffer — underflow read returns 0 and clamps readString", () => {
        let sb = SmartBuffer.new();
        sb.writeString("ab");
        expect_eq(sb.readString(10), "ab");
        expect_eq(sb.readU8(), 0);
        expect_eq(sb.readU16BE(), 0);
        expect_eq(sb.readU32LE(), 0);
        expect_eq(sb.remaining(), 0);
    });
}
