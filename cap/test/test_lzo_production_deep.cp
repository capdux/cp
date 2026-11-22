// Source: cp/cap/test/test_lzo_production_deep.cp (Production deep tests for cap:lzo)

import std from "std";
import { test, expect_eq } from "cap:test";
import {
    LZOP_MAGIC,
    METHOD_LZO1X_1,
    METHOD_LZO1X_1_15,
    METHOD_LZO1X_999,
    F_ADLER32_D,
    F_ADLER32_C,
    F_STDIN,
    F_STDOUT,
    F_EXTRA_FIELD,
    F_CRC32_D,
    F_CRC32_C,
    F_H_CRC32,
    lzoAdler32,
    lzoCrc32,
    LzopHeader,
    LzopBlock,
    LzopBlockIter,
    isLzop,
    parseLzopHeader,
    iterLzopBlocks,
    lzo1xDecompress,
} from "cap:pack";

fn putU16Be(buf: []u8, off: usize, v: u16) {
    buf[off + 0] = ((v >> 8) & 0xFF) as u8;
    buf[off + 1] = (v & 0xFF) as u8;
}

fn putU32Be(buf: []u8, off: usize, v: u32) {
    buf[off + 0] = ((v >> 24) & 0xFF) as u8;
    buf[off + 1] = ((v >> 16) & 0xFF) as u8;
    buf[off + 2] = ((v >> 8) & 0xFF) as u8;
    buf[off + 3] = (v & 0xFF) as u8;
}

fn putBytes(buf: []u8, off: usize, src: []const u8) {
    let i: usize = 0;
    while (i < src.len) {
        buf[off + i] = src[i];
        i += 1;
    }
}

fn slicesEq(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) {
        return false;
    }
    let i: usize = 0;
    while (i < a.len) {
        if (a[i] != b[i]) {
            return false;
        }
        i += 1;
    }
    return true;
}

fn writeLzopHeader(buf: []u8, flags: u32, filename: []const u8): usize {
    let i: usize = 0;
    while (i < 9) {
        buf[i] = LZOP_MAGIC[i];
        i += 1;
    }
    let pos: usize = 9;
    putU16Be(buf, pos, 0x1040); pos += 2; // version
    putU16Be(buf, pos, 0x20A0); pos += 2; // lib_version
    putU16Be(buf, pos, 0x0940); pos += 2; // version_needed
    buf[pos] = METHOD_LZO1X_1; pos += 1;  // method
    buf[pos] = 5; pos += 1;               // level
    putU32Be(buf, pos, flags); pos += 4;  // flags
    putU32Be(buf, pos, 0x81A4); pos += 4; // mode
    putU32Be(buf, pos, 1700000000); pos += 4; // mtime_low
    putU32Be(buf, pos, 0); pos += 4;          // mtime_high
    buf[pos] = filename.len as u8; pos += 1;
    putBytes(buf, pos, filename); pos += filename.len;

    const cksum = if ((flags & F_H_CRC32) != 0) {
        lzoCrc32(buf[9..pos])
    } else {
        lzoAdler32(buf[9..pos])
    };
    putU32Be(buf, pos, cksum); pos += 4;
    return pos;
}

