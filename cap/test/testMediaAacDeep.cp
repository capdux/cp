// Origin: Pure-CP Stdlib Test Suite (lib/media/aac.cp) | Created: 2026-11-23
// Production Deep Verification Suite for Upgraded Zero-Heap ISO/IEC 14496-3 AAC ADTS/ASC/ESDS/LOAS-LATM & RFC 3640 RTP:
//   1. ADTS 7-byte & 9-byte CRC16-IBM (0x8005) encode/verify, single-bit tamper detection & noise-resilient resync scanner
//   2. Full AudioSpecificConfig (ASC): standard 2B, 24-bit explicit sample rate, extended AOT>=32 & HE-AAC SBR+PS roundtrip
//   3. ADTS <-> ASC bidirectional conversion & MP4 `esds` FullBox (Tag 0x03/0x04/0x05/0x06) encode/decode closed loop
//   4. LOAS 11-bit syncword (0x2B7) multi-frame streaming with leading garbage skip & LATM AudioMuxElement AU extraction
//   5. RFC 3640 MPEG4-GENERIC (`AAC-hbr`) multi-AU aggregation packetizer, zero-copy depacketizer & oversized AU fragmentation
//   6. RFC 3640 SDP `a=fmtp` formatter verification
//   7. Malformed/truncated/boundary guard matrix & cap.media namespace parity

import { test, expect_eq, expect_true } from "cap:test";
import cap from "cap";
import {
    ADTS_HEADER_SIZE,
    ADTS_HEADER_CRC_SIZE,
    AAC_CRC16_IBM_POLY,
    LOAS_SYNC_WORD,
    AacError,
    AacProfile,
    aacSampleRateIndex,
    verifyAdtsFrameCrc,
    AdtsEncodeConfig,
    encodeAdtsFrame,
    encodeAdtsFrameWithCrc,
    AdtsFrameIter,
    AacAudioSpecificConfig,
    encodeAudioSpecificConfig,
    decodeAudioSpecificConfig,
    adtsToAudioSpecificConfig,
    audioSpecificConfigToAdtsHeader,
    encodeMp4EsdsBox,
    decodeMp4EsdsBox,
    encodeLoasFrame,
    LoasFrameIter,
    parseLatmStreamMuxConfigBasic,
    AacRtpAuSpan,
    AacRtpPacketSpan,
    packetizeAacRtpHbr,
    depacketizeAacRtpHbr,
    formatAacSdpFmtp,
} from "cap:media";

fn bytesEq(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) : (i += 1) {
        if (a[i] != b[i]) return false;
    }
    return true;
}

test("ADTS 7B/9B CRC16-IBM (0x8005) encode/verify, tamper guard & noise-resilient resync") {
    expect_eq(AAC_CRC16_IBM_POLY, 0x8005);
    expect_eq(aacSampleRateIndex(48000) orelse 99, 3);
    expect_eq(aacSampleRateIndex(44100) orelse 99, 4);

    // Build a stream with leading garbage (incl. false 0xFF 0xF1 sync!), followed by 2 valid ADTS frames (7B + 9B CRC)
    let stream: [128]u8 = [0; 128];
    stream[0] = 0x12;
    stream[1] = 0x34;
    stream[2] = 0xFF; // false sync noise
    stream[3] = 0xF1;
    stream[4] = 0x99;

    const cfg_lc = AdtsEncodeConfig{
        is_mpeg2: false,
        object_type: 2,
        sample_rate_hz: 48000,
        channel_config: 2,
        num_raw_data_blocks: 1,
    };
    const f1_len = encodeAdtsFrame(cfg_lc, "aac-frame-1", stream[5..]) catch unreachable;
    expect_eq(f1_len, ADTS_HEADER_SIZE + 11);

    const cfg_crc = AdtsEncodeConfig{
        is_mpeg2: true,
        object_type: 2,
        sample_rate_hz: 44100,
        channel_config: 6, // 5.1
        num_raw_data_blocks: 1,
    };
    const f2_off: usize = f1_len + 5;
    const f2_len = encodeAdtsFrameWithCrc(cfg_crc, "aac-crc-frame-2", stream[f2_off..]) catch unreachable;
    expect_eq(f2_len, ADTS_HEADER_CRC_SIZE + 15);

    // Verify embedded CRC16-IBM passes, and single-bit flip fails
    expect_true(verifyAdtsFrameCrc(stream[f2_off..(f2_off + f2_len)]) catch unreachable);
    stream[f2_off + f2_len - 1] ^= 0x01;
    expect_true(!(verifyAdtsFrameCrc(stream[f2_off..(f2_off + f2_len)]) catch unreachable));
    stream[f2_off + f2_len - 1] ^= 0x01;

    // Noise-resilient AdtsFrameIter skips the 5 garbage bytes and yields both frames
    let it = AdtsFrameIter.init(stream[0..(f2_off + f2_len)]);
    const fr1 = it.next() orelse unreachable;
    const fr2 = it.next() orelse unreachable;
    expect_true(it.next() == null);
    expect_eq(it.bytes_skipped, 5);
    expect_eq(it.frames_yielded, 2);

    expect_eq(fr1.profile, AacProfile.lc);
    expect_eq(fr1.sample_rate_hz, 48000);
    expect_eq(fr1.channels, 2);
    expect_true(fr1.protection_absent);
    expect_true(bytesEq(fr1.payload, "aac-frame-1"));

    expect_true(fr2.is_mpeg2);
    expect_true(!fr2.protection_absent);
    expect_eq(fr2.sample_rate_hz, 44100);
    expect_eq(fr2.channels, 6);
    expect_true(bytesEq(fr2.payload, "aac-crc-frame-2"));
}

