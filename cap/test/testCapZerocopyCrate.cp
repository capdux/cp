import { test, expect_eq } from "cap:test";
import {
    ByteCursor, ByteWriterCursor, isAlignedSlice,
    U16LE, U16BE, U32LE, U32BE, U64LE, U64BE,
    readFromPrefix, readFromSuffix, writeToPrefix, writeToSuffix,
    readU16LeFromPrefix, readU32BeFromPrefix, readU64LeFromPrefix,
    readU32LeFromSuffix,
    refFromBytesU16, refFromBytesU32, refFromBytesU64,
    refFromBytesSliceU32, sliceAsBytesU16, sliceAsBytesU32, sliceAsBytesU64
} from "../../packages/zerocopy/src/index.cp";

fn main() {
    test("1. @cap/zerocopy-crate — ByteWriterCursor + ByteCursor multi-field roundtrip", () => {
        let buf: [32]u8 = [0; 32];
        let w = ByteWriterCursor.new(32);
        expect_eq(w.putU8(buf[0..32], 42), true);
        expect_eq(w.putU16Le(buf[0..32], 4660), true);
        expect_eq(w.putU32Le(buf[0..32], 305419896), true);
        expect_eq(w.putU32Be(buf[0..32], 2018915346), true);
        expect_eq(w.putU64Le(buf[0..32], 1234567890123), true);
        expect_eq(w.written(), 19);

        let r = ByteCursor.new(w.written());
        expect_eq(r.nextU8(buf[0..32]) ?? 0, 42);
        expect_eq(r.nextU16Le(buf[0..32]) ?? 0, 4660);
        expect_eq(r.nextU32Le(buf[0..32]) ?? 0, 305419896);
        expect_eq(r.nextU32Be(buf[0..32]) ?? 0, 2018915346);
        expect_eq(r.nextU64Le(buf[0..32]) ?? 0, 1234567890123);
        expect_eq(r.remaining(), 0);
    });

    test("2. @cap/zerocopy-crate — ByteCursor rejects reads past end without advancing", () => {
        let buf: [4]u8 = [1, 2, 3, 4];
        let r = ByteCursor.new(4);
        expect_eq(r.nextU64Le(buf[0..4]) == null, true);
        expect_eq(r.pos, 0);
        expect_eq(r.nextU32Le(buf[0..4]) != null, true);
        expect_eq(r.nextU8(buf[0..4]) == null, true);
    });

    test("3. @cap/zerocopy-crate — ByteCursor skip advances cleanly and guards overflow", () => {
        let buf: [8]u8 = [0, 0, 0, 0, 10, 20, 0, 0];
        let r = ByteCursor.new(8);
        expect_eq(r.skip(4), true);
        expect_eq(r.nextU8(buf[0..8]) ?? 0, 10);
        expect_eq(r.nextU8(buf[0..8]) ?? 0, 20);
        expect_eq(r.skip(10), false);
    });

    test("4. @cap/zerocopy-crate — ByteWriterCursor enforces capacity boundary", () => {
        let buf: [6]u8 = [0; 6];
        let w = ByteWriterCursor.new(6);
        expect_eq(w.putU32Le(buf[0..6], 99), true);
        expect_eq(w.putU32Le(buf[0..6], 100), false);
        expect_eq(w.written(), 4);
    });

    test("5. @cap/zerocopy-crate — isAlignedSlice delegates to @cap/bytemuck-crate", () => {
        expect_eq(isAlignedSlice(24, 8, 8192, 8), true);
        expect_eq(isAlignedSlice(20, 8, 8192, 8), false);
        expect_eq(isAlignedSlice(24, 8, 8196, 8), false);
    });

    test("6. @cap/zerocopy-crate — empty buffer cursor has 0 remaining", () => {
        let buf: [1]u8 = [0];
        let r = ByteCursor.new(0);
        expect_eq(r.remaining(), 0);
        expect_eq(r.nextU8(buf[0..0]) == null, true);
    });

    test("7. @cap/zerocopy-crate — U16/U32/U64 LE & BE byteorder wrappers", () => {
        let a = U16LE.new(0x1234);
        expect_eq(a.bytes[0], 0x34 as u8);
        expect_eq(a.bytes[1], 0x12 as u8);
        expect_eq(a.get(), 0x1234 as u16);

        let b = U16BE.new(0x1234);
        expect_eq(b.bytes[0], 0x12 as u8);
        expect_eq(b.bytes[1], 0x34 as u8);
        expect_eq(b.get(), 0x1234 as u16);

        let c = U32LE.new(0xDEADBEEF);
        expect_eq(c.get(), 0xDEADBEEF as u32);
        c.set(0x01020304);
        expect_eq(c.bytes[0], 4 as u8);
        expect_eq(c.get(), 0x01020304 as u32);

        let d = U32BE.new(0x01020304);
        expect_eq(d.bytes[0], 1 as u8);
        expect_eq(d.get(), 0x01020304 as u32);

        let e = U64LE.new(0x0102030405060708);
        expect_eq(e.bytes[0], 8 as u8);
        expect_eq(e.get(), 0x0102030405060708 as u64);

        let f = U64BE.new(0x0102030405060708);
        expect_eq(f.bytes[0], 1 as u8);
        expect_eq(f.get(), 0x0102030405060708 as u64);
    });

    test("8. @cap/zerocopy-crate — readFromPrefix, readFromSuffix, writeToPrefix, writeToSuffix", () => {
        let buf: [16]u8 = [0; 16];
        const hdr: [4]u8 = [0x34, 0x12, 0x00, 0x01];
        const rest1 = writeToPrefix(buf[0..16], hdr[0..4]);
        expect_eq(rest1 != null, true);
        expect_eq(rest1.?.len, 12 as usize);

        const tail: [4]u8 = [0x78, 0x56, 0x34, 0x12];
        const lead = writeToSuffix(buf[0..16], tail[0..4]);
        expect_eq(lead != null, true);
        expect_eq(lead.?.len, 12 as usize);

        const p16 = readU16LeFromPrefix(buf[0..16]);
        expect_eq(p16 != null, true);
        expect_eq(p16.?.value, 0x1234 as u16);
        expect_eq(p16.?.rest.len, 14 as usize);

        const s32 = readU32LeFromSuffix(buf[0..16]);
        expect_eq(s32 != null, true);
        expect_eq(s32.?.value, 0x12345678 as u32);
        expect_eq(s32.?.rest.len, 12 as usize);

        const sp = readFromPrefix(buf[0..16], 4);
        expect_eq(sp != null, true);
        expect_eq(sp.?.head.len, 4 as usize);
        expect_eq(sp.?.tail.len, 12 as usize);

        const ss = readFromSuffix(buf[0..16], 4);
        expect_eq(ss != null, true);
        expect_eq(ss.?.head.len, 12 as usize);
        expect_eq(ss.?.tail.len, 4 as usize);

        expect_eq(readFromPrefix(buf[0..2], 4) == null, true);
        expect_eq(readFromSuffix(buf[0..2], 4) == null, true);
    });

    test("9. @cap/zerocopy-crate — refFromBytes alignment checks & slice conversions", () => {
        const vals16: [2]u16 = [0x1122, 0x3344];
        const b16 = sliceAsBytesU16(vals16[0..2]);
        expect_eq(b16.len, 4 as usize);
        expect_eq(refFromBytesU16(b16[0..2]) ?? 0, 0x1122 as u16);
        expect_eq(refFromBytesU16(b16[1..3]) == null, true); // misaligned by 1 byte

        const vals32: [2]u32 = [0x11223344, 0x55667788];
        const b32 = sliceAsBytesU32(vals32[0..2]);
        expect_eq(b32.len, 8 as usize);
        expect_eq(refFromBytesU32(b32[0..4]) ?? 0, 0x11223344 as u32);
        expect_eq(refFromBytesU32(b32[1..5]) == null, true); // misaligned
        const view32 = refFromBytesSliceU32(b32);
        expect_eq(view32 != null, true);
        expect_eq(view32.?.len, 2 as usize);
        expect_eq(view32.?[1], 0x55667788 as u32);

        const vals64: [1]u64 = [0x0102030405060708];
        const b64 = sliceAsBytesU64(vals64[0..1]);
        expect_eq(refFromBytesU64(b64) ?? 0, 0x0102030405060708 as u64);
    });
}
