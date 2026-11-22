// Origin: Pure-CP Stdlib Test Suite (lib/media/mpeg.cp) | Created: 2026-11-23
// Production Deep Verification Suite for Upgraded Zero-Heap MP3 / MPEG-1/2/2.5, CRC16, Xing/LAME/VBRI TOC Seek, Side-Info & RFC 2250 RTP:
//   1. MPEG-1/2/2.5 Layer I/II/III header encode/decode roundtrip + ISO/IEC 11172-3 CRC16-IBM (0x8005) stamp/verify & tamper check
//   2. Mp3StreamIter: leading ID3v2 skip + garbage resync (with false 0xFF 0xFB noise) + trailing 128B ID3v1 ignore
//   3. Xing + LAME Gapless (encoder_delay/encoder_padding) + 100-entry TOC linear seek (`seekMp3ByteOffsetByMs`) & `scanMp3Stream`
//   4. Fraunhofer VBRI header parsing (version/delay/quality/bytes/frames/toc_entries/scale/entry_bytes/frames_per_entry)
//   5. Layer III Side-Info bitstream parsing (MPEG-1 stereo & MPEG-2 mono: main_data_begin, private_bits, scfsi, part2_3_length, global_gain)
//   6. RFC 2250 MPA RTP multi-frame aggregation & oversized-frame fragmentation/depacketizer closed loop
//   7. Malformed/truncated boundary guards & cap.media namespace parity

import { test, expect_eq, expect_true } from "cap:test";
import cap from "cap";
import {
    MP3_CRC16_POLY,
    MP3_ID3V1_TAG_SIZE,
    MP3_XING_TOC_SIZE,
    MPA_RTP_HEADER_SIZE,
    MpegError,
    MpegVersion,
    MpegLayer,
    MpegChannelMode,
    parseMpegFrameHeader,
    Mp3HeaderSpec,
    encodeMp3FrameHeader,
    mp3SideInfoLen,
    verifyMp3FrameCrc,
    stampMp3FrameCrc,
    hasTrailingId3v1,
    Mp3StreamIter,
    parseMp3XingHeader,
    parseVbriHeader,
    seekMp3ByteOffsetByMs,
    scanMp3Stream,
    parseLayer3SideInfo,
    Mp3RtpSpan,
    packetizeMp3Rtp,
    Mp3RtpDepacketizer,
} from "cap:media";

fn bytesEq(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) : (i += 1) {
        if (a[i] != b[i]) return false;
    }
    return true;
}

fn writeBeU16(buf: []u8, off: usize, v: u16) {
    buf[off] = ((v >> 8) & 0xFF) as u8;
    buf[off + 1] = (v & 0xFF) as u8;
}

fn writeBeU32(buf: []u8, off: usize, v: u32) {
    buf[off] = ((v >> 24) & 0xFF) as u8;
    buf[off + 1] = ((v >> 16) & 0xFF) as u8;
    buf[off + 2] = ((v >> 8) & 0xFF) as u8;
    buf[off + 3] = (v & 0xFF) as u8;
}

