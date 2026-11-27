import { test, expect_eq } from "cap:test";
import { BytesReader, BytesMut, BYTES_MUT_CAP } from "../../packages/bytes/src/index.cp";

fn main() {
    test("1. @cap/bytes-buf — Big-endian put/get round-trip for u8/u16/u32/u64", () => {
        let bm = BytesMut.new();
        expect_eq(bm.putU8(0xAB), true);
        expect_eq(bm.putU16Be(0x1234), true);
        expect_eq(bm.putU32Be(0xDEADBEEF), true);
        let big: u64 = ((0x01020304 as u64) << 32) | (0x05060708 as u64);
        expect_eq(bm.putU64Be(big), true);
        expect_eq(bm.len(), 15);

        let raw: [32]u8 = undefined;
        let n = bm.copyAll(raw[0..32]);
        let r = BytesReader.new(raw[0..n]);
        expect_eq(r.getU8().val, 0xAB);
        expect_eq(r.getU16Be().val, 0x1234);
        expect_eq(r.getU32Be().val, 0xDEADBEEF);
        expect_eq(r.getU64Be().val, big);
        expect_eq(r.hasRemaining(), false);
    });

    test("2. @cap/bytes-buf — Little-endian put/get round-trip", () => {
        let bm = BytesMut.new();
        expect_eq(bm.putU16Le(0x5678), true);
        expect_eq(bm.putU32Le(0xCAFEBABE), true);
        let big: u64 = ((0xAABBCCDD as u64) << 32) | (0x11223344 as u64);
        expect_eq(bm.putU64Le(big), true);

        let raw: [32]u8 = undefined;
        let n = bm.copyAll(raw[0..32]);
        expect_eq(raw[0], 0x78);
        expect_eq(raw[1], 0x56);

        let r = BytesReader.new(raw[0..n]);
        expect_eq(r.getU16Le().val, 0x5678);
        expect_eq(r.getU32Le().val, 0xCAFEBABE);
        expect_eq(r.getU64Le().val, big);
    });

    test("3. @cap/bytes-buf — putSlice and copyToSlice", () => {
        let bm = BytesMut.new();
        expect_eq(bm.putSlice("hello"), true);
        expect_eq(bm.putSlice(" world"), true);
        expect_eq(bm.len(), 11);

        let raw: [16]u8 = undefined;
        let n = bm.copyAll(raw[0..16]);
        let r = BytesReader.new(raw[0..n]);
        let head: [5]u8 = undefined;
        expect_eq(r.copyToSlice(head[0..5]), true);
        expect_eq(head[0], 104); // 'h'
        expect_eq(head[4], 111); // 'o'
        expect_eq(r.remaining(), 6);
    });

    test("4. @cap/bytes-buf — splitTo shifts remaining bytes cleanly", () => {
        let bm = BytesMut.new();
        * = bm.putSlice("ABCDEFGH");
        let head: [3]u8 = undefined;
        expect_eq(bm.splitTo(3, head[0..3]), 3);
        expect_eq(head[0], 65);
        expect_eq(head[2], 67);
        expect_eq(bm.len(), 5);
        expect_eq(bm.byteAt(0), 68); // 'D'
        expect_eq(bm.byteAt(4), 72); // 'H'
    });

    test("5. @cap/bytes-buf — truncate, clear, and advance bounds", () => {
        let bm = BytesMut.new();
        * = bm.putSlice("1234567890");
        bm.truncate(4);
        expect_eq(bm.len(), 4);
        bm.clear();
        expect_eq(bm.isEmpty(), true);
        expect_eq(bm.remainingMut(), BYTES_MUT_CAP);
    });

    test("6. @cap/bytes-buf — Underflow and capacity overflow protection", () => {
        let empty: [1]u8 = [0];
        let r = BytesReader.new(empty[0..1]);
        expect_eq(r.getU16Be().ok, false);
        expect_eq(r.getU32Le().ok, false);
        expect_eq(r.getU64Be().ok, false);
        expect_eq(r.advance(2), false);
        expect_eq(r.advance(1), true);
        expect_eq(r.getU8().ok, false);
    });
}
