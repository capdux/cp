// Source: cp/cap/test/test_opus_production_deep.cp (Production deep tests for cap:opus)

import std from "std";
import { test, expect_eq } from "cap:test";
import {
    OPUS_HEAD_MAGIC,
    OPUS_TAGS_MAGIC,
    OPUS_SAMPLE_RATE,
    OpusMode,
    OpusBandwidth,
    OpusHead,
    OpusTag,
    OpusTagsInfo,
    OpusCommentIter,
    OpusToc,
    isOpusHead,
    isOpusTags,
    parseOpusHead,
    parseOpusTags,
    parseOpusToc,
} from "cap:opus";

fn putU16LE(buf: *[]u8, off: usize, v: u16) {
    buf[off] = (v & 0xFF) as u8;
    buf[off + 1] = ((v >> 8) & 0xFF) as u8;
}

fn putU32LE(buf: *[]u8, off: usize, v: u32) {
    buf[off] = (v & 0xFF) as u8;
    buf[off + 1] = ((v >> 8) & 0xFF) as u8;
    buf[off + 2] = ((v >> 16) & 0xFF) as u8;
    buf[off + 3] = ((v >> 24) & 0xFF) as u8;
}

fn copyBytes(dst: *[]u8, off: usize, src: []const u8) {
    let i: usize = 0;
    while (i < src.len) {
        dst[off + i] = src[i];
        i += 1;
    }
}

test("1. parseOpusHead decodes mono/stereo family=0 and 5.1 surround family=1 headers", () => {
    expect_eq(std.mem.eql(u8, OPUS_HEAD_MAGIC, "OpusHead"), true);
    expect_eq(std.mem.eql(u8, OPUS_TAGS_MAGIC, "OpusTags"), true);
    expect_eq(OPUS_SAMPLE_RATE, 48000);

    // Stereo family=0 (19 bytes), negative output_gain (-512 => -2.0 dB)
    let stereo_buf: [19]u8 = [0; 19];
    let sb = stereo_buf[0..19];
    copyBytes(*sb, 0, OPUS_HEAD_MAGIC);
    sb[8] = 1; // version 1
    sb[9] = 2; // stereo
    putU16LE(*sb, 10, 312); // pre_skip
    putU32LE(*sb, 12, 44100); // input_sample_rate
    const neg_gain: i16 = -512;
    putU16LE(*sb, 16, neg_gain as u16);
    sb[18] = 0; // mapping family 0

    expect_eq(isOpusHead(sb), true);
    expect_eq(isOpusTags(sb), false);

    const h_stereo = parseOpusHead(sb) ?? unreachable;
    expect_eq(h_stereo.version, 1);
    expect_eq(h_stereo.channels, 2);
    expect_eq(h_stereo.pre_skip, 312);
    expect_eq(h_stereo.input_sample_rate, 44100);
    expect_eq(h_stereo.output_gain, -512);
    expect_eq(h_stereo.outputGainDb(), -2.0);
    expect_eq(h_stereo.preSkipUs(), 6500);
    expect_eq(h_stereo.channel_mapping_family, 0);
    expect_eq(h_stereo.stream_count, 1);
    expect_eq(h_stereo.coupled_count, 1);
    expect_eq(h_stereo.channel_mapping.len, 0);

    // Mono family=0 with minor version 3 (high 4 bits == 0, valid per RFC 7845)
    sb[8] = 0x03;
    sb[9] = 1;
    const h_mono = parseOpusHead(sb) ?? unreachable;
    expect_eq(h_mono.version, 3);
    expect_eq(h_mono.channels, 1);
    expect_eq(h_mono.stream_count, 1);
    expect_eq(h_mono.coupled_count, 0);

    // 5.1 surround family=1 (27 bytes: 19 + 2 + 6)
    let sur_buf: [27]u8 = [0; 27];
    let sr = sur_buf[0..27];
    copyBytes(*sr, 0, OPUS_HEAD_MAGIC);
    sr[8] = 1;
    sr[9] = 6; // 6 channels (5.1)
    putU16LE(*sr, 10, 3840); // 80ms pre-skip
    putU32LE(*sr, 12, 48000);
    putU16LE(*sr, 16, 256); // +1.0 dB
    sr[18] = 1; // Vorbis channel order family 1
    sr[19] = 4; // stream_count = 4
    sr[20] = 2; // coupled_count = 2 (4 + 2 = 6 decoded channels)
    // Standard 5.1 mapping: [0, 4, 1, 2, 3, 5]
    sr[21] = 0;
    sr[22] = 4;
    sr[23] = 1;
    sr[24] = 2;
    sr[25] = 3;
    sr[26] = 5;

    const h_51 = parseOpusHead(sr) ?? unreachable;
    expect_eq(h_51.channels, 6);
    expect_eq(h_51.channel_mapping_family, 1);
    expect_eq(h_51.stream_count, 4);
    expect_eq(h_51.coupled_count, 2);
    expect_eq(h_51.outputGainDb(), 1.0);
    expect_eq(h_51.channel_mapping.len, 6);
    expect_eq(h_51.channel_mapping[0], 0);
    expect_eq(h_51.channel_mapping[1], 4);
    expect_eq(h_51.channel_mapping[5], 5);
});

