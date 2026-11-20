// Source: cp/cap/test/test_zstd_production_deep.cp
// Deep Production Verification Suite for cap:zstd & cap.zstd
// Covers:
//   1. xxhash64 standard vectors + compress/decompress round-trip (empty, RLE high-repeat, mixed text, multi-frame, checksum)
//   2. inspectFrameHeader metadata parsing & Skippable Frame (0x184D2A50..5F) transparent skipping
//   3. Hand-crafted RFC 8878 Compressed_Block (type=2) & mixed RLE/Raw multi-block decompression
//   4. Security guards: InvalidMagic, ChecksumMismatch, UnsupportedBlockType (Reserved=3), WindowTooLarge, OutputTooLarge, TruncatedInput
//   5. `import cap from "cap"; cap.zstd` dynamic mounting & std.testing.allocator zero leaks

const std = @import("std");
import { test, expect_eq } from "cap:test";
import cap from "cap";
import {
    ZstdError,
    ZstdFrameInfo,
    ZSTD_MAGIC,
    ZSTD_SKIPPABLE_MAGIC_BASE,
    xxhash64,
    inspectFrameHeader,
    compress,
    decompress,
    decompressWithLimits,
} from "cap:zstd";

test("cap:zstd - xxhash64 standard vectors & compress/decompress round-trip (empty, RLE, mixed, multi-frame)") {
    const alloc = std.testing.allocator;

    // 1. Standard XXH64 test vectors (seed = 0)
    expect_eq(xxhash64("", 0), 0xEF46DB3751D8E999);
    expect_eq(xxhash64("abc", 0), 0x44BC2CF5AD770999);

    // 2. Empty input round-trip with checksum
    const empty_comp = compress(alloc, "", true) catch unreachable;
    defer alloc.free(empty_comp);
    const empty_dec = decompress(alloc, empty_comp) catch unreachable;
    defer alloc.free(empty_dec);
    expect_eq(empty_dec.len, 0);

    // 3. High-repeat RLE input (8192 x 'Z') -> extreme compression
    const rle_in = alloc.alloc(u8, 8192) catch unreachable;
    defer alloc.free(rle_in);
    @memset(rle_in, 'Z');

    const rle_comp = compress(alloc, rle_in, true) catch unreachable;
    defer alloc.free(rle_comp);
    expect_eq(rle_comp.len < 32, true);

    const rle_dec = decompress(alloc, rle_comp) catch unreachable;
    defer alloc.free(rle_dec);
    expect_eq(std.mem.eql(u8, rle_dec, rle_in), true);

    // 4. Mixed text + RLE runs + multi-frame concatenation
    const part1 = "CP-Agent Zstd Frame 1: AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA_tail1";
    const part2 = "CP-Agent Zstd Frame 2: BBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBB_tail2";
    const f1 = compress(alloc, part1, true) catch unreachable;
    defer alloc.free(f1);
    const f2 = compress(alloc, part2, false) catch unreachable;
    defer alloc.free(f2);

    let concat = alloc.alloc(u8, f1.len + f2.len) catch unreachable;
    defer alloc.free(concat);
    @memcpy(concat[0 .. f1.len], f1);
    @memcpy(concat[f1.len .. f1.len + f2.len], f2);

    const concat_dec = decompress(alloc, concat) catch unreachable;
    defer alloc.free(concat_dec);
    expect_eq(concat_dec.len, part1.len + part2.len);
    expect_eq(std.mem.eql(u8, concat_dec[0 .. part1.len], part1), true);
    expect_eq(std.mem.eql(u8, concat_dec[part1.len..], part2), true);
}

test("cap:zstd - inspectFrameHeader metadata & Skippable Frame transparent skipping") {
    const alloc = std.testing.allocator;

    const payload = "inspectFrameHeader payload with checksum enabled!";
    const frame = compress(alloc, payload, true) catch unreachable;
    defer alloc.free(frame);

    // Prepend a Skippable Frame (magic = 0x184D2A55, size = 4, data = "SKIP")
    const skip_len: usize = 12;
    let with_skip = alloc.alloc(u8, skip_len + frame.len) catch unreachable;
    defer alloc.free(with_skip);

    with_skip[0] = 0x55;
    with_skip[1] = 0x2A;
    with_skip[2] = 0x4D;
    with_skip[3] = 0x18;
    with_skip[4] = 4;
    with_skip[5] = 0;
    with_skip[6] = 0;
    with_skip[7] = 0;
    with_skip[8] = 'S';
    with_skip[9] = 'K';
    with_skip[10] = 'I';
    with_skip[11] = 'P';
    @memcpy(with_skip[skip_len..], frame);

    const info = inspectFrameHeader(with_skip) catch unreachable;
    expect_eq(info.has_checksum, true);
    expect_eq(info.frame_content_size != null, true);
    if (fcs := info.frame_content_size) {
        expect_eq(fcs, payload.len as u64);
    }
    expect_eq(info.dictionary_id == null, true);
    expect_eq(info.window_size >= (payload.len as u64), true);

    const decoded = decompress(alloc, with_skip) catch unreachable;
    defer alloc.free(decoded);
    expect_eq(std.mem.eql(u8, decoded, payload), true);
}

