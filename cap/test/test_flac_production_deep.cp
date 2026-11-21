// Source: cp/cap/test/test_flac_production_deep.cp (Production deep tests for cap:flac)

import std from "std";
import { test, expect_eq } from "cap:test";
import {
    FLAC_MAGIC_SIZE,
    FLAC_BLOCK_HEADER_SIZE,
    FLAC_STREAMINFO_SIZE,
    FlacBlockType,
    FlacMetadataBlock,
    FlacStreamInfo,
    isFlac,
    FlacBlockIter,
    parseStreamInfoPayload,
    parseFlacStreamInfo,
} from "cap:flac";

fn writeBlockHeader(buf: []u8, off: usize, is_last: bool, raw_type: u8, payload_len: u32) {
    let b0: u8 = raw_type & 0x7F;
    if (is_last) {
        b0 = b0 | 0x80;
    }
    buf[off] = b0;
    buf[off + 1] = ((payload_len >> 16) & 0xFF) as u8;
    buf[off + 2] = ((payload_len >> 8) & 0xFF) as u8;
    buf[off + 3] = (payload_len & 0xFF) as u8;
}

fn encodeStreamInfo(
    out: []u8,
    min_blk: u16,
    max_blk: u16,
    min_frm: u32,
    max_frm: u32,
    sample_rate: u32,
    channels: u8,
    bps: u8,
    total_samples: u64,
) {
    out[0] = ((min_blk >> 8) & 0xFF) as u8;
    out[1] = (min_blk & 0xFF) as u8;
    out[2] = ((max_blk >> 8) & 0xFF) as u8;
    out[3] = (max_blk & 0xFF) as u8;

    out[4] = ((min_frm >> 16) & 0xFF) as u8;
    out[5] = ((min_frm >> 8) & 0xFF) as u8;
    out[6] = (min_frm & 0xFF) as u8;

    out[7] = ((max_frm >> 16) & 0xFF) as u8;
    out[8] = ((max_frm >> 8) & 0xFF) as u8;
    out[9] = (max_frm & 0xFF) as u8;

    const ch_bits: u8 = (channels - 1) & 0x07;
    const bps_bits: u8 = (bps - 1) & 0x1F;

    out[10] = ((sample_rate >> 12) & 0xFF) as u8;
    out[11] = ((sample_rate >> 4) & 0xFF) as u8;
    out[12] = ((((sample_rate & 0x0F) as u8) << 4) | (ch_bits << 1) | ((bps_bits >> 4) & 0x01));
    out[13] = (((bps_bits & 0x0F) << 4) | (((total_samples >> 32) & 0x0F) as u8));
    out[14] = ((total_samples >> 24) & 0xFF) as u8;
    out[15] = ((total_samples >> 16) & 0xFF) as u8;
    out[16] = ((total_samples >> 8) & 0xFF) as u8;
    out[17] = (total_samples & 0xFF) as u8;

    let i: usize = 0;
    while (i < 16) {
        out[18 + i] = (0xA0 + i) as u8;
        i += 1;
    }
}

test("1. isFlac and parseFlacStreamInfo decode standard 44.1kHz stereo 16-bit STREAMINFO and durationMs", () => {
    let buf: [42]u8 = std.mem.zeroes([42]u8);
    buf[0] = 'f'; buf[1] = 'L'; buf[2] = 'a'; buf[3] = 'C';
    writeBlockHeader(buf[0..], 4, true, 0, FLAC_STREAMINFO_SIZE as u32);
    encodeStreamInfo(buf[8..42], 4096, 4096, 1024, 8192, 44100, 2, 16, 441000);

    expect_eq(isFlac(buf[0..]), true);
    const info = parseFlacStreamInfo(buf[0..]) ?? unreachable;
    expect_eq(info.min_block_size, 4096);
    expect_eq(info.max_block_size, 4096);
    expect_eq(info.min_frame_size, 1024);
    expect_eq(info.max_frame_size, 8192);
    expect_eq(info.sample_rate, 44100);
    expect_eq(info.channels, 2);
    expect_eq(info.bits_per_sample, 16);
    expect_eq(info.total_samples, 441000);
    expect_eq(info.md5[0], 0xA0);
    expect_eq(info.md5[15], 0xAF);
    expect_eq(info.durationMs(), 10000);
});

test("2. STREAMINFO 36-bit total_samples high bits, 96kHz 8ch 24-bit, and zero sample_rate durationMs guard", () => {
    let payload: [34]u8 = std.mem.zeroes([34]u8);
    // 0x5_0000_0000 exercises the upper 4 bits of the 36-bit total_samples field
    const big_samples: u64 = ((5 as u64) << 32) | (96000 as u64);
    encodeStreamInfo(payload[0..], 16, 65535, 0, 0x00FF_FFFF, 96000, 8, 24, big_samples);

    const info = parseStreamInfoPayload(payload[0..]) ?? unreachable;
    expect_eq(info.sample_rate, 96000);
    expect_eq(info.channels, 8);
    expect_eq(info.bits_per_sample, 24);
    expect_eq(info.total_samples, big_samples);
    expect_eq(info.max_frame_size, 0x00FF_FFFF);
    expect_eq(info.durationMs() > 223_000_000, true);

    // Zero sample_rate must return 0 without dividing by zero
    encodeStreamInfo(payload[0..], 16, 16, 0, 0, 0, 1, 8, 1000);
    const zero_sr = parseStreamInfoPayload(payload[0..]) ?? unreachable;
    expect_eq(zero_sr.sample_rate, 0);
    expect_eq(zero_sr.durationMs(), 0);
});