test("2. parseOpusTags and OpusCommentIter parse vendor and multiple zero-copy KEY=VALUE comments", () => {
    let buf: [160]u8 = [0; 160];
    let b = buf[0..160];

    const vendor = "libopus 1.4";
    const c0 = "TITLE=Symphony No. 9";
    const c1 = "ARTIST=Beethoven=Classic";
    const c2 = "PLAIN_NOTE";

    let off: usize = 0;
    copyBytes(*b, off, OPUS_TAGS_MAGIC);
    off += 8;
    putU32LE(*b, off, vendor.len as u32);
    off += 4;
    copyBytes(*b, off, vendor);
    off += vendor.len;
    putU32LE(*b, off, 3); // 3 user comments
    off += 4;

    putU32LE(*b, off, c0.len as u32);
    off += 4;
    copyBytes(*b, off, c0);
    off += c0.len;

    putU32LE(*b, off, c1.len as u32);
    off += 4;
    copyBytes(*b, off, c1);
    off += c1.len;

    putU32LE(*b, off, c2.len as u32);
    off += 4;
    copyBytes(*b, off, c2);
    off += c2.len;

    const pkt = b[0..off];
    expect_eq(isOpusTags(pkt), true);
    expect_eq(isOpusHead(pkt), false);

    const tags = parseOpusTags(pkt) ?? unreachable;
    expect_eq(std.mem.eql(u8, tags.vendor, "libopus 1.4"), true);
    expect_eq(tags.user_comment_count, 3);

    let it = tags.comments();
    const t0 = it.next() ?? unreachable;
    expect_eq(std.mem.eql(u8, t0.key, "TITLE"), true);
    expect_eq(std.mem.eql(u8, t0.value, "Symphony No. 9"), true);

    const t1 = it.next() ?? unreachable;
    expect_eq(std.mem.eql(u8, t1.key, "ARTIST"), true);
    // Splits on first '=' only
    expect_eq(std.mem.eql(u8, t1.value, "Beethoven=Classic"), true);

    const t2 = it.next() ?? unreachable;
    expect_eq(std.mem.eql(u8, t2.key, "PLAIN_NOTE"), true);
    expect_eq(t2.value.len, 0);

    expect_eq(it.next() == null, true);
});