test("MPEG-1/2/2.5 Layer I/II/III header encode/decode & CRC16-IBM (0x8005) stamp/verify") {
    expect_eq(MP3_CRC16_POLY, 0x8005);

    // 1. MPEG-1 Layer III 128kbps 44.1kHz stereo with CRC -> frame_size=417, samples=1152, side_info=32
    let f1: [417]u8 = [0; 417];
    * = encodeMp3FrameHeader(Mp3HeaderSpec{
        version: MpegVersion.v1,
        layer: MpegLayer.layer3,
        has_crc: true,
        bitrate_kbps: 128,
        sample_rate: 44100,
        channel_mode: MpegChannelMode.joint_stereo,
        mode_extension: 2,
    }, f1[0..4]) catch unreachable;
    f1[6] = 0xAB;
    f1[37] = 0xCD;
    const h1 = parseMpegFrameHeader(f1[0..]) orelse unreachable;
    expect_eq(h1.version, MpegVersion.v1);
    expect_eq(h1.layer, MpegLayer.layer3);
    expect_true(h1.has_crc);
    expect_eq(h1.bitrate_kbps, 128);
    expect_eq(h1.sample_rate, 44100);
    expect_eq(h1.samples_per_frame, 1152);
    expect_eq(h1.frame_size, 417);
    expect_eq(mp3SideInfoLen(h1), 32);

    const crc = stampMp3FrameCrc(f1[0..]) catch unreachable;
    expect_true(crc != 0);
    expect_true(verifyMp3FrameCrc(f1[0..]) catch unreachable);
    // Flip 1 bit in side-info -> CRC check fails!
    f1[10] ^= 0x01;
    expect_true(!(verifyMp3FrameCrc(f1[0..]) catch unreachable));

    // 2. MPEG-2 Layer III 64kbps 24kHz mono -> samples=576, side_info=9
    let hbuf: [4]u8 = [0; 4];
    * = encodeMp3FrameHeader(Mp3HeaderSpec{
        version: MpegVersion.v2,
        layer: MpegLayer.layer3,
        bitrate_kbps: 64,
        sample_rate: 24000,
        channel_mode: MpegChannelMode.mono,
    }, hbuf[0..]) catch unreachable;
    const h2 = parseMpegFrameHeader(hbuf[0..]) orelse unreachable;
    expect_eq(h2.samples_per_frame, 576);
    expect_eq(mp3SideInfoLen(h2), 9);

    // 3. MPEG-2.5 Layer I 64kbps 8kHz -> samples=384
    * = encodeMp3FrameHeader(Mp3HeaderSpec{
        version: MpegVersion.v2_5,
        layer: MpegLayer.layer1,
        bitrate_kbps: 64,
        sample_rate: 8000,
        channel_mode: MpegChannelMode.mono,
    }, hbuf[0..]) catch unreachable;
    const h3 = parseMpegFrameHeader(hbuf[0..]) orelse unreachable;
    expect_eq(h3.version, MpegVersion.v2_5);
    expect_eq(h3.layer, MpegLayer.layer1);
    expect_eq(h3.samples_per_frame, 384);
}

test("Mp3StreamIter skips ID3v2, resyncs past garbage noise & ignores trailing 128B ID3v1") {
    // 14B ID3v2 header (10B + 4B body) + 5B garbage + 2x MPEG-1 L3 128kbps 48kHz (384B each) + 128B ID3v1
    const fsize: usize = 384;
    let stream: [1024]u8 = [0; 1024];
    // ID3v2.3 tag with synchsafe size = 4
    stream[0] = 'I'; stream[1] = 'D'; stream[2] = '3';
    stream[3] = 3; stream[4] = 0; stream[5] = 0;
    stream[9] = 4;
    // 5 bytes garbage at [14..19] including false sync 0xFF 0xFB
    stream[14] = 0x11; stream[15] = 0xFF; stream[16] = 0xFB; stream[17] = 0x00; stream[18] = 0x22;

    // Frame 0 at offset 19 (384B) & Frame 1 at offset 403 (384B)
    const spec = Mp3HeaderSpec{
        version: MpegVersion.v1,
        layer: MpegLayer.layer3,
        bitrate_kbps: 128,
        sample_rate: 48000,
        channel_mode: MpegChannelMode.stereo,
    };
    * = encodeMp3FrameHeader(spec, stream[19..23]) catch unreachable;
    * = encodeMp3FrameHeader(spec, stream[(fsize + 19)..(fsize + 23)]) catch unreachable;

    // Trailing 128-byte ID3v1 tag starting at 19 + 768 = 787
    const id3v1_off: usize = fsize * 2 + 19;
    stream[id3v1_off] = 'T'; stream[id3v1_off + 1] = 'A'; stream[id3v1_off + 2] = 'G';
    const total_len: usize = id3v1_off + MP3_ID3V1_TAG_SIZE;
    expect_true(hasTrailingId3v1(stream[0..total_len]));

    let it = Mp3StreamIter.init(stream[0..total_len]);
    const fr0 = it.next() orelse unreachable;
    const fr1 = it.next() orelse unreachable;
    expect_true(it.next() == null);
    expect_eq(it.bytes_skipped, 5);
    expect_eq(it.frames_yielded, 2);
    expect_eq(fr0.offset, 19);
    expect_eq(fr1.offset, fsize + 19);
}

