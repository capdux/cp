import { test, expect_eq } from "cap:test";
import { bytesEq } from "cap:bin";
import {
    KIB,
    MIB,
    GIB,
    KB,
    MB,
    GB,
    parse,
    parseBytes,
    formatBytes,
    formatIec,
    formatSi,
    BYTES_MUT_CAP,
    BytesReader,
    BytesMut,
} from "../../packages/bytes/src/index.cp";

fn main() {
    // --- npm `bytes` size parser & formatter tests ---
    test("1. @cap/bytes [npm] — binary default parsing", () => {
        expect_eq(parse("0"), 0);
        expect_eq(parse("512B"), 512);
        expect_eq(parse("1KB"), KIB as i64);
        expect_eq(parse("1.5 MB"), (MIB + MIB / 2) as i64);
        expect_eq(parse("2gb"), (2 * GIB) as i64);
    });

    test("2. @cap/bytes [npm] — SI decimal vs explicit IEC binary units", () => {
        expect_eq(parseBytes("1KB", false), KB as i64);
        expect_eq(parseBytes("2.5 MB", false), (2 * MB + MB / 2) as i64);
        expect_eq(parseBytes("1GB", false), GB as i64);
        expect_eq(parseBytes("1KiB", false), KIB as i64);
        expect_eq(parseBytes("1MiB", false), MIB as i64);
    });

    test("3. @cap/bytes [npm] — formatIec formatting across scales", () => {
        let buf: [64]u8 = undefined;
        let n0: usize = formatIec(0, buf[0..]);
        expect_eq(bytesEq(buf[0..n0], "0 B"), true);

        let n1: usize = formatIec(1536, buf[0..]);
        expect_eq(bytesEq(buf[0..n1], "1.50 KiB"), true);

        let n2: usize = formatIec(MIB * 5, buf[0..]);
        expect_eq(bytesEq(buf[0..n2], "5.00 MiB"), true);

        let n3: usize = formatIec(GIB * 2 + MIB * 512, buf[0..]);
        expect_eq(bytesEq(buf[0..n3], "2.50 GiB"), true);
    });

    test("4. @cap/bytes [npm] — formatSi and custom decimal places", () => {
        let buf: [64]u8 = undefined;
        let n1: usize = formatSi(1500, buf[0..]);
        expect_eq(bytesEq(buf[0..n1], "1.50 KB"), true);

        let n2: usize = formatBytes(2500000, false, 1, buf[0..]);
        expect_eq(bytesEq(buf[0..n2], "2.5 MB"), true);

        let n3: usize = formatBytes(3000000000, false, 0, buf[0..]);
        expect_eq(bytesEq(buf[0..n3], "3 GB"), true);
    });

    test("5. @cap/bytes [npm] — invalid inputs and small output buffer protection", () => {
        expect_eq(parse(""), -1);
        expect_eq(parse("   "), -1);
        expect_eq(parse("KB"), -1);
        expect_eq(parse("100 XB"), -1);
        expect_eq(parse("-50 MB"), -1);

        let tiny: [3]u8 = undefined;
        expect_eq(formatIec(1536, tiny[0..]), 0);
        expect_eq(formatSi(500, tiny[0..]), 0);
    });

    // --- crates.io `bytes` Buf & BufMut cursor tests ---
    test("6. @cap/bytes [crates.io] — Big-endian put/get round-trip for u8/u16/u32/u64", () => {
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

    test("7. @cap/bytes [crates.io] — Little-endian put/get round-trip", () => {
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

    test("8. @cap/bytes [crates.io] — putSlice, copyToSlice, and splitTo", () => {
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

        let bm2 = BytesMut.new();
        * = bm2.putSlice("ABCDEFGH");
        let split_head: [3]u8 = undefined;
        expect_eq(bm2.splitTo(3, split_head[0..3]), 3);
        expect_eq(split_head[0], 65);
        expect_eq(split_head[2], 67);
        expect_eq(bm2.len(), 5);
        expect_eq(bm2.byteAt(0), 68); // 'D'
        expect_eq(bm2.byteAt(4), 72); // 'H'
    });

    test("9. @cap/bytes [crates.io] — truncate, clear, advance, and underflow protection", () => {
        let bm = BytesMut.new();
        * = bm.putSlice("1234567890");
        bm.truncate(4);
        expect_eq(bm.len(), 4);
        bm.clear();
        expect_eq(bm.isEmpty(), true);
        expect_eq(bm.remainingMut(), BYTES_MUT_CAP);

        let empty: [1]u8 = [0];
        let r = BytesReader.new(empty[0..1]);
        expect_eq(r.getU16Be().ok, false);
        expect_eq(r.getU32Le().ok, false);
        expect_eq(r.getU64Be().ok, false);
        expect_eq(r.advance(2), false);
        expect_eq(r.advance(1), true);
        expect_eq(r.position(), 1);
        expect_eq(r.getU8().ok, false);
    });
}