test("3. parseOpusToc decodes all RFC 6716 SILK, Hybrid, and CELT configs (0..31)", () => {
    // Config 0: SILK NB 10ms mono c=0
    const p_silk_nb: [1]u8 = [(0 << 3) | 0x00];
    const t0 = parseOpusToc(p_silk_nb[0..1]) ?? unreachable;
    expect_eq(t0.config, 0);
    expect_eq(t0.mode == OpusMode.silk, true);
    expect_eq(t0.bandwidth == OpusBandwidth.narrowband, true);
    expect_eq(t0.frame_duration_us, 10000);
    expect_eq(t0.is_stereo, false);
    expect_eq(t0.channels(), 1);
    expect_eq(t0.samplesPerFrame(), 480);

    // Config 7: SILK MB 60ms stereo c=0
    const p_silk_mb: [1]u8 = [(7 << 3) | 0x04];
    const t7 = parseOpusToc(p_silk_mb[0..1]) ?? unreachable;
    expect_eq(t7.mode == OpusMode.silk, true);
    expect_eq(t7.bandwidth == OpusBandwidth.mediumband, true);
    expect_eq(t7.frame_duration_us, 60000);
    expect_eq(t7.is_stereo, true);
    expect_eq(t7.channels(), 2);
    expect_eq(t7.samplesPerFrame(), 2880);

    // Config 10: SILK WB 40ms mono c=0
    const p_silk_wb: [1]u8 = [(10 << 3)];
    const t10 = parseOpusToc(p_silk_wb[0..1]) ?? unreachable;
    expect_eq(t10.mode == OpusMode.silk, true);
    expect_eq(t10.bandwidth == OpusBandwidth.wideband, true);
    expect_eq(t10.frame_duration_us, 40000);

    // Config 12: Hybrid SWB 10ms
    const p_hyb_swb: [1]u8 = [(12 << 3)];
    const t12 = parseOpusToc(p_hyb_swb[0..1]) ?? unreachable;
    expect_eq(t12.mode == OpusMode.hybrid, true);
    expect_eq(t12.bandwidth == OpusBandwidth.superwideband, true);
    expect_eq(t12.frame_duration_us, 10000);

    // Config 15: Hybrid FB 20ms stereo
    const p_hyb_fb: [1]u8 = [(15 << 3) | 0x04];
    const t15 = parseOpusToc(p_hyb_fb[0..1]) ?? unreachable;
    expect_eq(t15.mode == OpusMode.hybrid, true);
    expect_eq(t15.bandwidth == OpusBandwidth.fullband, true);
    expect_eq(t15.frame_duration_us, 20000);
    expect_eq(t15.is_stereo, true);

    // Config 16: CELT NB 2.5ms
    const p_celt_nb: [1]u8 = [(16 << 3)];
    const t16 = parseOpusToc(p_celt_nb[0..1]) ?? unreachable;
    expect_eq(t16.mode == OpusMode.celt, true);
    expect_eq(t16.bandwidth == OpusBandwidth.narrowband, true);
    expect_eq(t16.frame_duration_us, 2500);
    expect_eq(t16.samplesPerFrame(), 120);

    // Config 21: CELT WB 5ms
    const p_celt_wb: [1]u8 = [(21 << 3)];
    const t21 = parseOpusToc(p_celt_wb[0..1]) ?? unreachable;
    expect_eq(t21.mode == OpusMode.celt, true);
    expect_eq(t21.bandwidth == OpusBandwidth.wideband, true);
    expect_eq(t21.frame_duration_us, 5000);
    expect_eq(t21.samplesPerFrame(), 240);

    // Config 26: CELT SWB 10ms
    const p_celt_swb: [1]u8 = [(26 << 3)];
    const t26 = parseOpusToc(p_celt_swb[0..1]) ?? unreachable;
    expect_eq(t26.mode == OpusMode.celt, true);
    expect_eq(t26.bandwidth == OpusBandwidth.superwideband, true);
    expect_eq(t26.frame_duration_us, 10000);

    // Config 31: CELT FB 20ms stereo
    const p_celt_fb: [1]u8 = [(31 << 3) | 0x04];
    const t31 = parseOpusToc(p_celt_fb[0..1]) ?? unreachable;
    expect_eq(t31.mode == OpusMode.celt, true);
    expect_eq(t31.bandwidth == OpusBandwidth.fullband, true);
    expect_eq(t31.frame_duration_us, 20000);
    expect_eq(t31.is_stereo, true);
    expect_eq(t31.samplesPerFrame(), 960);
});

test("4. parseOpusToc handles frame count codes c=0/1/2/3, VBR/padding flags, and total duration", () => {
    // config 31 (CELT FB 20ms), stereo
    // c=0 => 1 frame, 20000 us
    const c0_pkt: [1]u8 = [(31 << 3) | 0x04 | 0];
    const r0 = parseOpusToc(c0_pkt[0..1]) ?? unreachable;
    expect_eq(r0.frame_count_code, 0);
    expect_eq(r0.frame_count, 1);
    expect_eq(r0.packet_duration_us, 20000);
    expect_eq(r0.totalSamples(), 960);

    // c=1 => 2 equal-size frames (CBR), 40000 us
    const c1_pkt: [1]u8 = [(31 << 3) | 0x04 | 1];
    const r1 = parseOpusToc(c1_pkt[0..1]) ?? unreachable;
    expect_eq(r1.frame_count_code, 1);
    expect_eq(r1.frame_count, 2);
    expect_eq(r1.is_vbr, false);
    expect_eq(r1.packet_duration_us, 40000);
    expect_eq(r1.totalSamples(), 1920);

    // c=2 => 2 different-size frames (VBR), 40000 us
    const c2_pkt: [1]u8 = [(31 << 3) | 0x04 | 2];
    const r2 = parseOpusToc(c2_pkt[0..1]) ?? unreachable;
    expect_eq(r2.frame_count_code, 2);
    expect_eq(r2.frame_count, 2);
    expect_eq(r2.is_vbr, true);
    expect_eq(r2.packet_duration_us, 40000);

    // c=3 => 6 frames of 20ms = 120000 us (max legal duration), VBR=1, Padding=1
    const c3_pkt: [2]u8 = [(31 << 3) | 0x04 | 3, 0x80 | 0x40 | 6];
    const r3 = parseOpusToc(c3_pkt[0..2]) ?? unreachable;
    expect_eq(r3.frame_count_code, 3);
    expect_eq(r3.frame_count, 6);
    expect_eq(r3.is_vbr, true);
    expect_eq(r3.has_padding, true);
    expect_eq(r3.packet_duration_us, 120000);
    expect_eq(r3.totalSamples(), 5760);

    // c=3 with 48 frames of 2.5ms (config 16) = 120000 us (valid boundary)
    const c3_max_frames: [2]u8 = [(16 << 3) | 3, 48];
    const r48 = parseOpusToc(c3_max_frames[0..2]) ?? unreachable;
    expect_eq(r48.frame_count, 48);
    expect_eq(r48.is_vbr, false);
    expect_eq(r48.has_padding, false);
    expect_eq(r48.packet_duration_us, 120000);
});