test("AudioSpecificConfig (ASC): standard 2B, 24-bit explicit SR, extended AOT & HE-AAC SBR+PS") {
    let buf: [16]u8 = [0; 16];

    // 1. Standard 2-byte AAC-LC 48kHz Stereo -> canonical 0x11 0x90
    const asc_std = AacAudioSpecificConfig{
        object_type: 2,
        sample_rate_hz: 48000,
        channel_config: 2,
    };
    const n1 = encodeAudioSpecificConfig(asc_std, buf[0..]) catch unreachable;
    expect_eq(n1, 2);
    expect_eq(buf[0], 0x11);
    expect_eq(buf[1], 0x90);
    const d1 = decodeAudioSpecificConfig(buf[0..n1]) catch unreachable;
    expect_eq(d1.object_type, 2);
    expect_eq(d1.sample_rate_hz, 48000);
    expect_eq(d1.channels, 2);
    expect_true(!d1.frame_length_960);

    // 2. Explicit 24-bit sample rate (50000 Hz) + frame_length_960 = true
    const asc_exp = AacAudioSpecificConfig{
        object_type: 2,
        sample_rate_hz: 50000,
        channel_config: 2,
        frame_length_960: true,
    };
    const n2 = encodeAudioSpecificConfig(asc_exp, buf[0..]) catch unreachable;
    const d2 = decodeAudioSpecificConfig(buf[0..n2]) catch unreachable;
    expect_eq(d2.sample_rate_idx, 15);
    expect_eq(d2.sample_rate_hz, 50000);
    expect_true(d2.frame_length_960);

    // 3. Explicit HE-AAC v2 (AAC-LC 24kHz mono core + SBR 48kHz + PS)
    const asc_he = AacAudioSpecificConfig{
        object_type: 2,
        sample_rate_hz: 24000,
        channel_config: 1,
        sbr_present: true,
        ps_present: true,
        ext_sample_rate_hz: 48000,
    };
    const n3 = encodeAudioSpecificConfig(asc_he, buf[0..]) catch unreachable;
    const d3 = decodeAudioSpecificConfig(buf[0..n3]) catch unreachable;
    expect_eq(d3.object_type, 2);
    expect_eq(d3.sample_rate_hz, 24000);
    expect_eq(d3.channels, 1);
    expect_true(d3.sbr_present);
    expect_true(d3.ps_present);
    expect_eq(d3.ext_object_type, 5);
    expect_eq(d3.ext_sample_rate_hz, 48000);
}

