// Source: cp/cap/test/test_mpeg_production_deep.cp (Production deep tests for cap:mpeg)

import { test, expect_eq } from "cap:test";
import {
    MpegVersion,
    MpegLayer,
    MpegChannelMode,
    MpegFrameHeader,
    MpegFrame,
    MpegFrameIter,
    XingHeader,
    MpegSummary,
    skipId3v2,
    parseMpegFrameHeader,
    isMpegAudio,
    parseXingHeader,
    summarizeMpeg,
} from "cap:mpeg";

fn writeFrameHeader(
    buf: *[]u8,
    off: usize,
    ver: u8,
    layer: u8,
    no_crc: u8,
    bitrate_idx: u8,
    sr_idx: u8,
    pad: u8,
    ch_mode: u8,
) {
    buf[off] = 0xFF;
    buf[off + 1] = ((ver & 0x03) << 3) | ((layer & 0x03) << 1) | (no_crc & 0x01) | 0xE0;
    buf[off + 2] = ((bitrate_idx & 0x0F) << 4) | ((sr_idx & 0x03) << 2) | ((pad & 0x01) << 1);
    buf[off + 3] = (ch_mode & 0x03) << 6;
}

fn putU32BE(buf: *[]u8, off: usize, v: u32) {
    buf[off] = ((v >> 24) & 0xFF) as u8;
    buf[off + 1] = ((v >> 16) & 0xFF) as u8;
    buf[off + 2] = ((v >> 8) & 0xFF) as u8;
    buf[off + 3] = (v & 0xFF) as u8;
}

test("1. MPEG-1 Layer III 128kbps and 320kbps 44.1kHz CBR frame sizing and multi-frame iteration", () => {
    // 128 kbps (idx=9), 44.1 kHz (sr_idx=0), no padding => frame_size = 417
    // 320 kbps (idx=14), 44.1 kHz (sr_idx=0), padding=1 => frame_size = 1045
    // Total 2 frames = 417 + 1045 = 1462 bytes
    let stream: [1462]u8 = [0; 1462];
    let s = stream[0..1462];

    writeFrameHeader(*s, 0, 3, 1, 1, 9, 0, 0, 0); // MPEG-1 L3 128kbps 44.1kHz stereo
    writeFrameHeader(*s, 417, 3, 1, 1, 14, 0, 1, 1); // MPEG-1 L3 320kbps 44.1kHz joint_stereo pad=1

    expect_eq(isMpegAudio(s), true);

    let it = MpegFrameIter.init(s);

    const f0 = it.next() ?? unreachable;
    expect_eq(f0.offset, 0);
    expect_eq(f0.header.version == MpegVersion.v1, true);
    expect_eq(f0.header.layer == MpegLayer.layer3, true);
    expect_eq(f0.header.bitrate_kbps, 128);
    expect_eq(f0.header.sample_rate, 44100);
    expect_eq(f0.header.samples_per_frame, 1152);
    expect_eq(f0.header.frame_size, 417);
    expect_eq(f0.header.channels(), 2);
    expect_eq(f0.data.len, 417);

    const f1 = it.next() ?? unreachable;
    expect_eq(f1.offset, 417);
    expect_eq(f1.header.bitrate_kbps, 320);
    expect_eq(f1.header.padding, true);
    expect_eq(f1.header.channel_mode == MpegChannelMode.joint_stereo, true);
    expect_eq(f1.header.frame_size, 1045);
    expect_eq(f1.data.len, 1045);

    expect_eq(it.next() == null, true);

    // summarizeMpeg detects VBR across differing frame bitrates (avg = (128+320)/2 = 224)
    const summary = summarizeMpeg(s) ?? unreachable;
    expect_eq(summary.total_frames, 2);
    expect_eq(summary.avg_bitrate_kbps, 224);
    expect_eq(summary.is_vbr, true);
    expect_eq(summary.has_xing, false);
    // 2304 * 1000 / 44100 = 52 ms
    expect_eq(summary.duration_ms, 52);
});

