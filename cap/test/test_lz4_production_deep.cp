// Source: cp/cap/test/test_lz4_production_deep.cp
// Deep Production Verification Suite for cap:lz4 & cap.lz4
// Covers:
//   1. Lz4Block round-trip on high-repeat text, binary RLE overlap (all zeros), empty & short (<12B) inputs + compression ratio < 20%
//   2. Lz4Frame round-trip with & without content_checksum, plus large multi-block payload (>100KB)
//   3. Security guards: InvalidOffset (0 & out-of-bounds), OutputTooLarge, InvalidMagic, InvalidHeaderChecksum, InvalidContentChecksum
//   4. xxhash32 standard test vectors & std.testing.allocator zero memory leaks
//   5. `import cap from "cap"; cap.lz4` dynamic mounting verification

const std = @import("std");
import { test, expect_eq } from "cap:test";
import cap from "cap";
import {
    Lz4Error,
    LZ4_FRAME_MAGIC,
    xxhash32,
    Lz4Block,
    Lz4Frame,
} from "cap:pack";

test("cap:lz4 - Lz4Block round-trip: high-repeat text, RLE zeros, empty & short (<12B) inputs") {
    const alloc = std.testing.allocator;

    // 1. Empty input
    const empty_comp = Lz4Block.compress(alloc, "") catch unreachable;
    defer alloc.free(empty_comp);
    expect_eq(empty_comp.len, 0);
    const empty_dec = Lz4Block.decompress(alloc, empty_comp, 1024) catch unreachable;
    defer alloc.free(empty_dec);
    expect_eq(empty_dec.len, 0);

    // 2. Short inputs (< 12 bytes)
    const short_inputs = ["a", "hello", "12345678901"];
    for (s of short_inputs) {
        const c = Lz4Block.compress(alloc, s) catch unreachable;
        defer alloc.free(c);
        const d = Lz4Block.decompress(alloc, c, 1024) catch unreachable;
        defer alloc.free(d);
        expect_eq(std.mem.eql(u8, d, s), true);
    }

    // 3. High-repeat text (> 4KB) -> compression ratio < 20%
    const unit = "CP_AGENT_WAL_RECORD_0001_PAYLOAD_DATA_";
    const repeat_count: usize = 128;
    const text_buf = alloc.alloc(u8, unit.len * repeat_count) catch unreachable;
    defer alloc.free(text_buf);
    for (i of 0..repeat_count) {
        const start = i * unit.len;
        @memcpy(text_buf[start .. start + unit.len], unit);
    }

    const text_comp = Lz4Block.compress(alloc, text_buf) catch unreachable;
    defer alloc.free(text_comp);
    // Significant compression: < 20% of original size
    expect_eq(text_comp.len * 5 < text_buf.len, true);

    const text_dec = Lz4Block.decompress(alloc, text_comp, text_buf.len) catch unreachable;
    defer alloc.free(text_dec);
    expect_eq(std.mem.eql(u8, text_dec, text_buf), true);

    // 4. Binary all-zero RLE overlapping match (offset == 1, match_len >> 1)
    const zero_buf = alloc.alloc(u8, 8192) catch unreachable;
    defer alloc.free(zero_buf);
    @memset(zero_buf, 0);

    const zero_comp = Lz4Block.compress(alloc, zero_buf) catch unreachable;
    defer alloc.free(zero_comp);
    expect_eq(zero_comp.len * 5 < zero_buf.len, true);

    const zero_dec = Lz4Block.decompress(alloc, zero_comp, zero_buf.len) catch unreachable;
    defer alloc.free(zero_dec);
    expect_eq(std.mem.eql(u8, zero_dec, zero_buf), true);
}

test("cap:lz4 - Lz4Frame with & without content_checksum + large multi-block (>100KB) payload") {
    const alloc = std.testing.allocator;

    // 110 KB (> 100KB, crosses 64KB block boundary into 2 blocks)
    const total_len: usize = 112640;
    let big_buf = alloc.alloc(u8, total_len) catch unreachable;
    defer alloc.free(big_buf);
    for (i of 0..total_len) {
        // Mix of compressible pattern and varying bytes
        big_buf[i] = if ((i % 64) < 48) ('A' + ((i / 64) % 26) as u8) else ((i * 31) & 0xFF) as u8;
    }

    // With content checksum
    const frame_cksum = Lz4Frame.compressFrame(alloc, big_buf, true) catch unreachable;
    defer alloc.free(frame_cksum);
    expect_eq(frame_cksum[0], 0x04);
    expect_eq(frame_cksum[1], 0x22);
    expect_eq(frame_cksum[2], 0x4D);
    expect_eq(frame_cksum[3], 0x18);

    const dec_cksum = Lz4Frame.decompressFrame(alloc, frame_cksum, total_len) catch unreachable;
    defer alloc.free(dec_cksum);
    expect_eq(std.mem.eql(u8, dec_cksum, big_buf), true);

    // Without content checksum
    const frame_no_cksum = Lz4Frame.compressFrame(alloc, big_buf, false) catch unreachable;
    defer alloc.free(frame_no_cksum);
    expect_eq(frame_cksum.len, frame_no_cksum.len + 4);

    const dec_no_cksum = Lz4Frame.decompressFrame(alloc, frame_no_cksum, total_len) catch unreachable;
    defer alloc.free(dec_no_cksum);
    expect_eq(std.mem.eql(u8, dec_no_cksum, big_buf), true);
}