test("ADTS <-> ASC conversion & MP4 `esds` FullBox encode/decode closed loop") {
    let adts_buf: [64]u8 = [0; 64];
    const flen = encodeAdtsFrame(AdtsEncodeConfig{
        object_type: 2,
        sample_rate_hz: 44100,
        channel_config: 2,
    }, "raw-au-bytes", adts_buf[0..]) catch unreachable;

    let it = AdtsFrameIter.init(adts_buf[0..flen]);
    const frame = it.next() orelse unreachable;
    const asc = adtsToAudioSpecificConfig(frame) catch unreachable;
    expect_eq(asc.object_type, 2);
    expect_eq(asc.sample_rate_hz, 44100);
    expect_eq(asc.channels, 2);

    // Convert ASC back to 7-byte ADTS header
    let hdr7: [7]u8 = [0; 7];
    const hlen = audioSpecificConfigToAdtsHeader(asc, 12, false, hdr7[0..]) catch unreachable;
    expect_eq(hlen, 7);
    expect_true(bytesEq(hdr7[0..7], adts_buf[0..7]));

    // Encode ASC -> MP4 `esds` FullBox -> Decode back
    let asc_bytes: [8]u8 = [0; 8];
    const asc_len = encodeAudioSpecificConfig(asc, asc_bytes[0..]) catch unreachable;
    expect_eq(asc_len, 2);
    expect_eq(asc_bytes[0], 0x12);
    expect_eq(asc_bytes[1], 0x10);

    let esds_box: [64]u8 = [0; 64];
    const esds_len = encodeMp4EsdsBox(asc_bytes[0..asc_len], 128000, 96000, 1, esds_box[0..]) catch unreachable;
    const esds = decodeMp4EsdsBox(esds_box[0..esds_len]) catch unreachable;
    expect_eq(esds.es_id, 1);
    expect_eq(esds.object_type_indication, 0x40);
    expect_eq(esds.stream_type, 5);
    expect_eq(esds.max_bitrate, 128000);
    expect_eq(esds.avg_bitrate, 96000);
    expect_true(bytesEq(esds.asc_raw, asc_bytes[0..asc_len]));
    expect_eq(esds.asc.sample_rate_hz, 44100);
    expect_eq(esds.asc.channels, 2);
}

test("LOAS 11-bit syncword (0x2B7) streaming & LATM AudioMuxElement AU extraction") {
    expect_eq(LOAS_SYNC_WORD, 0x2B7);

    // Hand-packed LATM AudioMuxElement:
    // useSameStreamMux=0, audioMuxVersion=0, allStreamsSameTimeFraming=1, subFrames=0, prog=0, layer=0
    // ASC = AAC-LC 48kHz stereo (0x11 0x90), frameLengthType=0, fullness=0xFF, other=0, crc=0, len=4, payload="LATM"
    const latm_mux: [11]u8 = [
        0x20, 0x00, 0x11, 0x90, 0x1F, 0xE0, 0x22, 0x62, 0x0A, 0xA2, 0x68,
    ];

    // Wrap in LOAS stream with 3 leading garbage bytes
    let loas_stream: [32]u8 = [0; 32];
    loas_stream[0] = 0xAA;
    loas_stream[1] = 0xBB;
    loas_stream[2] = 0xCC;
    const l_len = encodeLoasFrame(latm_mux[0..], loas_stream[3..]) catch unreachable;
    expect_eq(l_len, 14);

    let lit = LoasFrameIter.init(loas_stream[0..(l_len + 3)]);
    const lf = (lit.next() catch unreachable) orelse unreachable;
    expect_true((lit.next() catch unreachable) == null);
    expect_eq(lit.bytes_skipped, 3);
    expect_true(bytesEq(lf.mux_element, latm_mux[0..]));

    // Parse LATM StreamMuxConfig & extract bit-unaligned raw AAC Access Unit "LATM"
    let au_out: [16]u8 = [0; 16];
    const latm = parseLatmStreamMuxConfigBasic(lf.mux_element, au_out[0..]) catch unreachable;
    expect_true(!latm.use_same_stream_mux);
    expect_true(latm.all_streams_same_time_framing);
    expect_true(latm.has_asc);
    expect_eq(latm.asc.object_type, 2);
    expect_eq(latm.asc.sample_rate_hz, 48000);
    expect_eq(latm.asc.channels, 2);
    expect_eq(latm.payload_len, 4);
    expect_true(bytesEq(au_out[0..4], "LATM"));
}

