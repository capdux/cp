// cap/test/testCapByteorder.cp
// Comprehensive unit tests for @cap/byteorder

import { test, expect_eq } from "cap:test";
import {
    readU16Be,
    readU16Le,
    readU32Be,
    readU32Le,
    readU64Be,
    readU64Le,
    writeU16Be,
    writeU32Be,
    writeU64Le,
    ByteReader,
    ByteWriter,
} from "../../packages/untrusted/src/index.cp";

fn main() {
    test("1. @cap/byteorder — stateless u16/u32/u64 BE & LE round-trip", () => {
        let buf: [32]u8 = undefined;
        expect_eq(writeU16Be(buf[0..32], 0, 0x1234), true);
        expect_eq(writeU32Be(buf[0..32], 2, 0xDEADBEEF), true);
        expect_eq(writeU64Le(buf[0..32], 6, 0x0102030405060708), true);

        const s: string = buf[0..14];
        expect_eq(readU16Be(s, 0) ?? 0, 0x1234 as u16);
        expect_eq(readU16Le(s, 0) ?? 0, 0x3412 as u16);
        expect_eq(readU32Be(s, 2) ?? 0, 0xDEADBEEF as u32);
        expect_eq(readU32Le(s, 2) ?? 0, 0xEFBEADDE as u32);
        expect_eq(readU64Le(s, 6) ?? 0, 0x0102030405060708 as u64);
        expect_eq(readU64Be(s, 6) ?? 0, 0x0807060504030201 as u64);
    });

    test("2. @cap/byteorder — ByteWriter & ByteReader protocol frame", () => {
        let buf: [64]u8 = undefined;
        let w = ByteWriter.init(buf[0..64]);
        expect_eq(w.writeU8(0x01), true);
        expect_eq(w.writeU16Be(0xCAFE), true);
        expect_eq(w.writeU32Le(0x11223344), true);
        const big64: u64 = ((0xAABBCCDD as u64) << 32) | (0xEEFF0011 as u64);
        expect_eq(w.writeU64Be(big64), true);
        expect_eq(w.writeBytes("capdux"), true);
        expect_eq(w.overflow, false);

        let r = ByteReader.init(w.asSlice());
        expect_eq(r.remaining(), 21 as usize);
        expect_eq(r.peekU8() ?? 0, 0x01 as u8);
        expect_eq(r.readU8() ?? 0, 0x01 as u8);
        expect_eq(r.readU16Be() ?? 0, 0xCAFE as u16);
        expect_eq(r.readU32Le() ?? 0, 0x11223344 as u32);
        expect_eq(r.readU64Be() ?? 0, big64);
        expect_eq(r.readBytes(6) ?? "", "capdux");
        expect_eq(r.atEnd(), true);
    });

    test("3. @cap/byteorder — bounds safety on truncated untrusted input", () => {
        let r = ByteReader.init("AB");
        expect_eq(r.readU32Be() == null, true);
        expect_eq(r.pos, 0 as usize); // Cursor must not advance on failed read!
        expect_eq(r.readBytes(5) == null, true);
        expect_eq(r.skip(3), false);
        expect_eq(r.readU16Be() ?? 0, 0x4142 as u16);
        expect_eq(r.readU8() == null, true);
    });

    test("4. @cap/byteorder — skip and readToEnd", () => {
        let r = ByteReader.init("HDR:payload-bytes");
        expect_eq(r.skip(4), true);
        expect_eq(r.readToEnd(), "payload-bytes");
        expect_eq(r.atEnd(), true);
    });

    test("5. @cap/byteorder — ByteWriter overflow protection", () => {
        let small: [4]u8 = undefined;
        let w = ByteWriter.init(small[0..4]);
        expect_eq(w.writeU16Le(0x0102), true);
        expect_eq(w.writeU32Be(0x11223344), false);
        expect_eq(w.overflow, true);
        expect_eq(w.pos, 2 as usize);
    });

    test("6. @cap/byteorder — stateless out-of-bounds returns null/false", () => {
        let tiny: [2]u8 = undefined;
        expect_eq(writeU32Be(tiny[0..2], 0, 1), false);
        expect_eq(readU32Be("ab", 0) == null, true);
        expect_eq(readU64Be("1234567", 0) == null, true);
    });
}