test("Xing + LAME Gapless delay/padding + 100-entry TOC O(1) seek & scanMp3Stream") {
    // Build a 417-byte MPEG-1 L3 128kbps 44.1kHz stereo frame containing Xing + TOC + LAME tag
    let frame: [417]u8 = [0; 417];
    * = encodeMp3FrameHeader(Mp3HeaderSpec{
        version: MpegVersion.v1,
        layer: MpegLayer.layer3,
        bitrate_kbps: 128,
        sample_rate: 44100,
        channel_mode: MpegChannelMode.stereo,
    }, frame[0..4]) catch unreachable;

    // Side-info is 32B -> Xing starts at offset 36
    frame[36] = 'X'; frame[37] = 'i'; frame[38] = 'n'; frame[39] = 'g';
    writeBeU32(frame[0..], 40, 0x000F); // frames + bytes + TOC + quality
    writeBeU32(frame[0..], 44, 1000);   // 1000 frames
    writeBeU32(frame[0..], 48, 417000); // 417,000 bytes
    // Linear 100-entry TOC at [52..152]: toc[i] = (i * 256) / 100
    let ti: usize = 0;
    while (ti < MP3_XING_TOC_SIZE) : (ti += 1) {
        frame[52 + ti] = ((ti * 256) / 100) as u8;
    }
    writeBeU32(frame[0..], 152, 85); // quality = 85

    // LAME tag at offset 156: "LAME3.100", delay=576 (0x240), padding=1152 (0x480)
    // Packed at +21..+23: 0x24, 0x04, 0x80
    frame[156] = 'L'; frame[157] = 'A'; frame[158] = 'M'; frame[159] = 'E';
    frame[160] = '3'; frame[161] = '.'; frame[162] = '1'; frame[163] = '0'; frame[164] = '0';
    frame[156 + 21] = 0x24;
    frame[156 + 22] = 0x04;
    frame[156 + 23] = 0x80;

    const hdr = parseMpegFrameHeader(frame[0..]) orelse unreachable;
    const xing = parseMp3XingHeader(frame[0..], hdr) catch unreachable;
    expect_true(!xing.is_info);
    expect_true(xing.has_frames);
    expect_eq(xing.frames, 1000);
    expect_eq(xing.bytes, 417000);
    expect_true(xing.has_toc);
    expect_eq(xing.quality, 85);
    expect_true(xing.has_lame);
    expect_eq(xing.encoder_delay, 576);
    expect_eq(xing.encoder_padding, 1152);

    // O(1) TOC seek at 50% of duration -> ~208,500 bytes
    const mid_off = seekMp3ByteOffsetByMs(13061, 26122, 417000, xing);
    expect_true(mid_off >= 205000 && mid_off <= 212000);

    // Full scanMp3Stream accounts for LAME gapless sample trim
    const summary = scanMp3Stream(frame[0..]) catch unreachable;
    expect_true(summary.has_xing);
    expect_true(summary.has_lame);
    expect_true(summary.is_vbr);
    expect_eq(summary.total_frames, 1000);
    expect_eq(summary.trimmed_samples, 1000 * 1152 - 576 - 1152);
}

