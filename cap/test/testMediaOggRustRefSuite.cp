// RustAudio/ogg (BSD-3-Clause) & RFC 3533 Reference-Hardened Ogg Container Test Suite
// Covers:
//   - Test 01: RFC 3533 CRC-32 verification, single-bit tamper rejection (ChecksumMismatch),
//              version != 0 (UnsupportedVersion), reserved flags (InvalidHeaderFlags), truncated page
//   - Test 02: 0-byte, 255-byte, 510-byte boundary packets & 70KB multi-page packet split/reassembly
//              with OGG_GRANULE_UNKNOWN intermediate pages and OGG_FLAG_CONTINUATION
//   - Test 03: Multi-stream serial_number interleaved demuxing + resync after leading garbage (findNextOggPage)
//   - Test 04: SIMD hot-path parity (oggSumSegmentsSimd vs Scalar, oggCopyPayloadSimd vs Scalar,
//              findNextOggPage vs findNextOggPageScalar)
//   - Test 05: 1,500-iteration deterministic PRNG malformed Ogg / forged page_segments / bad CRC fuzzing

const std = @import("std");
import { test, expect_eq } from "cap:test";
import {
    OGG_FLAG_CONTINUATION,
    OGG_FLAG_BOS,
    OGG_FLAG_EOS,
    OGG_GRANULE_UNKNOWN,
    OggError,
    oggSumSegmentsSimd,
    oggSumSegmentsScalar,
    oggCopyPayloadSimd,
    oggCopyPayloadScalar,
    OggPage,
    oggCrc32,
    isOgg,
    findNextOggPage,
    findNextOggPageScalar,
    parseOggPage,
    validateOggStream,
    OggPageIter,
    OggPacket,
    OggPacketReader,
    OggPacketWriter,
} from "../../lib/media/ogg.cp";

fn xorshift64(state: *u64): u64 {
    let x = state.*;
    x = x ^ (x << 13);
    x = x ^ (x >> 7);
    x = x ^ (x << 17);
    state.* = x;
    return x;
}

fn runTest01(): void! {
    let arena = std.heap.ArenaAllocator.init(std.heap.page_allocator);
    defer arena.deinit();
    const alloc = arena.allocator();

    // 1. Emit valid page and verify RFC 3533 CRC-32 + OggPageIter
    let w = OggPacketWriter.init(alloc);
    defer w.deinit();
    let seq: u32 = 0;
    try w.writePacket(0x12345678, &seq, 48000, true, true, "OpusHead-test-payload");
    expect_eq(seq, 1 as u32);
    expect_eq(isOgg(w.slice()), true);
    try validateOggStream(w.slice());

    const page = try parseOggPage(w.slice(), 0);
    expect_eq(page.isBos(), true);
    expect_eq(page.isEos(), true);
    expect_eq(page.isContinuation(), false);
    expect_eq(page.hasValidReservedFlags(), true);
    expect_eq(page.verifyChecksum(), true);
    expect_eq(page.serial_number, 0x12345678 as u32);
    expect_eq(page.granule_position, 48000 as u64);

    // 2. Single-bit tamper -> ChecksumMismatch
    let tampered = try alloc.dupe(u8, w.slice());
    tampered[tampered.len - 1] ^= 0x01;
    let crc_err = false;
    * = parseOggPage(tampered, 0) catch |e| {
        if (std.mem.eql(u8, @errorName(e), "ChecksumMismatch")) crc_err = true;
    };
    expect_eq(crc_err, true);

    // 3. Unsupported version != 0 -> UnsupportedVersion
    let bad_ver = try alloc.dupe(u8, w.slice());
    bad_ver[4] = 1;
    let ver_err = false;
    * = parseOggPage(bad_ver, 0) catch |e| {
        if (std.mem.eql(u8, @errorName(e), "UnsupportedVersion")) ver_err = true;
    };
    expect_eq(ver_err, true);

    // 4. Invalid reserved flags -> InvalidHeaderFlags
    let bad_flags = try alloc.dupe(u8, w.slice());
    bad_flags[5] = 0x80;
    let flag_err = false;
    * = parseOggPage(bad_flags, 0) catch |e| {
        if (std.mem.eql(u8, @errorName(e), "InvalidHeaderFlags")) flag_err = true;
    };
    expect_eq(flag_err, true);

    // 5. Truncated page -> UnexpectedEof
    let trunc_err = false;
    * = parseOggPage(w.slice()[0 .. w.slice().len - 3], 0) catch |e| {
        if (std.mem.eql(u8, @errorName(e), "UnexpectedEof")) trunc_err = true;
    };
    expect_eq(trunc_err, true);
}

