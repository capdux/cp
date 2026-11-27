import { test, expect_eq } from "cap:test";
import {
    bytesEq,
    ctEqual,
    startsWithBytes,
    endsWithBytes,
    findByteInBytes,
    countByte,
    indexOfBytes,
    readU16BE,
    readU16LE,
    readU32BE,
    readU32LE,
    readU64BE,
    readU64LE,
    writeU16BE,
    writeU16LE,
    writeU32BE,
    writeU32LE,
    writeU64BE,
    writeU64LE,
} from "cap:bin";

fn main() {
    test("1. cap:bin — bytesEq, ctEqual, startsWithBytes, endsWithBytes", () => {
        expect_eq(bytesEq("0123456789abcdef", "0123456789abcdef"), true);
        expect_eq(bytesEq("0123456789abcdef", "0123456789abcde0"), false);
        expect_eq(ctEqual("secret-token", "secret-token"), true);
        expect_eq(ctEqual("secret-token", "secret-tokeX"), false);
        expect_eq(startsWithBytes("https://capdux.org", "https://"), true);
        expect_eq(endsWithBytes("index.d.cp", ".d.cp"), true);
    });

    test("2. cap:bin — findByte and countByte across 16-byte unrolled chunks", () => {
        const long_s = "0123456789abcdef0123456789abcdefXend";
        expect_eq(findByteInBytes(long_s, 88, 0) ?? 0, 32 as usize); // 'X'
        expect_eq(findByteInBytes(long_s, 90, 0) == null, true);      // 'Z'
        expect_eq(countByte("a,b,c,d,e,f,g,h,i", 44), 8 as usize);
    });

    test("3. cap:bin — indexOfBytes finds subslice and handles empty/missing", () => {
        expect_eq(indexOfBytes("HTTP/1.1 200 OK\r\n\r\nbody", "\r\n\r\n", 0) ?? 0, 15 as usize);
        expect_eq(indexOfBytes("abcdef", "xyz", 0) == null, true);
        expect_eq(indexOfBytes("abcdef", "", 2) ?? 0, 2 as usize);
    });

    test("4. cap:bin — U16 and U32 Big-Endian / Little-Endian round-trip", () => {
        let buf: [16]u8 = undefined;
        expect_eq(writeU16BE(buf[0..16], 0, 0x1234), true);
        expect_eq(readU16BE(buf[0..16], 0) ?? 0, 0x1234 as u16);
        expect_eq(readU16LE(buf[0..16], 0) ?? 0, 0x3412 as u16);

        expect_eq(writeU32LE(buf[0..16], 4, 0xDEADBEEF), true);
        expect_eq(readU32LE(buf[0..16], 4) ?? 0, 0xDEADBEEF as u32);
        expect_eq(readU32BE(buf[0..16], 4) ?? 0, 0xEFBEADDE as u32);
    });

    test("5. cap:bin — U64 Big-Endian / Little-Endian round-trip", () => {
        let buf: [16]u8 = undefined;
        const val: u64 = ((0x01020304 as u64) << 32) | (0x05060708 as u64);
        expect_eq(writeU64BE(buf[0..16], 0, val), true);
        expect_eq(readU64BE(buf[0..16], 0) ?? 0, val);
        expect_eq(writeU64LE(buf[0..16], 8, val), true);
        expect_eq(readU64LE(buf[0..16], 8) ?? 0, val);
    });

    test("6. cap:bin — out-of-bounds reads and writes fail safely", () => {
        let buf: [3]u8 = undefined;
        expect_eq(writeU32BE(buf[0..3], 0, 1), false);
        expect_eq(readU32BE(buf[0..3], 0) == null, true);
        expect_eq(readU64LE(buf[0..3], 0) == null, true);
    });
}