test("Fraunhofer VBRI header parsing at fixed offset 36") {
    let frame: [128]u8 = [0; 128];
    frame[36] = 'V'; frame[37] = 'B'; frame[38] = 'R'; frame[39] = 'I';
    writeBeU16(frame[0..], 40, 1);       // version
    writeBeU16(frame[0..], 42, 1105);    // delay
    writeBeU16(frame[0..], 44, 75);      // quality
    writeBeU32(frame[0..], 46, 2500000); // bytes
    writeBeU32(frame[0..], 50, 4800);    // frames
    writeBeU16(frame[0..], 54, 200);     // toc_entries
    writeBeU16(frame[0..], 56, 1);       // toc_scale
    writeBeU16(frame[0..], 58, 2);       // toc_entry_bytes
    writeBeU16(frame[0..], 60, 24);      // toc_frames_per_entry

    const vbri = parseVbriHeader(frame[0..]) catch unreachable;
    expect_eq(vbri.version, 1);
    expect_eq(vbri.delay, 1105);
    expect_eq(vbri.quality, 75);
    expect_eq(vbri.bytes, 2500000);
    expect_eq(vbri.frames, 4800);
    expect_eq(vbri.toc_entries, 200);
    expect_eq(vbri.toc_scale, 1);
    expect_eq(vbri.toc_entry_bytes, 2);
    expect_eq(vbri.toc_frames_per_entry, 24);
}

test("Layer III Side-Info bitstream parsing (MPEG-1 stereo & MPEG-2 mono)") {
    // MPEG-1 Layer III Stereo (no CRC -> side-info at [4..36], 32 bytes):
    //   main_data_begin (9b) = 0x105 (261): 1_0000_0101
    //   private_bits (3b)    = 0b101 (5)
    //   scfsi[0] (4b)        = 0xA (1010)
    //   scfsi[1] (4b)        = 0x5 (0101)
    //   gr0 ch0: part2_3_length (12b) = 0x321 (801), big_values (9b) = 0x0AA (170), global_gain (8b) = 0x99 (153)
    // Pack bits:
    //   byte 4: 1000_0010 = 0x82
    //   byte 5: 1_101_1010 = 0xDA
    //   byte 6: 0101_0011 = 0x53 (scfsi[1]=0101, top 4b of 0x321=0011)
    //   byte 7: 0010_0001 = 0x21 (low 8b of 0x321)
    //   byte 8: 0101_0101 = 0x55 (top 8b of 9-bit 0x0AA = 010101010)
    //   byte 9: 0_1001100 = 0x4C (low 1b of 0x0AA=0, top 7b of 0x99=1001100)
    //   byte 10: 1_0000000 = 0x80 (low 1b of 0x99=1)
    let frame: [64]u8 = [0; 64];
    * = encodeMp3FrameHeader(Mp3HeaderSpec{
        version: MpegVersion.v1,
        layer: MpegLayer.layer3,
        bitrate_kbps: 128,
        sample_rate: 44100,
        channel_mode: MpegChannelMode.stereo,
    }, frame[0..4]) catch unreachable;
    frame[4] = 0x82;
    frame[5] = 0xDA;
    frame[6] = 0x53;
    frame[7] = 0x21;
    frame[8] = 0x55;
    frame[9] = 0x4C;
    frame[10] = 0x80;

    const hdr = parseMpegFrameHeader(frame[0..]) orelse unreachable;
    const si = parseLayer3SideInfo(frame[0..], hdr) catch unreachable;
    expect_eq(si.num_granules, 2);
    expect_eq(si.num_channels, 2);
    expect_eq(si.main_data_begin, 261);
    expect_eq(si.private_bits, 5);
    expect_eq(si.scfsi[0], 0x0A);
    expect_eq(si.scfsi[1], 0x05);
    expect_eq(si.granules[0].part2_3_length, 801);
    expect_eq(si.granules[0].big_values, 170);
    expect_eq(si.granules[0].global_gain, 0x99);
}

