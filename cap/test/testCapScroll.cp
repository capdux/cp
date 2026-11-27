import { test, expect_eq } from "cap:test";
import {
    Endian,
    PreadCursor,
    PwriteCursor,
} from "../../packages/scroll/src/index.cp";

fn strEq(a: string, b: string): bool {
    if (a.len != b.len) { return false; }
    let i: usize = 0;
    while (i < a.len) {
        if (a[i] != b[i]) { return false; }
        i += 1;
    }
    return true;
}

fn main() {
    test("1. @cap/scroll — Little-endian PwriteCursor & PreadCursor round-trip", () => {
        let buf: [16]u8 = undefined;
        let w = PwriteCursor.new(buf[0..16]);

        const u64_val: u64 = ((0x01020304 as u64) << 32) | (0x05060708 as u64);
        expect_eq(w.writeU8(0xAB as u8), true);
        expect_eq(w.writeU16(0x1234 as u16, Endian.Little), true);
        expect_eq(w.writeU32(0xDEADBEEF as u32, Endian.Little), true);
        expect_eq(w.writeU64(u64_val, Endian.Little), true);
        expect_eq(w.written(), 15 as usize);
        expect_eq(w.remaining(), 1 as usize);

        // Verify raw little-endian byte order for u16 (0x34, 0x12)
        expect_eq(buf[1], 0x34 as u8);
        expect_eq(buf[2], 0x12 as u8);

        let r = PreadCursor.new(buf[0..15]);
        expect_eq(r.remaining(), 15 as usize);
        expect_eq(r.readU8().?, 0xAB as u8);
        expect_eq(r.readU16(Endian.Little).?, 0x1234 as u16);
        expect_eq(r.readU32(Endian.Little).?, 0xDEADBEEF as u32);
        expect_eq(r.readU64(Endian.Little).?, u64_val);
        expect_eq(r.remaining(), 0 as usize);
    });

    test("2. @cap/scroll — Big-endian (network byte order) round-trip", () => {
        let buf: [16]u8 = undefined;
        let w = PwriteCursor.new(buf[0..16]);

        const u64_val: u64 = ((0x11223344 as u64) << 32) | (0x55667788 as u64);
        expect_eq(w.writeU16(0xCAFE as u16, Endian.Big), true);
        expect_eq(w.writeU32(0x01020304 as u32, Endian.Big), true);
        expect_eq(w.writeU64(u64_val, Endian.Big), true);
        expect_eq(w.written(), 14 as usize);

        // Verify raw big-endian byte order
        expect_eq(buf[0], 0xCA as u8);
        expect_eq(buf[1], 0xFE as u8);
        expect_eq(buf[2], 0x01 as u8);
        expect_eq(buf[5], 0x04 as u8);

        let r = PreadCursor.new(buf[0..14]);
        expect_eq(r.readU16(Endian.Big).?, 0xCAFE as u16);
        expect_eq(r.readU32(Endian.Big).?, 0x01020304 as u32);
        expect_eq(r.readU64(Endian.Big).?, u64_val);
        expect_eq(r.remaining(), 0 as usize);
    });

    test("3. @cap/scroll — Signed readI32 in Little & Big endian (negative & positive)", () => {
        let buf: [8]u8 = undefined;
        let w = PwriteCursor.new(buf[0..8]);
        const neg_one: u32 = 0xFFFFFFFF as u32;
        const neg_1000: u32 = @as(u32, @bitCast(-1000 as i32));
        expect_eq(w.writeU32(neg_one, Endian.Little), true);
        expect_eq(w.writeU32(neg_1000, Endian.Big), true);

        let r = PreadCursor.new(buf[0..8]);
        expect_eq(r.readI32(Endian.Little).?, -1 as i32);
        expect_eq(r.readI32(Endian.Big).?, -1000 as i32);
        expect_eq(r.remaining(), 0 as usize);
    });

    test("4. @cap/scroll — readCstr sequential NUL-terminated string extraction", () => {
        const raw = "ELF\x00scroll\x00\x00tail";
        let r = PreadCursor.new(raw);

        const s1 = r.readCstr();
        expect_eq(s1 != null, true);
        expect_eq(strEq(s1.?, "ELF"), true);

        const s2 = r.readCstr();
        expect_eq(s2 != null, true);
        expect_eq(strEq(s2.?, "scroll"), true);

        const s3 = r.readCstr();
        expect_eq(s3 != null, true);
        expect_eq(s3.?.len, 0 as usize);

        // Unterminated trailing bytes return null without advancing
        const before = r.offset;
        expect_eq(r.readCstr() == null, true);
        expect_eq(r.offset, before);
        expect_eq(r.remaining(), 4 as usize);
    });

    test("5. @cap/scroll — Truncated read bounds protection preserves offset", () => {
        const short_buf: [3]u8 = [0x01, 0x02, 0x03];
        let r = PreadCursor.new(short_buf[0..3]);

        expect_eq(r.readU32(Endian.Little) == null, true);
        expect_eq(r.readU64(Endian.Big) == null, true);
        expect_eq(r.readI32(Endian.Little) == null, true);
        expect_eq(r.offset, 0 as usize);

        expect_eq(r.readU16(Endian.Big).?, 0x0102 as u16);
        expect_eq(r.remaining(), 1 as usize);
        expect_eq(r.readU16(Endian.Big) == null, true);
        expect_eq(r.readU8().?, 0x03 as u8);
        expect_eq(r.readU8() == null, true);
    });

    test("6. @cap/scroll — PwriteCursor overflow protection preserves offset", () => {
        let small: [5]u8 = [0, 0, 0, 0, 0];
        let w = PwriteCursor.new(small[0..5]);

        expect_eq(w.writeU64(1 as u64, Endian.Little), false);
        expect_eq(w.written(), 0 as usize);

        expect_eq(w.writeU32(0x11223344 as u32, Endian.Big), true);
        expect_eq(w.written(), 4 as usize);
        expect_eq(w.writeU16(0x5566 as u16, Endian.Big), false);
        expect_eq(w.written(), 4 as usize);
        expect_eq(w.writeU8(0x77 as u8), true);
        expect_eq(w.written(), 5 as usize);
        expect_eq(w.writeU8(0x88 as u8), false);
    });
}
