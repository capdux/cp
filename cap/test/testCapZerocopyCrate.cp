import { test, expect_eq } from "cap:test";
import {
    ByteCursor, ByteWriterCursor, isAlignedSlice
} from "../../packages/@cap/zerocopy-crate/src/index.cp";

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
}