test("5. malformed headers, truncated buffers, bad magic, channels=0, invalid version, and illegal TOC safely return null", () => {
    const empty: [0]u8 = [];
    expect_eq(isOpusHead(empty[0..0]), false);
    expect_eq(isOpusTags(empty[0..0]), false);
    expect_eq(parseOpusHead(empty[0..0]) == null, true);
    expect_eq(parseOpusTags(empty[0..0]) == null, true);
    expect_eq(parseOpusToc(empty[0..0]) == null, true);

    // Valid baseline OpusHead buffer for mutation tests
    let head: [27]u8 = [0; 27];
    let hb = head[0..27];
    copyBytes(*hb, 0, OPUS_HEAD_MAGIC);
    hb[8] = 1;
    hb[9] = 2;
    hb[18] = 0;

    // Truncated (< 19 bytes)
    expect_eq(parseOpusHead(hb[0..18]) == null, true);

    // Wrong magic
    hb[0] = 'X';
    expect_eq(parseOpusHead(hb[0..19]) == null, true);
    hb[0] = 'O';

    // Invalid major version (high 4 bits != 0, e.g. 0x10 and 0x21)
    hb[8] = 0x10;
    expect_eq(parseOpusHead(hb[0..19]) == null, true);
    hb[8] = 0x21;
    expect_eq(parseOpusHead(hb[0..19]) == null, true);
    hb[8] = 1;

    // channels == 0
    hb[9] = 0;
    expect_eq(parseOpusHead(hb[0..19]) == null, true);

    // family == 0 with channels > 2
    hb[9] = 3;
    hb[18] = 0;
    expect_eq(parseOpusHead(hb[0..19]) == null, true);

    // family == 1 truncated mapping table (needs 21 + 6 = 27, give 26)
    hb[9] = 6;
    hb[18] = 1;
    hb[19] = 4;
    hb[20] = 2;
    expect_eq(parseOpusHead(hb[0..26]) == null, true);

    // family == 1 invalid coupled_count > stream_count
    hb[19] = 2;
    hb[20] = 3;
    expect_eq(parseOpusHead(hb[0..27]) == null, true);

    // family == 1 invalid mapping index out of range (>= stream_count + coupled_count and != 255)
    hb[19] = 4;
    hb[20] = 2;
    hb[21] = 6; // >= 6 and != 255
    expect_eq(parseOpusHead(hb[0..27]) == null, true);

    // Truncated OpusTags (vendor_len exceeds buffer)
    let tags_bad: [16]u8 = [0; 16];
    let tb = tags_bad[0..16];
    copyBytes(*tb, 0, OPUS_TAGS_MAGIC);
    putU32LE(*tb, 8, 100); // vendor_len = 100 > remaining
    expect_eq(parseOpusTags(tb) == null, true);

    // Truncated comment inside OpusCommentIter safely stops returning null
    putU32LE(*tb, 8, 0); // vendor_len = 0
    putU32LE(*tb, 12, 1); // claims 1 comment, but 0 bytes left
    const t_trunc = parseOpusTags(tb) ?? unreachable;
    let it_trunc = t_trunc.comments();
    expect_eq(it_trunc.next() == null, true);

    // Illegal TOC packets:
    // c=3 missing second byte
    const c3_short: [1]u8 = [(31 << 3) | 3];
    expect_eq(parseOpusToc(c3_short[0..1]) == null, true);

    // c=3 frame_count == 0
    const c3_zero: [2]u8 = [(31 << 3) | 3, 0x00];
    expect_eq(parseOpusToc(c3_zero[0..2]) == null, true);

    // c=3 total duration > 120ms (7 * 20ms = 140ms)
    const c3_over: [2]u8 = [(31 << 3) | 3, 7];
    expect_eq(parseOpusToc(c3_over[0..2]) == null, true);
});