test("cap:lz4 - security guards: InvalidOffset, OutputTooLarge, InvalidMagic, Header/Content Checksum") {
    const alloc = std.testing.allocator;

    // 1. InvalidOffset: offset == 0
    // Token = 0x10 (1 literal 'A', match_len=4), offset = 0x0000
    const bad_zero_off: [4]u8 = [0x10, 'A', 0x00, 0x00];
    let got_zero_off = false;
    * = Lz4Block.decompress(alloc, bad_zero_off[0..], 1024) catch e => {
        if (e == Lz4Error.InvalidOffset) got_zero_off = true;
    };
    expect_eq(got_zero_off, true);

    // 2. InvalidOffset: offset > out.len (out.len == 1, offset == 5)
    const bad_oob_off: [4]u8 = [0x10, 'A', 0x05, 0x00];
    let got_oob_off = false;
    * = Lz4Block.decompress(alloc, bad_oob_off[0..], 1024) catch e => {
        if (e == Lz4Error.InvalidOffset) got_oob_off = true;
    };
    expect_eq(got_oob_off, true);

    // 3. OutputTooLarge: anti-bomb protection
    const sample = "AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA";
    const comp = Lz4Block.compress(alloc, sample) catch unreachable;
    defer alloc.free(comp);
    let got_too_large = false;
    * = Lz4Block.decompress(alloc, comp, 16) catch e => {
        if (e == Lz4Error.OutputTooLarge) got_too_large = true;
    };
    expect_eq(got_too_large, true);

    // 4. Frame guards: InvalidMagic, InvalidHeaderChecksum, InvalidContentChecksum
    const frame = Lz4Frame.compressFrame(alloc, sample, true) catch unreachable;
    defer alloc.free(frame);

    let mutated = alloc.dupe(u8, frame) catch unreachable;
    defer alloc.free(mutated);

    // 4a. InvalidMagic
    mutated[0] ^= 0xFF;
    let got_magic = false;
    * = Lz4Frame.decompressFrame(alloc, mutated, 1024) catch e => {
        if (e == Lz4Error.InvalidMagic) got_magic = true;
    };
    expect_eq(got_magic, true);
    mutated[0] ^= 0xFF;

    // 4b. InvalidHeaderChecksum (corrupt HC byte at index 6)
    mutated[6] ^= 0x01;
    let got_hc = false;
    * = Lz4Frame.decompressFrame(alloc, mutated, 1024) catch e => {
        if (e == Lz4Error.InvalidHeaderChecksum) got_hc = true;
    };
    expect_eq(got_hc, true);
    mutated[6] ^= 0x01;

    // 4c. InvalidContentChecksum (corrupt last byte of content checksum)
    mutated[mutated.len - 1] ^= 0x01;
    let got_cc = false;
    * = Lz4Frame.decompressFrame(alloc, mutated, 1024) catch e => {
        if (e == Lz4Error.InvalidContentChecksum) got_cc = true;
    };
    expect_eq(got_cc, true);
}

test("cap:lz4 - xxhash32 standard test vectors & zero leaks") {
    expect_eq(xxhash32("", 0), 0x02CC5D05);
    expect_eq(xxhash32("", 0x9E3779B1), 0x36B78AE7);
    expect_eq(xxhash32("abc", 0), 0x32D153FF);
    expect_eq(xxhash32("The quick brown fox jumps over the lazy dog", 0), 0xE85EA4DE);
    expect_eq(LZ4_FRAME_MAGIC, 0x184D2204);
}

test("cap:lz4 - dynamic cap.lz4 mounting verification") {
    const alloc = std.testing.allocator;
    const msg = "cap.lz4 dynamic mount test payload repeated repeated repeated!";

    const frame = cap.pack.Lz4Frame.compressFrame(alloc, msg, true) catch unreachable;
    defer alloc.free(frame);

    const decoded = cap.pack.Lz4Frame.decompressFrame(alloc, frame, 4096) catch unreachable;
    defer alloc.free(decoded);

    expect_eq(std.mem.eql(u8, decoded, msg), true);
    expect_eq(cap.pack.xxhash32(msg, 0), xxhash32(msg, 0));
}