test("2. MPEG-2 and MPEG-2.5 Layer III 576-sample frames and Layer I 384-sample frames", () => {
    let hdr_buf: [4]u8 = [0; 4];
    let hb = hdr_buf[0..4];

    // MPEG-2 (ver=2), Layer III (layer=1), 64kbps (idx=8), 22050Hz (sr_idx=0), mono (ch=3)
    writeFrameHeader(*hb, 0, 2, 1, 1, 8, 0, 0, 3);
    const h_v2 = parseMpegFrameHeader(hb) ?? unreachable;
    expect_eq(h_v2.version == MpegVersion.v2, true);
    expect_eq(h_v2.layer == MpegLayer.layer3, true);
    expect_eq(h_v2.bitrate_kbps, 64);
    expect_eq(h_v2.sample_rate, 22050);
    expect_eq(h_v2.samples_per_frame, 576);
    expect_eq(h_v2.frame_size, 208);
    expect_eq(h_v2.channels(), 1);

    // MPEG-2.5 (ver=0), Layer III (layer=1), 32kbps (idx=4), 11025Hz (sr_idx=0), pad=1
    writeFrameHeader(*hb, 0, 0, 1, 1, 4, 0, 1, 0);
    const h_v25 = parseMpegFrameHeader(hb) ?? unreachable;
    expect_eq(h_v25.version == MpegVersion.v2_5, true);
    expect_eq(h_v25.bitrate_kbps, 32);
    expect_eq(h_v25.sample_rate, 11025);
    expect_eq(h_v25.samples_per_frame, 576);
    expect_eq(h_v25.frame_size, 209);

    // MPEG-1 (ver=3), Layer I (layer=3), 128kbps (idx=4), 48000Hz (sr_idx=1), pad=0
    // frame_size = (12 * 128000 / 48000 + 0) * 4 = 32 * 4 = 128
    writeFrameHeader(*hb, 0, 3, 3, 0, 4, 1, 0, 0);
    const h_l1 = parseMpegFrameHeader(hb) ?? unreachable;
    expect_eq(h_l1.layer == MpegLayer.layer1, true);
    expect_eq(h_l1.has_crc, true);
    expect_eq(h_l1.samples_per_frame, 384);
    expect_eq(h_l1.frame_size, 128);
});

test("3. ID3v2 header with synchsafe size is automatically skipped by isMpegAudio and MpegFrameIter", () => {
    // ID3v2 header (10B) + 20B body = 30B prefix, followed by two 417B MPEG-1 L3 128kbps frames
    let buf: [864]u8 = [0; 864];
    let b = buf[0..864];

    b[0] = 'I';
    b[1] = 'D';
    b[2] = '3';
    b[3] = 3; // ID3v2.3
    b[4] = 0;
    b[5] = 0; // no footer
    b[6] = 0;
    b[7] = 0;
    b[8] = 0;
    b[9] = 20; // synchsafe size = 20

    writeFrameHeader(*b, 30, 3, 1, 1, 9, 0, 0, 0);
    writeFrameHeader(*b, 30 + 417, 3, 1, 1, 9, 0, 0, 0);

    expect_eq(skipId3v2(b), 30);
    expect_eq(isMpegAudio(b), true);

    let it = MpegFrameIter.init(b);
    const f0 = it.next() ?? unreachable;
    expect_eq(f0.offset, 30);
    const f1 = it.next() ?? unreachable;
    expect_eq(f1.offset, 447);
    expect_eq(it.next() == null, true);

    const summary = summarizeMpeg(b) ?? unreachable;
    expect_eq(summary.total_frames, 2);
    expect_eq(summary.avg_bitrate_kbps, 128);
    expect_eq(summary.is_vbr, false);
});