test("3. FlacBlockIter traverses multiple metadata blocks and stops immediately after is_last", () => {
    let buf: [96]u8 = std.mem.zeroes([96]u8);
    buf[0] = 'f'; buf[1] = 'L'; buf[2] = 'a'; buf[3] = 'C';
    let pos: usize = 4;

    // Block 0: STREAMINFO (34 bytes, is_last=false)
    writeBlockHeader(buf[0..], pos, false, 0, 34);
    pos += 4;
    encodeStreamInfo(buf[pos..(pos + 34)], 4096, 4096, 0, 0, 48000, 2, 24, 96000);
    pos += 34;

    // Block 1: VORBIS_COMMENT (4 bytes, is_last=false)
    writeBlockHeader(buf[0..], pos, false, 4, 4);
    pos += 4;
    buf[pos] = 'T'; buf[pos + 1] = 'A'; buf[pos + 2] = 'G'; buf[pos + 3] = 'S';
    pos += 4;

    // Block 2: PADDING (2 bytes, is_last=true)
    writeBlockHeader(buf[0..], pos, true, 1, 2);
    pos += 4;
    pos += 2;

    // Trailing audio frame bytes (must NOT be iterated as metadata blocks)
    buf[pos] = 0xFF; buf[pos + 1] = 0xF8;
    pos += 2;

    let it = FlacBlockIter.init(buf[0..pos]);

    const b0 = it.next() ?? unreachable;
    expect_eq(b0.is_last, false);
    expect_eq(b0.block_type == FlacBlockType.streaminfo, true);
    expect_eq(b0.payload.len, 34);

    const b1 = it.next() ?? unreachable;
    expect_eq(b1.is_last, false);
    expect_eq(b1.block_type == FlacBlockType.vorbis_comment, true);
    expect_eq(std.mem.eql(u8, b1.payload, "TAGS"), true);

    const b2 = it.next() ?? unreachable;
    expect_eq(b2.is_last, true);
    expect_eq(b2.block_type == FlacBlockType.padding, true);
    expect_eq(b2.payload.len, 2);

    // Must stop after is_last == true
    expect_eq(it.next() == null, true);
});

test("4. FlacBlockIter maps application, seektable, cuesheet, picture, unknown, and rejects forbidden type 127", () => {
    let buf: [64]u8 = std.mem.zeroes([64]u8);
    buf[0] = 'f'; buf[1] = 'L'; buf[2] = 'a'; buf[3] = 'C';
    let pos: usize = 4;

    writeBlockHeader(buf[0..], pos, false, 2, 0); pos += 4; // application
    writeBlockHeader(buf[0..], pos, false, 3, 0); pos += 4; // seektable
    writeBlockHeader(buf[0..], pos, false, 5, 0); pos += 4; // cuesheet
    writeBlockHeader(buf[0..], pos, false, 6, 0); pos += 4; // picture
    writeBlockHeader(buf[0..], pos, false, 42, 0); pos += 4; // unknown
    writeBlockHeader(buf[0..], pos, true, 127, 0); pos += 4; // forbidden type 127

    let it = FlacBlockIter.init(buf[0..pos]);
    expect_eq((it.next() ?? unreachable).block_type == FlacBlockType.application, true);
    expect_eq((it.next() ?? unreachable).block_type == FlacBlockType.seektable, true);
    expect_eq((it.next() ?? unreachable).block_type == FlacBlockType.cuesheet, true);
    expect_eq((it.next() ?? unreachable).block_type == FlacBlockType.picture, true);

    const unk = it.next() ?? unreachable;
    expect_eq(unk.block_type == FlacBlockType.unknown, true);
    expect_eq(unk.raw_type, 42);

    // Type 127 is forbidden and terminates iteration cleanly
    expect_eq(it.next() == null, true);
});

test("5. Malformed magic, wrong first block, bad STREAMINFO length, and truncated payloads fail safely", () => {
    let buf: [42]u8 = std.mem.zeroes([42]u8);
    expect_eq(isFlac(buf[0..3]), false);
    expect_eq(isFlac(buf[0..]), false);
    expect_eq(parseFlacStreamInfo(buf[0..]) == null, true);

    // Valid magic, but first block is PADDING instead of STREAMINFO
    buf[0] = 'f'; buf[1] = 'L'; buf[2] = 'a'; buf[3] = 'C';
    writeBlockHeader(buf[0..], 4, true, 1, 4);
    expect_eq(parseFlacStreamInfo(buf[0..12]) == null, true);

    // STREAMINFO with invalid payload length (33 instead of 34)
    writeBlockHeader(buf[0..], 4, true, 0, 33);
    expect_eq(parseFlacStreamInfo(buf[0..41]) == null, true);

    // Truncated payload (header claims 34 bytes, buffer only has 20 bytes after header)
    writeBlockHeader(buf[0..], 4, true, 0, 34);
    expect_eq(parseFlacStreamInfo(buf[0..28]) == null, true);
    let it = FlacBlockIter.init(buf[0..28]);
    expect_eq(it.next() == null, true);
});