test("RFC 3640 MPEG4-GENERIC (AAC-hbr) multi-AU aggregation, fragmentation & zero-copy depacketizer") {
    // 1. Aggregate 3 AAC Access Units into 1 RTP packet (MTU=1200)
    const au_slab = "AU0-helloAU1-world-123AU2-end";
    const spans: [3]AacRtpAuSpan = [
        AacRtpAuSpan{ offset: 0, len: 9 },
        AacRtpAuSpan{ offset: 9, len: 13 },
        AacRtpAuSpan{ offset: 22, len: 7 },
    ];
    let rtp_buf: [256]u8 = [0; 256];
    let pkts: [4]AacRtpPacketSpan = [AacRtpPacketSpan{}; 4];
    const npkts = packetizeAacRtpHbr(au_slab, spans[0..], 1200, rtp_buf[0..], pkts[0..]) catch unreachable;
    expect_eq(npkts, 1);

    let dec_aus: [4]AacRtpAuSpan = [AacRtpAuSpan{}; 4];
    const pkt0 = pkts[0].slice(rtp_buf[0..]);
    const nau = depacketizeAacRtpHbr(pkt0, dec_aus[0..]) catch unreachable;
    expect_eq(nau, 3);
    expect_eq(dec_aus[0].au_index, 0);
    expect_eq(dec_aus[1].au_index, 1);
    expect_eq(dec_aus[2].au_index, 2);
    expect_true(bytesEq(dec_aus[0].slice(pkt0), "AU0-hello"));
    expect_true(bytesEq(dec_aus[1].slice(pkt0), "AU1-world-123"));
    expect_true(bytesEq(dec_aus[2].slice(pkt0), "AU2-end"));

    // 2. Oversized AU fragmentation (30-byte AU with MTU=14 -> 3 packets of 10B payload each)
    const big_au = "0123456789ABCDEFGHIJabcdefghij";
    const big_span: [1]AacRtpAuSpan = [AacRtpAuSpan{ offset: 0, len: 30 }];
    const nfrag = packetizeAacRtpHbr(big_au, big_span[0..], 14, rtp_buf[0..], pkts[0..]) catch unreachable;
    expect_eq(nfrag, 3);
    const fp0 = pkts[0].slice(rtp_buf[0..]);
    const fp2 = pkts[2].slice(rtp_buf[0..]);
    expect_eq(depacketizeAacRtpHbr(fp0, dec_aus[0..]) catch unreachable, 1);
    expect_true(bytesEq(dec_aus[0].slice(fp0), "0123456789"));
    expect_eq(depacketizeAacRtpHbr(fp2, dec_aus[0..]) catch unreachable, 1);
    expect_true(bytesEq(dec_aus[0].slice(fp2), "abcdefghij"));
}

test("RFC 3640 SDP `a=fmtp` formatter produces standard AAC-hbr attribute line") {
    const asc: [2]u8 = [0x11, 0x90];
    let sdp: [160]u8 = [0; 160];
    const n = formatAacSdpFmtp(97, asc[0..], sdp[0..]) catch unreachable;
    expect_true(bytesEq(
        sdp[0..n],
        "a=fmtp:97 streamtype=5;profile-level-id=1;mode=AAC-hbr;sizelength=13;indexlength=3;indexdeltalength=3;config=1190",
    ));
}

test("AAC malformed/truncated boundary guards & cap.media namespace parity") {
    // 1. InvalidSampleRate in encodeAdtsFrame
    let tmp: [32]u8 = [0; 32];
    let got_bad_sr: bool = false;
    * = encodeAdtsFrame(AdtsEncodeConfig{ sample_rate_hz: 12345 }, "x", tmp[0..]) catch |e| {
        if (e == AacError.InvalidSampleRate) got_bad_sr = true;
    };
    expect_true(got_bad_sr);

    // 2. Truncated RFC 3640 AAC-hbr RTP payload (AU-size exceeds packet length)
    const bad_rtp: [6]u8 = [0x00, 0x10, 0x05, 0x00, 0xAA, 0xBB]; // declares 160B AU, only 2B follow
    let aus: [2]AacRtpAuSpan = [AacRtpAuSpan{}; 2];
    let got_rtp_trunc: bool = false;
    * = depacketizeAacRtpHbr(bad_rtp[0..], aus[0..]) catch |e| {
        if (e == AacError.TruncatedData) got_rtp_trunc = true;
    };
    expect_true(got_rtp_trunc);

    // 3. InvalidEsds
    const bad_esds: [20]u8 = [0; 20];
    let got_bad_esds: bool = false;
    * = decodeMp4EsdsBox(bad_esds[0..]) catch |e| {
        if (e == AacError.InvalidEsds) got_bad_esds = true;
    };
    expect_true(got_bad_esds);

    // 4. cap.media namespace parity
    expect_eq(cap.media.AAC_CRC16_IBM_POLY, 0x8005);
    expect_eq(cap.media.LOAS_SYNC_WORD, 0x2B7);
    expect_eq(cap.media.ESDS_TAG_DECODER_SPECIFIC, 0x05);
}