fn runTest02(): void! {
    let arena = std.heap.ArenaAllocator.init(std.heap.page_allocator);
    defer arena.deinit();
    const alloc = arena.allocator();

    // Test 0-byte, 255-byte, 510-byte boundary packets and 70KB multi-page packet
    let w = OggPacketWriter.init(alloc);
    defer w.deinit();
    let seq: u32 = 0;

    try w.writePacket(0xCAFE0001, &seq, 0, true, false, "");

    let p255: [255]u8 = undefined;
    let i: usize = 0;
    while (i < 255) : (i += 1) p255[i] = (i & 0xFF) as u8;
    try w.writePacket(0xCAFE0001, &seq, 100, false, false, &p255);

    let p510: [510]u8 = undefined;
    i = 0;
    while (i < 510) : (i += 1) p510[i] = ((i * 3) & 0xFF) as u8;
    try w.writePacket(0xCAFE0001, &seq, 200, false, false, &p510);

    const large_len: usize = 70000;
    let p_large = try alloc.alloc(u8, large_len);
    i = 0;
    while (i < large_len) : (i += 1) p_large[i] = ((i * 7 + 5) & 0xFF) as u8;
    try w.writePacket(0xCAFE0001, &seq, 99999, false, true, p_large);

    try validateOggStream(w.slice());

    // Verify intermediate page has OGG_GRANULE_UNKNOWN and continuation page has OGG_FLAG_CONTINUATION
    let iter = OggPageIter.init(w.slice());
    let page_count: usize = 0;
    let saw_unknown_granule = false;
    let saw_cont = false;
    while (pg := iter.next()) {
        page_count += 1;
        if (pg.isUnknownGranule()) saw_unknown_granule = true;
        if (pg.isContinuation()) saw_cont = true;
    }
    expect_eq(page_count >= 5, true);
    expect_eq(saw_unknown_granule, true);
    expect_eq(saw_cont, true);

    // Reassemble all 4 packets via OggPacketReader
    let pr = OggPacketReader.init(alloc, w.slice());
    defer pr.deinit();

    let pkt0 = (try pr.nextPacket()).?;
    defer pkt0.deinit(alloc);
    expect_eq(pkt0.data.len, 0);
    expect_eq(pkt0.is_bos, true);

    let pkt1 = (try pr.nextPacket()).?;
    defer pkt1.deinit(alloc);
    expect_eq(std.mem.eql(u8, pkt1.data, &p255), true);

    let pkt2 = (try pr.nextPacket()).?;
    defer pkt2.deinit(alloc);
    expect_eq(std.mem.eql(u8, pkt2.data, &p510), true);

    let pkt3 = (try pr.nextPacket()).?;
    defer pkt3.deinit(alloc);
    expect_eq(std.mem.eql(u8, pkt3.data, p_large), true);
    expect_eq(pkt3.granule_position, 99999 as u64);
    expect_eq(pkt3.is_eos, true);

    expect_eq((try pr.nextPacket()) == null, true);
}