test("1. 9B lzop Header parsing with both Adler32 and F_H_CRC32 header verification", () => {
    // Standard checksum vectors
    expect_eq(lzoAdler32("Wikipedia"), 0x11E60398 as u32);
    expect_eq(lzoCrc32("123456789"), 0xCBF43926 as u32);

    // 1a. Header with Adler32 checksum
    let buf1: [128]u8 = std.mem.zeroes([128]u8);
    const len1 = writeLzopHeader(buf1[0..], F_ADLER32_D | F_ADLER32_C, "hello.txt");
    expect_eq(isLzop(buf1[0..len1]), true);
    if (h := parseLzopHeader(buf1[0..len1])) {
        expect_eq(h.version, 0x1040 as u16);
        expect_eq(h.lib_version, 0x20A0 as u16);
        expect_eq(h.version_needed, 0x0940 as u16);
        expect_eq(h.method, METHOD_LZO1X_1);
        expect_eq(h.level, 5 as u8);
        expect_eq(h.usesHeaderCrc32(), false);
        expect_eq(h.hasUncompressedAdler32(), true);
        expect_eq(h.hasCompressedAdler32(), true);
        expect_eq(h.hasUncompressedCrc32(), false);
        expect_eq(h.hasCompressedCrc32(), false);
        expect_eq(slicesEq(h.filename, "hello.txt"), true);
        expect_eq(h.blocks_offset, len1);
    } else {
        expect_eq(true, false);
    }

    // 1b. Header with F_H_CRC32 checksum
    let buf2: [128]u8 = std.mem.zeroes([128]u8);
    const len2 = writeLzopHeader(buf2[0..], F_H_CRC32 | F_CRC32_D | F_CRC32_C, "data.bin");
    if (h := parseLzopHeader(buf2[0..len2])) {
        expect_eq(h.usesHeaderCrc32(), true);
        expect_eq(h.hasUncompressedCrc32(), true);
        expect_eq(h.hasCompressedCrc32(), true);
        expect_eq(slicesEq(h.filename, "data.bin"), true);
    } else {
        expect_eq(true, false);
    }

    expect_eq(METHOD_LZO1X_1_15, 2 as u8);
    expect_eq(METHOD_LZO1X_999, 3 as u8);
    expect_eq(F_STDIN, 0x04 as u32);
    expect_eq(F_STDOUT, 0x08 as u32);
    expect_eq(F_EXTRA_FIELD, 0x40 as u32);
});

test("2. LzopBlockIter multi-block iteration (uncompressed passthrough + compressed block w/ c_adler32 + EOF)", () => {
    let stream: [256]u8 = std.mem.zeroes([256]u8);
    let pos = writeLzopHeader(stream[0..], F_ADLER32_D | F_ADLER32_C, "multi.lzo");

    // Block 1: uncompressed passthrough ("RAW!")
    const raw1 = "RAW!";
    putU32Be(stream[0..], pos, 4); pos += 4;                // uncompressed_size = 4
    putU32Be(stream[0..], pos, 4); pos += 4;                // compressed_size = 4 (passthrough)
    putU32Be(stream[0..], pos, lzoAdler32(raw1)); pos += 4; // d_adler32
    putBytes(stream[0..], pos, raw1); pos += 4;

    // Block 2: LZO1X compressed ("AAAAAAAAAA" = 10 x 'A')
    // Compressed stream (7 bytes):
    //   0x12 ('18' -> init_lit = 1), 'A',
    //   0x27 (M3: lraw = 7 -> len = 9), 0x00, 0x00 (dist = 1, trail = 0),
    //   0x11, 0x00, 0x00 (EOS) -> 8 bytes (< 10)
    let comp2: [8]u8 = [0x12, 'A', 0x27, 0x00, 0x00, 0x11, 0x00, 0x00];
    putU32Be(stream[0..], pos, 10); pos += 4;                     // uncompressed_size = 10
    putU32Be(stream[0..], pos, 8); pos += 4;                      // compressed_size = 8
    putU32Be(stream[0..], pos, lzoAdler32("AAAAAAAAAA")); pos += 4; // d_adler32
    putU32Be(stream[0..], pos, lzoAdler32(comp2[0..])); pos += 4; // c_adler32
    putBytes(stream[0..], pos, comp2[0..]); pos += 8;

    // EOF sentinel: uncompressed_size = 0
    putU32Be(stream[0..], pos, 0); pos += 4;

    if (hdr := parseLzopHeader(stream[0..pos])) {
        let it = iterLzopBlocks(hdr, stream[0..pos]);
        if (b1 := it.next()) {
            expect_eq(b1.isUncompressed(), true);
            expect_eq(slicesEq(b1.payload, "RAW!"), true);
        } else {
            expect_eq(true, false);
        }

        if (b2 := it.next()) {
            expect_eq(b2.isUncompressed(), false);
            expect_eq(b2.uncompressed_size, 10 as u32);
            expect_eq(b2.compressed_size, 8 as u32);

            let out_arr: [32]u8 = std.mem.zeroes([32]u8);
            let out_slice: []u8 = out_arr[0..];
            if (dec_len := lzo1xDecompress(b2.payload, *out_slice)) {
                expect_eq(dec_len, 10 as usize);
                expect_eq(slicesEq(out_arr[0..dec_len], "AAAAAAAAAA"), true);
                expect_eq(lzoAdler32(out_arr[0..dec_len]), b2.d_adler32);
            } else {
                expect_eq(true, false);
            }
        } else {
            expect_eq(true, false);
        }

        // EOF
        if (_ := it.next()) {
            expect_eq(true, false);
        } else {
            expect_eq(it.hasError(), false);
        }
    } else {
        expect_eq(true, false);
    }
});