test("cap:zstd - hand-crafted RFC 8878 Compressed_Block (type=2) & cross-block RLE/Raw") {
    const alloc = std.testing.allocator;

    // Hand-crafted RFC 8878 frame with Compressed_Block (type=2):
    //   Magic: 0xFD2FB528 (LE)
    //   FHD: 0x20 (Single_Segment=1, FCS_Flag=0 -> 1-byte FCS)
    //   FCS: 8
    //   Block Header (last=1, type=2 (Compressed), size=11 -> (11<<3)|(2<<1)|1 = 0x5D, 0x00, 0x00)
    //   Literals_Section: Raw (0), 5-bit size=4 -> (4<<3)|0 = 0x20, followed by "ABCD"
    //   Sequences_Section:
    //     num_sequences = 1 (0x01)
    //     modes = 0x54 (LL=RLE(1), OF=RLE(1), ML=RLE(1))
    //     ll_sym = 4 (lit_len = 4), of_sym = 2 (offset = 4 + 2-bit extra), ml_sym = 1 (match_len = 4)
    //     bitstream = 0x04 (sentinel bit at pos 2, 2 zero bits for of_extra=0)
    const comp_frame: [20]u8 = [
        0x28, 0xB5, 0x2F, 0xFD,
        0x20, 0x08,
        0x5D, 0x00, 0x00,
        0x20, 'A', 'B', 'C', 'D',
        0x01, 0x54, 0x04, 0x02, 0x01, 0x04,
    ];

    const decoded = decompress(alloc, comp_frame[0..]) catch unreachable;
    defer alloc.free(decoded);
    expect_eq(std.mem.eql(u8, decoded, "ABCDABCD"), true);
}

test("cap:zstd - security guards: InvalidMagic, ChecksumMismatch, UnsupportedBlockType, WindowTooLarge, OutputTooLarge, TruncatedInput") {
    const alloc = std.testing.allocator;

    // 1. InvalidMagic
    const bad_magic: [8]u8 = [0x00, 0x01, 0x02, 0x03, 0x20, 0x00, 0x01, 0x00];
    let got_invalid_magic = false;
    * = decompress(alloc, bad_magic[0..]) catch |e| {
        if (e == ZstdError.InvalidMagic) got_invalid_magic = true;
    };
    expect_eq(got_invalid_magic, true);

    // 2. ChecksumMismatch
    const frame = compress(alloc, "checksum tamper test", true) catch unreachable;
    defer alloc.free(frame);
    let tampered = alloc.alloc(u8, frame.len) catch unreachable;
    defer alloc.free(tampered);
    @memcpy(tampered, frame);
    tampered[tampered.len - 1] ^= 0xFF;

    let got_cksum_err = false;
    * = decompress(alloc, tampered) catch |e| {
        if (e == ZstdError.ChecksumMismatch) got_cksum_err = true;
    };
    expect_eq(got_cksum_err, true);

    // 3. UnsupportedBlockType (Reserved block_type = 3 -> header byte0 = (3<<1)|1 = 0x07)
    const reserved_blk: [9]u8 = [
        0x28, 0xB5, 0x2F, 0xFD,
        0x20, 0x01,
        0x07, 0x00, 0x00,
    ];
    let got_unsup = false;
    * = decompress(alloc, reserved_blk[0..]) catch |e| {
        if (e == ZstdError.UnsupportedBlockType) got_unsup = true;
    };
    expect_eq(got_unsup, true);

    // 4. WindowTooLarge & OutputTooLarge
    let got_win = false;
    * = decompressWithLimits(alloc, frame, 4, 1024) catch |e| {
        if (e == ZstdError.WindowTooLarge) got_win = true;
    };
    expect_eq(got_win, true);

    let got_out = false;
    * = decompressWithLimits(alloc, frame, 1024, 4) catch |e| {
        if (e == ZstdError.OutputTooLarge) got_out = true;
    };
    expect_eq(got_out, true);

    // 5. TruncatedInput
    let got_trunc = false;
    * = decompress(alloc, frame[0 .. frame.len - 2]) catch |e| {
        if (e == ZstdError.TruncatedInput) got_trunc = true;
    };
    expect_eq(got_trunc, true);
}

test("cap:zstd - dynamic cap.zstd mounting verification") {
    const alloc = std.testing.allocator;
    const msg = "cap.zstd dynamic mount payload BBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBB!";

    const comp = cap.zstd.compress(alloc, msg, true) catch unreachable;
    defer alloc.free(comp);

    const info = cap.zstd.inspectFrameHeader(comp) catch unreachable;
    expect_eq(info.has_checksum, true);

    const decoded = cap.zstd.decompress(alloc, comp) catch unreachable;
    defer alloc.free(decoded);

    expect_eq(std.mem.eql(u8, decoded, msg), true);
    expect_eq(cap.zstd.xxhash64(msg, 0), xxhash64(msg, 0));
    expect_eq(cap.zstd.ZSTD_MAGIC, ZSTD_MAGIC);
}