test("RFC 2250 MPA RTP multi-frame aggregation & oversized-frame fragmentation closed loop") {
    // 2 MPEG-1 L3 128kbps 48kHz frames (384 bytes each = 768 bytes total)
    let stream: [768]u8 = [0; 768];
    const spec = Mp3HeaderSpec{
        version: MpegVersion.v1,
        layer: MpegLayer.layer3,
        bitrate_kbps: 128,
        sample_rate: 48000,
        channel_mode: MpegChannelMode.stereo,
    };
    * = encodeMp3FrameHeader(spec, stream[0..4]) catch unreachable;
    stream[4] = 0xA1; stream[383] = 0xA2;
    * = encodeMp3FrameHeader(spec, stream[384..388]) catch unreachable;
    stream[388] = 0xB1; stream[767] = 0xB2;

    // 1. Aggregation (MTU=1200 -> 1 RTP packet holding both frames, frag_offset=0)
    let rtp_slab: [1200]u8 = [0; 1200];
    let spans: [8]Mp3RtpSpan = [Mp3RtpSpan{}; 8];
    const n_agg = packetizeMp3Rtp(stream[0..], 1200, rtp_slab[0..], spans[0..]) catch unreachable;
    expect_eq(n_agg, 1);
    expect_eq(spans[0].frag_offset, 0);
    expect_eq(spans[0].len, 768 + MPA_RTP_HEADER_SIZE);

    let dep_buf: [1024]u8 = [0; 1024];
    let dep = Mp3RtpDepacketizer.init(dep_buf[0..]) catch unreachable;
    expect_true(dep.pushRtpPacket(spans[0].slice(rtp_slab[0..])) catch unreachable);
    expect_true(bytesEq(dep.bytes(), stream[0..]));

    // 2. Oversized-frame fragmentation (1 frame of 384B with MTU=204 -> 2 fragments: 200B + 184B)
    const n_frag = packetizeMp3Rtp(stream[0..384], 204, rtp_slab[0..], spans[0..]) catch unreachable;
    expect_eq(n_frag, 2);
    expect_eq(spans[0].frag_offset, 0);
    expect_eq(spans[1].frag_offset, 200);

    expect_true(!(dep.pushRtpPacket(spans[0].slice(rtp_slab[0..])) catch unreachable));
    expect_true(dep.pushRtpPacket(spans[1].slice(rtp_slab[0..])) catch unreachable);
    expect_true(bytesEq(dep.bytes(), stream[0..384]));
}

test("MPEG/MP3 malformed boundary guards & cap.media namespace parity") {
    let tmp: [8]u8 = [0; 8];
    // 1. InvalidBitrate in encodeMp3FrameHeader
    let got_bad_br: bool = false;
    * = encodeMp3FrameHeader(Mp3HeaderSpec{ bitrate_kbps: 777 }, tmp[0..]) catch |e| {
        if (e == MpegError.InvalidBitrate) got_bad_br = true;
    };
    expect_true(got_bad_br);

    // 2. Non-zero MBZ in RFC 2250 MPA RTP header -> InvalidRtpPacket
    let dep_buf: [64]u8 = [0; 64];
    let dep = Mp3RtpDepacketizer.init(dep_buf[0..]) catch unreachable;
    const bad_mpa: [8]u8 = [0x00, 0x01, 0x00, 0x00, 0xFF, 0xFB, 0x90, 0x00];
    let got_bad_mpa: bool = false;
    * = dep.pushRtpPacket(bad_mpa[0..]) catch |e| {
        if (e == MpegError.InvalidRtpPacket) got_bad_mpa = true;
    };
    expect_true(got_bad_mpa);

    // 3. cap.media namespace parity
    expect_eq(cap.media.MP3_CRC16_POLY, 0x8005);
    expect_eq(cap.media.MP3_ID3V1_TAG_SIZE, 128);
    expect_eq(cap.media.MPA_RTP_HEADER_SIZE, 4);
}