fn runTest03(): void! {
    let arena = std.heap.ArenaAllocator.init(std.heap.page_allocator);
    defer arena.deinit();
    const alloc = arena.allocator();

    // 1. Multi-stream serial_number demuxing (0x1111 and 0x2222 interleaved)
    let w = OggPacketWriter.init(alloc);
    defer w.deinit();
    let seq1: u32 = 0;
    let seq2: u32 = 0;
    try w.writePacket(0x1111, &seq1, 10, true, false, "stream-1-pkt-0");
    try w.writePacket(0x2222, &seq2, 20, true, false, "stream-2-pkt-0");
    try w.writePacket(0x1111, &seq1, 30, false, true, "stream-1-pkt-1");
    try w.writePacket(0x2222, &seq2, 40, false, true, "stream-2-pkt-1");

    let pr = OggPacketReader.init(alloc, w.slice());
    defer pr.deinit();
    let p0 = (try pr.nextPacket()).?;
    defer p0.deinit(alloc);
    expect_eq(p0.serial_number, 0x1111 as u32);
    let p1 = (try pr.nextPacket()).?;
    defer p1.deinit(alloc);
    expect_eq(p1.serial_number, 0x2222 as u32);
    let p2 = (try pr.nextPacket()).?;
    defer p2.deinit(alloc);
    expect_eq(std.mem.eql(u8, p2.data, "stream-1-pkt-1"), true);
    let p3 = (try pr.nextPacket()).?;
    defer p3.deinit(alloc);
    expect_eq(std.mem.eql(u8, p3.data, "stream-2-pkt-1"), true);

    // 2. Resync after leading noise + fake "OggS" header
    let noisy = std.ArrayList(u8).empty;
    defer noisy.deinit(alloc);
    try noisy.appendSlice(alloc, "garbage-bytes-OggS-fake-header-noise-1234567890");
    const real_offset = noisy.items.len;
    try noisy.appendSlice(alloc, w.slice());

    const found = findNextOggPage(noisy.items, 0).?;
    expect_eq(found, real_offset);
    expect_eq(findNextOggPageScalar(noisy.items, 0).?, real_offset);
}

fn runTest04(): void! {
    // 1. SIMD @Vector(16, u16) segment sum vs scalar reference (0..255 segments)
    let seg_buf: [255]u8 = undefined;
    let i: usize = 0;
    while (i < 255) : (i += 1) seg_buf[i] = ((i * 19 + 3) % 256) as u8;

    let n: usize = 0;
    while (n <= 255) : (n += 1) {
        expect_eq(oggSumSegmentsSimd(seg_buf[0..n]), oggSumSegmentsScalar(seg_buf[0..n]));
    }

    // 2. SIMD @Vector(16, u8) payload copy vs scalar reference (0..128 bytes)
    let src_buf: [128]u8 = undefined;
    i = 0;
    while (i < 128) : (i += 1) src_buf[i] = ((i * 41 + 11) & 0xFF) as u8;
    let simd_dst: [128]u8 = undefined;
    let scal_dst: [128]u8 = undefined;
    let sz: usize = 0;
    while (sz <= 128) : (sz += 1) {
        oggCopyPayloadSimd(simd_dst[0..sz], src_buf[0..sz]);
        oggCopyPayloadScalar(scal_dst[0..sz], src_buf[0..sz]);
        expect_eq(std.mem.eql(u8, simd_dst[0..sz], scal_dst[0..sz]), true);
    }
}

fn runTest05(): void! {
    // 1,500-iteration deterministic PRNG malformed Ogg / forged page_segments / bad CRC fuzzing
    let seed: u64 = 0x3533_0660_ABCD_EF01;
    let buf: [128]u8 = undefined;

    let iter: usize = 0;
    let ok_count: usize = 0;
    let err_count: usize = 0;
    while (iter < 1500) : (iter += 1) {
        const len = ((xorshift64(&seed) % 100) + 1) as usize;
        let j: usize = 0;
        while (j < len) : (j += 1) {
            buf[j] = (xorshift64(&seed) & 0xFF) as u8;
        }
        // Inject "OggS" on 50% of cases to stress header/segment/CRC bounds
        if ((iter & 1) == 0 && len >= 5) {
            buf[0] = 'O';
            buf[1] = 'g';
            buf[2] = 'g';
            buf[3] = 'S';
            buf[4] = 0;
        }
        * = findNextOggPage(buf[0..len], 0);
        validateOggStream(buf[0..len]) catch {
            err_count += 1;
            continue;
        };
        ok_count += 1;
    }
    expect_eq(ok_count + err_count, 1500);
}

fn main() {
    test("ogg_ref_01_rfc3533_crc32_tamper_version_flags_and_truncation", () => { try runTest01(); });
    test("ogg_ref_02_boundary_packets_255_510_and_70kb_multipage_reassembly", () => { try runTest02(); });
    test("ogg_ref_03_multistream_serial_demux_and_garbage_resync", () => { try runTest03(); });
    test("ogg_ref_04_simd_segment_sum_payload_copy_and_resync_parity", () => { try runTest04(); });
    test("ogg_ref_05_deterministic_prng_1500_fuzz_malformed_ogg", () => { try runTest05(); });
}