test("4. parseXingHeader and summarizeMpeg decode Xing VBR and Info CBR headers accurately", () => {
    // MPEG-1 Layer III 128kbps 48000Hz stereo => frame_size = 144 * 128000 / 48000 = 384 bytes
    // Side-Info offset for MPEG-1 stereo = 36
    let frame: [384]u8 = [0; 384];
    let f = frame[0..384];
    writeFrameHeader(*f, 0, 3, 1, 1, 9, 1, 0, 0);

    // Write "Xing" at offset 36 with flags = frames(1) | bytes(2) | quality(8) = 0x0B
    f[36] = 'X';
    f[37] = 'i';
    f[38] = 'n';
    f[39] = 'g';
    putU32BE(*f, 40, 0x0000000B);
    putU32BE(*f, 44, 5000); // 5000 frames => 5000 * 1152 / 48000 = 120s = 120000 ms
    putU32BE(*f, 48, 2880000); // 2,880,000 bytes => 2880000 * 8 / 120000 = 192 kbps
    putU32BE(*f, 52, 75); // quality = 75

    const hdr = parseMpegFrameHeader(f) ?? unreachable;
    const xing = parseXingHeader(f, hdr) ?? unreachable;
    expect_eq(xing.is_cbr, false);
    expect_eq(xing.is_vbri, false);
    expect_eq(xing.frames ?? 0, 5000);
    expect_eq(xing.bytes ?? 0, 2880000);
    expect_eq(xing.quality ?? 0, 75);

    const sum_vbr = summarizeMpeg(f) ?? unreachable;
    expect_eq(sum_vbr.has_xing, true);
    expect_eq(sum_vbr.is_vbr, true);
    expect_eq(sum_vbr.total_frames, 5000);
    expect_eq(sum_vbr.duration_ms, 120000);
    expect_eq(sum_vbr.avg_bitrate_kbps, 192);

    // Switch tag to "Info" => CBR indicator
    f[36] = 'I';
    f[37] = 'n';
    f[38] = 'f';
    f[39] = 'o';
    const info = parseXingHeader(f, hdr) ?? unreachable;
    expect_eq(info.is_cbr, true);
    const sum_cbr = summarizeMpeg(f) ?? unreachable;
    expect_eq(sum_cbr.is_vbr, false);

    // MPEG-2 mono (Side-Info offset = 13)
    let f2_buf: [208]u8 = [0; 208];
    let f2 = f2_buf[0..208];
    writeFrameHeader(*f2, 0, 2, 1, 1, 8, 0, 0, 3);
    f2[13] = 'X';
    f2[14] = 'i';
    f2[15] = 'n';
    f2[16] = 'g';
    putU32BE(*f2, 17, 0x00000001); // frames only
    putU32BE(*f2, 21, 1000);
    const hdr2 = parseMpegFrameHeader(f2) ?? unreachable;
    const xing2 = parseXingHeader(f2, hdr2) ?? unreachable;
    expect_eq(xing2.frames ?? 0, 1000);
    expect_eq(xing2.bytes == null, true);
});

test("5. malformed sync, reserved version/layer, invalid bitrate/sample_rate, and truncated streams fail safely", () => {
    const empty: [0]u8 = [];
    expect_eq(parseMpegFrameHeader(empty[0..0]) == null, true);
    expect_eq(isMpegAudio(empty[0..0]), false);
    expect_eq(summarizeMpeg(empty[0..0]) == null, true);

    let b: [4]u8 = [0; 4];
    let s = b[0..4];

    // Bad sync word
    writeFrameHeader(*s, 0, 3, 1, 1, 9, 0, 0, 0);
    s[0] = 0xFE;
    expect_eq(parseMpegFrameHeader(s) == null, true);

    // Reserved version (ver=1)
    writeFrameHeader(*s, 0, 1, 1, 1, 9, 0, 0, 0);
    expect_eq(parseMpegFrameHeader(s) == null, true);

    // Reserved layer (layer=0)
    writeFrameHeader(*s, 0, 3, 0, 1, 9, 0, 0, 0);
    expect_eq(parseMpegFrameHeader(s) == null, true);

    // Free bitrate (idx=0) and bad bitrate (idx=15)
    writeFrameHeader(*s, 0, 3, 1, 1, 0, 0, 0, 0);
    expect_eq(parseMpegFrameHeader(s) == null, true);
    writeFrameHeader(*s, 0, 3, 1, 1, 15, 0, 0, 0);
    expect_eq(parseMpegFrameHeader(s) == null, true);

    // Reserved sample rate (sr_idx=3)
    writeFrameHeader(*s, 0, 3, 1, 1, 9, 3, 0, 0);
    expect_eq(parseMpegFrameHeader(s) == null, true);

    // Valid first frame followed by corrupted second frame sync => isMpegAudio returns false
    let broken_pair: [430]u8 = [0; 430];
    let bp = broken_pair[0..430];
    writeFrameHeader(*bp, 0, 3, 1, 1, 9, 0, 0, 0); // 417 bytes
    bp[417] = 0x00; // invalid sync for second frame
    expect_eq(isMpegAudio(bp), false);

    // Truncated single frame (header says 417 bytes, only 100 bytes available)
    expect_eq(isMpegAudio(bp[0..100]), false);
    let it_trunc = MpegFrameIter.init(bp[0..100]);
    expect_eq(it_trunc.next() == null, true);
});