test("3. lzo1xDecompress: literal runs, M2 back-reference, and overlapping 1-byte RLE expansion", () => {
    // Target output: "ABCDABCD" + 20 * 'Z' + "END" (31 bytes)
    // Stream:
    //   0x15 (21 -> init_lit = 4): "ABCD"
    //   M2 match: dist=4, len=4, trail=1 ->
    //     len=4 => (t >> 5) = 3 => 0x60; dist=4 => 1 + ((t>>2)&7) = 4 => (t>>2)&7 = 3 => 0x0C; trail=1 => 0x01
    //     => t = 0x6D, next_b = 0x00, trailing literal = 'Z'
    //   M3 match (overlapping RLE dist=1, len=19, trail=3):
    //     lraw = 19 - 2 = 17 => t = 0x20 | 17 = 0x31
    //     dist = 1 => lo >> 2 = 0, hi = 0; trail = 3 => lo = 0x03, hi = 0x00
    //     trailing literals = "END"
    //   EOS: 0x11, 0x00, 0x00
    let src: [17]u8 = [
        0x15, 'A', 'B', 'C', 'D',
        0x6D, 0x00, 'Z',
        0x31, 0x03, 0x00, 'E', 'N', 'D',
        0x11, 0x00, 0x00,
    ];

    let dst_arr: [64]u8 = std.mem.zeroes([64]u8);
    let dst_slice: []u8 = dst_arr[0..];
    if (n := lzo1xDecompress(src[0..], *dst_slice)) {
        expect_eq(n, 31 as usize);
        expect_eq(slicesEq(dst_arr[0..8], "ABCDABCD"), true);
        let i: usize = 8;
        while (i < 28) {
            expect_eq(dst_arr[i], 'Z' as u8);
            i += 1;
        }
        expect_eq(slicesEq(dst_arr[28..31], "END"), true);
    } else {
        expect_eq(true, false);
    }
});

test("4. lzo1xDecompress: long-distance M4 back-reference (> 16384B sliding window)", () => {
    // Produce 16388 bytes:
    //   1. "M4OK" (4 bytes via init_lit: 0x15, 'M', '4', 'O', 'K')
    //   2. 16384 bytes of 'X' using extended M3 RLE:
    //      First emit 'X' via trail=1? Simpler: init_lit=5 ("M4OKX"), then M3 RLE dist=1, len=16383
    //      Wait: len=16383 => lraw=16381 => 31 + ext = 16381 => ext = 16350
    //      16350 / 255 = 64 remainder 30 -> 64 zeroes + 30!
    //   3. M4 match copying "M4OK" from dist = 16388 (= 0x4000 + 4):
    //      t = 0x10 | (4 - 2) = 0x12 (dist_hi = 0, lraw = 2 -> mlen = 4)
    //      d = 4 => (lo >> 2) = 4 => lo = 16 (0x10), hi = 0x00, trail = 0
    //   4. EOS: 0x11, 0x00, 0x00
    let src: [80]u8 = std.mem.zeroes([80]u8);
    let p: usize = 0;
    src[p] = 22; p += 1; // init_lit = 5
    putBytes(src[0..], p, "M4OKX"); p += 5;

    // M3 RLE: dist=1, len=16383 (lraw=0 -> 31 + 64*255 + 30 = 16381 -> mlen=16383)
    src[p] = 0x20; p += 1; // M3 with lraw=0
    let z: usize = 0;
    while (z < 64) {
        src[p] = 0;
        p += 1;
        z += 1;
    }
    src[p] = 30; p += 1;
    src[p] = 0x00; p += 1; // lo: dist=1, trail=0
    src[p] = 0x00; p += 1; // hi=0

    // Now op == 5 + 16383 == 16388. Copy 4 bytes from offset 0 (dist = 16388 = 0x4000 + 4):
    src[p] = 0x12; p += 1; // M4: dist_hi=0, lraw=2 -> mlen=4
    src[p] = 0x10; p += 1; // lo: (16 >> 2) = 4, trail=0
    src[p] = 0x00; p += 1; // hi=0

    // EOS
    src[p] = 0x11; p += 1;
    src[p] = 0x00; p += 1;
    src[p] = 0x00; p += 1;

    let dst_arr: [16400]u8 = std.mem.zeroes([16400]u8);
    let dst_slice: []u8 = dst_arr[0..];
    if (n := lzo1xDecompress(src[0..p], *dst_slice)) {
        expect_eq(n, 16392 as usize);
        expect_eq(slicesEq(dst_arr[0..4], "M4OK"), true);
        expect_eq(slicesEq(dst_arr[16388..16392], "M4OK"), true);
    } else {
        expect_eq(true, false);
    }
});

test("5. Malformed input rejection: output overflow, invalid back-reference distance, bad header checksum, bad magic", () => {
    // 5a. Output buffer too small for LZO1X decompression
    let rle_src: [8]u8 = [0x12, 'A', 0x27, 0x00, 0x00, 0x11, 0x00, 0x00]; // expands to 10 bytes
    let tiny_arr: [9]u8 = std.mem.zeroes([9]u8);
    let tiny_slice: []u8 = tiny_arr[0..];
    if (_ := lzo1xDecompress(rle_src[0..], *tiny_slice)) {
        expect_eq(true, false);
    } else {
        expect_eq(true, true);
    }

    // 5b. Invalid back-reference distance (dist > op)
    let bad_dist: [8]u8 = [0x12, 'A', 0x22, 0x08, 0x00, 0x11, 0x00, 0x00]; // op=1, dist=3 (> 1)
    let ok_arr: [32]u8 = std.mem.zeroes([32]u8);
    let ok_slice: []u8 = ok_arr[0..];
    if (_ := lzo1xDecompress(bad_dist[0..], *ok_slice)) {
        expect_eq(true, false);
    } else {
        expect_eq(true, true);
    }

    // 5c. Bad lzop header checksum
    let hdr_buf: [64]u8 = std.mem.zeroes([64]u8);
    const hdr_len = writeLzopHeader(hdr_buf[0..], F_ADLER32_D, "bad.lzo");
    hdr_buf[hdr_len - 1] ^= 0xFF; // corrupt checksum
    expect_eq(isLzop(hdr_buf[0..hdr_len]), true);
    if (_ := parseLzopHeader(hdr_buf[0..hdr_len])) {
        expect_eq(true, false);
    } else {
        expect_eq(true, true);
    }

    // 5d. Bad magic & truncated header
    hdr_buf[0] = 0x00;
    expect_eq(isLzop(hdr_buf[0..hdr_len]), false);
    if (_ := parseLzopHeader(hdr_buf[0..hdr_len])) {
        expect_eq(true, false);
    } else {
        expect_eq(true, true);
    }
    if (_ := parseLzopHeader(hdr_buf[0..8])) {
        expect_eq(true, false);
    } else {
        expect_eq(true, true);
    }
});
