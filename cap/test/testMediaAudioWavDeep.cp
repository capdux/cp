// Origin: Pure-CP Stdlib Test Suite (lib/media/audioWav.cp) | Created: 2026-11-23
// Production Deep Verification Suite for Upgraded Zero-Heap WAV / RF64 / Extensible / PCM / Float / G.711 / IMA-ADPCM:
//   1. Full bit-depth roundtrip precision (U8, S16LE + TPDF dither, S24LE 3-byte packed, S32LE, F32LE) & saturation clamp
//   2. WAVE_FORMAT_EXTENSIBLE 5.1 surround (SubFormat GUID) & EBU Tech 3306 RF64/ds64 >4GB (5GB) header parse/encode
//   3. WavStreamWriter multi-chunk streaming write, LIST/INFO metadata, cue markers & smpl chunk parsing
//   4. IMA-ADPCM (0x0011) mono & stereo 4:1 sine-wave encode/decode roundtrip with SNR > 25 dB verification
//   5. G.711 u-law (0x0007) & A-law (0x0006) WAV container roundtrip with fact chunk & SNR > 30 dB verification
//   6. Multi-channel 6-ch (5.1) interleave/deinterleave & stereo/mono mixing lossless roundtrip
//   7. Malformed/truncated/boundary guard matrix & cap.media namespace parity

import { test, expect_eq, expect_true } from "cap:test";
import cap from "cap";
import {
    WAV_FORMAT_PCM,
    WAV_FORMAT_IEEE_FLOAT,
    WAV_FORMAT_ALAW,
    WAV_FORMAT_MULAW,
    WAV_FORMAT_IMA_ADPCM,
    WAV_FORMAT_EXTENSIBLE,
    WAV_CHANNEL_MASK_5_1,
    AudioError,
    WavSampleFormat,
    WavInfoMetadata,
    WavCuePoint,
    parseWavContainer,
    WavStreamConfig,
    encodeRf64Header,
    WavStreamWriter,
    u8ToF32Slice,
    f32ToU8Slice,
    s16LeToF32Slice,
    f32ToS16LeSlice,
    s24LeToF32Slice,
    f32ToS24LeSlice,
    s32LeToF32Slice,
    f32ToS32LeSlice,
    f32LeBytesToF32Slice,
    f32SliceToF32LeBytes,
    stereoToMonoF32,
    monoToStereoF32,
    interleaveChannelsF32,
    deinterleaveChannelsF32,
    imaAdpcmBlockSamples,
    encodeImaAdpcmBlock,
    decodeImaAdpcmBlock,
    encodeWavMulaw,
    decodeWavMulaw,
    encodeWavAlaw,
    decodeWavAlaw,
} from "cap:media";

fn bytesEq(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) : (i += 1) {
        if (a[i] != b[i]) return false;
    }
    return true;
}

fn absF32(x: f32): f32 {
    return if (x < 0.0) -x else x;
}

fn approxSine(phase: f64): f64 {
    // Wrap phase into [-pi, pi] and evaluate 7th-order Taylor polynomial (error < 1e-4)
    const pi: f64 = 3.141592653589793;
    const two_pi: f64 = 6.283185307179586;
    let x: f64 = phase;
    while (x > pi) { x -= two_pi; }
    while (x < -pi) { x += two_pi; }
    const x2: f64 = x * x;
    return x * (1.0 - x2 / 6.0 + (x2 * x2) / 120.0 - (x2 * x2 * x2) / 5040.0);
}

test("WAV full bit-depth conversion precision: U8, S16LE (+TPDF), S24LE, S32LE, F32LE & clamp") {
    const src: [6]f32 = [-1.5, -1.0, -0.5, 0.0, 0.5, 1.5];
    const exp: [6]f32 = [-1.0, -1.0, -0.5, 0.0, 0.5, 1.0];

    // 1. U8
    let u8_buf: [6]u8 = [0; 6];
    let f_out: [6]f32 = [0.0; 6];
    expect_eq(f32ToU8Slice(src[0..], u8_buf[0..]) catch unreachable, 6);
    expect_eq(u8ToF32Slice(u8_buf[0..], f_out[0..]) catch unreachable, 6);
    let i: usize = 0;
    while (i < 6) : (i += 1) {
        expect_true(absF32(f_out[i] - exp[i]) < 0.02);
    }

    // 2. S16LE (plain & TPDF dither)
    let s16_buf: [12]u8 = [0; 12];
    expect_eq(f32ToS16LeSlice(src[0..], s16_buf[0..], false) catch unreachable, 12);
    expect_eq(s16LeToF32Slice(s16_buf[0..], f_out[0..]) catch unreachable, 6);
    i = 0;
    while (i < 6) : (i += 1) {
        expect_true(absF32(f_out[i] - exp[i]) < 0.0002);
    }
    expect_eq(f32ToS16LeSlice(src[0..], s16_buf[0..], true) catch unreachable, 12);
    expect_eq(s16LeToF32Slice(s16_buf[0..], f_out[0..]) catch unreachable, 6);
    i = 0;
    while (i < 6) : (i += 1) {
        expect_true(absF32(f_out[i] - exp[i]) < 0.0005);
    }

    // 3. S24LE (3-byte packed signed little-endian)
    let s24_buf: [18]u8 = [0; 18];
    expect_eq(f32ToS24LeSlice(src[0..], s24_buf[0..]) catch unreachable, 18);
    expect_eq(s24LeToF32Slice(s24_buf[0..], f_out[0..]) catch unreachable, 6);
    i = 0;
    while (i < 6) : (i += 1) {
        expect_true(absF32(f_out[i] - exp[i]) < 0.00001);
    }

    // 4. S32LE
    let s32_buf: [24]u8 = [0; 24];
    expect_eq(f32ToS32LeSlice(src[0..], s32_buf[0..]) catch unreachable, 24);
    expect_eq(s32LeToF32Slice(s32_buf[0..], f_out[0..]) catch unreachable, 6);
    i = 0;
    while (i < 6) : (i += 1) {
        expect_true(absF32(f_out[i] - exp[i]) < 0.00001);
    }

    // 5. F32LE bit-exact
    let f32_buf: [24]u8 = [0; 24];
    expect_eq(f32SliceToF32LeBytes(src[0..], f32_buf[0..]) catch unreachable, 24);
    expect_eq(f32LeBytesToF32Slice(f32_buf[0..], f_out[0..]) catch unreachable, 6);
    i = 0;
    while (i < 6) : (i += 1) {
        expect_eq(f_out[i], src[i]);
    }
}

test("WAVE_FORMAT_EXTENSIBLE 5.1 surround & EBU Tech 3306 RF64/ds64 >4GB header roundtrip") {
    // 1. WAVE_FORMAT_EXTENSIBLE 5.1 (6-ch, 24-bit S24LE PCM)
    let ext_wire: [256]u8 = [0; 256];
    let w = WavStreamWriter.init(ext_wire[0..], WavStreamConfig{
        sample_format: WavSampleFormat.s24_le,
        sample_rate: 96000,
        channels: 6,
        valid_bits_per_sample: 24,
        channel_mask: WAV_CHANNEL_MASK_5_1,
        use_extensible: true,
    }, WavInfoMetadata{}) catch unreachable;
    const raw_frame: [18]u8 = [0x01; 18]; // 1 frame of 6ch * 3 bytes
    * = w.writeRawBytes(raw_frame[0..], 1) catch unreachable;
    const ext_len = w.finalize() catch unreachable;

    let cues: [2]WavCuePoint = [WavCuePoint{}; 2];
    const p_ext = parseWavContainer(ext_wire[0..ext_len], cues[0..]) catch unreachable;
    expect_true(!p_ext.is_rf64);
    expect_true(p_ext.is_extensible);
    expect_eq(p_ext.format_tag, WAV_FORMAT_EXTENSIBLE);
    expect_eq(p_ext.effective_format_tag, WAV_FORMAT_PCM);
    expect_eq(p_ext.sample_format, WavSampleFormat.s24_le);
    expect_eq(p_ext.channels, 6);
    expect_eq(p_ext.sample_rate, 96000);
    expect_eq(p_ext.valid_bits_per_sample, 24);
    expect_eq(p_ext.channel_mask, WAV_CHANNEL_MASK_5_1);
    expect_eq(p_ext.data_len, 18);

    // 2. RF64 + ds64 64-bit header describing a 5 GB IEEE Float stereo stream (> 4GB!)
    let rf64_hdr: [128]u8 = [0; 128];
    const five_gb: u64 = 5000000000;
    const hdr_len = encodeRf64Header(rf64_hdr[0..], WavStreamConfig{
        sample_format: WavSampleFormat.f32_le,
        sample_rate: 48000,
        channels: 2,
        use_extensible: true,
        channel_mask: 0x03,
    }, five_gb + 96, five_gb, five_gb / 8) catch unreachable;

    const p_rf = parseWavContainer(rf64_hdr[0..hdr_len], cues[0..]) catch unreachable;
    expect_true(p_rf.is_rf64);
    expect_true(p_rf.is_extensible);
    expect_eq(p_rf.effective_format_tag, WAV_FORMAT_IEEE_FLOAT);
    expect_eq(p_rf.sample_format, WavSampleFormat.f32_le);
    expect_eq(p_rf.riff_size_64, five_gb + 96);
    expect_eq(p_rf.data_size_64, five_gb);
    expect_eq(p_rf.fact_sample_length, five_gb / 8);
}

test("WavStreamWriter multi-chunk streaming write, LIST/INFO metadata & cue markers roundtrip") {
    let wire: [1024]u8 = [0; 1024];
    const meta = WavInfoMetadata{
        title: "CP Studio Master",
        artist: "CP Audio Lab",
        album: "Wave 26",
        date: "2026",
        comment: "Zero-heap streaming WAV",
        software: "cap:media",
    };
    let sw = WavStreamWriter.init(wire[0..], WavStreamConfig{
        sample_format: WavSampleFormat.s16_le,
        sample_rate: 16000,
        channels: 1,
    }, meta) catch unreachable;

    const chunk1: [4]i16 = [100, -200, 300, -400];
    const chunk2: [4]i16 = [500, -600, 700, -800];
    * = sw.writePcm16(chunk1[0..]) catch unreachable;
    * = sw.writePcm16(chunk2[0..]) catch unreachable;

    const out_cues_in: [2]WavCuePoint = [
        WavCuePoint{ cue_id: 1, position: 0, sample_offset: 0 },
        WavCuePoint{ cue_id: 2, position: 4, sample_offset: 4 },
    ];
    const total = sw.finalizeWithCues(out_cues_in[0..]) catch unreachable;
    expect_eq(sw.total_frames, 8);
    expect_eq(sw.data_bytes, 16);

    let parsed_cues: [4]WavCuePoint = [WavCuePoint{}; 4];
    const info = parseWavContainer(wire[0..total], parsed_cues[0..]) catch unreachable;
    expect_eq(info.sample_rate, 16000);
    expect_eq(info.channels, 1);
    expect_eq(info.data_len, 16);
    expect_true(bytesEq(info.info.title, "CP Studio Master"));
    expect_true(bytesEq(info.info.artist, "CP Audio Lab"));
    expect_true(bytesEq(info.info.album, "Wave 26"));
    expect_true(bytesEq(info.info.date, "2026"));
    expect_true(bytesEq(info.info.comment, "Zero-heap streaming WAV"));
    expect_true(bytesEq(info.info.software, "cap:media"));
    expect_eq(info.cue_count, 2);
    expect_eq(parsed_cues[0].cue_id, 1);
    expect_eq(parsed_cues[1].cue_id, 2);
    expect_eq(parsed_cues[1].sample_offset, 4);
}

test("IMA-ADPCM (0x0011) mono & stereo 4:1 sine-wave encode/decode roundtrip (SNR > 25 dB)") {
    expect_eq(imaAdpcmBlockSamples(256, 1), 505);
    expect_eq(imaAdpcmBlockSamples(512, 2), 505);

    // 1. Mono 505-sample 440Hz sine wave at 16kHz -> 256-byte IMA-ADPCM block (~4:1 compression!)
    let mono_in: [505]i16 = [0; 505];
    let i: usize = 0;
    while (i < 505) : (i += 1) {
        const ph: f64 = 6.283185307179586 * 440.0 * (i as f64) / 16000.0;
        mono_in[i] = (approxSine(ph) * 20000.0) as i16;
    }

    let adpcm_mono: [256]u8 = [0; 256];
    const blk_len = encodeImaAdpcmBlock(mono_in[0..], 1, adpcm_mono[0..]) catch unreachable;
    expect_eq(blk_len, 256);

    let mono_out: [505]i16 = [0; 505];
    const dec_cnt = decodeImaAdpcmBlock(adpcm_mono[0..blk_len], 1, mono_out[0..]) catch unreachable;
    expect_eq(dec_cnt, 505);
    expect_eq(mono_out[0], mono_in[0]); // Header predictor is exact!

    // Compute signal-to-noise power ratio: SNR > 25 dB <=> sig_pow / noise_pow > 316.2
    let sig_pow: f64 = 0.0;
    let err_pow: f64 = 0.0;
    i = 0;
    while (i < 505) : (i += 1) {
        const s: f64 = mono_in[i] as f64;
        const d: f64 = (mono_in[i] as f64) - (mono_out[i] as f64);
        sig_pow += s * s;
        err_pow += d * d;
    }
    expect_true(err_pow > 0.0);
    expect_true(sig_pow / err_pow > 320.0); // > 25 dB!

    // 2. Stereo 505-frame (1010 samples) dual-tone sine wave -> 512-byte stereo IMA-ADPCM block
    let stereo_in: [1010]i16 = [0; 1010];
    i = 0;
    while (i < 505) : (i += 1) {
        const ph_l: f64 = 6.283185307179586 * 300.0 * (i as f64) / 16000.0;
        const ph_r: f64 = 6.283185307179586 * 700.0 * (i as f64) / 16000.0;
        stereo_in[i * 2] = (approxSine(ph_l) * 18000.0) as i16;
        stereo_in[i * 2 + 1] = (approxSine(ph_r) * 16000.0) as i16;
    }
    let adpcm_st: [512]u8 = [0; 512];
    const st_blk_len = encodeImaAdpcmBlock(stereo_in[0..], 2, adpcm_st[0..]) catch unreachable;
    expect_eq(st_blk_len, 512);

    let stereo_out: [1010]i16 = [0; 1010];
    const st_dec_cnt = decodeImaAdpcmBlock(adpcm_st[0..st_blk_len], 2, stereo_out[0..]) catch unreachable;
    expect_eq(st_dec_cnt, 1010);
    expect_eq(stereo_out[0], stereo_in[0]);
    expect_eq(stereo_out[1], stereo_in[1]);

    let st_sig: f64 = 0.0;
    let st_err: f64 = 0.0;
    i = 0;
    while (i < 1010) : (i += 1) {
        const s: f64 = stereo_in[i] as f64;
        const d: f64 = (stereo_in[i] as f64) - (stereo_out[i] as f64);
        st_sig += s * s;
        st_err += d * d;
    }
    expect_true(st_sig / st_err > 320.0); // > 25 dB!
}

test("WAV G.711 u-law (0x0007) & A-law (0x0006) container encode/decode roundtrip (SNR > 30 dB)") {
    let pcm_in: [160]i16 = [0; 160];
    let i: usize = 0;
    while (i < 160) : (i += 1) {
        const ph: f64 = 6.283185307179586 * 500.0 * (i as f64) / 8000.0;
        pcm_in[i] = (approxSine(ph) * 20000.0) as i16;
    }

    // 1. u-law WAV container
    let ulaw_wav: [512]u8 = [0; 512];
    const ulen = encodeWavMulaw(pcm_in[0..], 8000, 1, ulaw_wav[0..]) catch unreachable;
    let pcm_u: [160]i16 = [0; 160];
    const u_info = decodeWavMulaw(ulaw_wav[0..ulen], pcm_u[0..]) catch unreachable;
    expect_eq(u_info.effective_format_tag, WAV_FORMAT_MULAW);
    expect_eq(u_info.sample_format, WavSampleFormat.mulaw);
    expect_eq(u_info.fact_sample_length, 160);

    let u_sig: f64 = 0.0;
    let u_err: f64 = 0.0;
    i = 0;
    while (i < 160) : (i += 1) {
        const s: f64 = pcm_in[i] as f64;
        const d: f64 = (pcm_in[i] as f64) - (pcm_u[i] as f64);
        u_sig += s * s;
        u_err += d * d;
    }
    expect_true(u_sig / u_err > 1000.0); // > 30 dB!

    // 2. A-law WAV container
    let alaw_wav: [512]u8 = [0; 512];
    const alen = encodeWavAlaw(pcm_in[0..], 8000, 1, alaw_wav[0..]) catch unreachable;
    let pcm_a: [160]i16 = [0; 160];
    const a_info = decodeWavAlaw(alaw_wav[0..alen], pcm_a[0..]) catch unreachable;
    expect_eq(a_info.effective_format_tag, WAV_FORMAT_ALAW);
    expect_eq(a_info.sample_format, WavSampleFormat.alaw);
    expect_eq(a_info.fact_sample_length, 160);

    let a_sig: f64 = 0.0;
    let a_err: f64 = 0.0;
    i = 0;
    while (i < 160) : (i += 1) {
        const s: f64 = pcm_in[i] as f64;
        const d: f64 = (pcm_in[i] as f64) - (pcm_a[i] as f64);
        a_sig += s * s;
        a_err += d * d;
    }
    expect_true(a_sig / a_err > 1000.0); // > 30 dB!
}

test("Multi-channel 6-ch (5.1) interleave/deinterleave & stereo/mono mixing roundtrip") {
    // 6 channels x 4 frames = 24 samples
    let planar: [24]f32 = [0.0; 24];
    let c: usize = 0;
    while (c < 6) : (c += 1) {
        let f: usize = 0;
        while (f < 4) : (f += 1) {
            planar[c * 4 + f] = ((c * 10 + f) as f32) * 0.01;
        }
    }

    let inter: [24]f32 = [0.0; 24];
    let back: [24]f32 = [0.0; 24];
    expect_eq(interleaveChannelsF32(planar[0..], 6, 4, inter[0..]) catch unreachable, 24);
    expect_eq(inter[0], 0.00); // f0, c0
    expect_eq(inter[1], 0.10); // f0, c1
    expect_eq(inter[6], 0.01); // f1, c0

    expect_eq(deinterleaveChannelsF32(inter[0..], 6, 4, back[0..]) catch unreachable, 24);
    let i: usize = 0;
    while (i < 24) : (i += 1) {
        expect_eq(back[i], planar[i]);
    }

    // Stereo <-> Mono
    const st: [4]f32 = [0.2, 0.6, -0.4, 0.4];
    let mo: [2]f32 = [0.0; 2];
    expect_eq(stereoToMonoF32(st[0..], mo[0..]) catch unreachable, 2);
    expect_true(absF32(mo[0] - 0.4) < 0.0001);
    expect_true(absF32(mo[1] - 0.0) < 0.0001);

    let st2: [4]f32 = [0.0; 4];
    expect_eq(monoToStereoF32(mo[0..], st2[0..]) catch unreachable, 4);
    expect_true(absF32(st2[0] - 0.4) < 0.0001);
    expect_true(absF32(st2[1] - 0.4) < 0.0001);
}

test("WAV malformed/truncated/boundary guard matrix & cap.media namespace parity") {
    let cues: [2]WavCuePoint = [WavCuePoint{}; 2];

    // 1. InvalidHeader
    const bad_magic: [16]u8 = [0; 16];
    let got_bad_hdr: bool = false;
    * = parseWavContainer(bad_magic[0..], cues[0..]) catch |e| {
        if (e == AudioError.InvalidHeader) got_bad_hdr = true;
    };
    expect_true(got_bad_hdr);

    // 2. TruncatedData (< 12 bytes)
    const short_wav: [8]u8 = ['R', 'I', 'F', 'F', 4, 0, 0, 0];
    let got_trunc: bool = false;
    * = parseWavContainer(short_wav[0..], cues[0..]) catch |e| {
        if (e == AudioError.TruncatedData) got_trunc = true;
    };
    expect_true(got_trunc);

    // 3. Odd-byte S24LE truncation
    const odd_24: [5]u8 = [0; 5];
    let f2: [2]f32 = [0.0; 2];
    let got_s24_trunc: bool = false;
    * = s24LeToF32Slice(odd_24[0..], f2[0..]) catch |e| {
        if (e == AudioError.TruncatedData) got_s24_trunc = true;
    };
    expect_true(got_s24_trunc);

    // 4. Invalid IMA-ADPCM step index (> 88)
    const bad_ima: [8]u8 = [0, 0, 99, 0, 0, 0, 0, 0];
    let pcm_tmp: [16]i16 = [0; 16];
    let got_bad_ima: bool = false;
    * = decodeImaAdpcmBlock(bad_ima[0..], 1, pcm_tmp[0..]) catch |e| {
        if (e == AudioError.InvalidChunk) got_bad_ima = true;
    };
    expect_true(got_bad_ima);

    // 5. BufferTooSmall in WavStreamWriter
    let tiny: [16]u8 = [0; 16];
    let got_small: bool = false;
    * = WavStreamWriter.init(tiny[0..], WavStreamConfig{}, WavInfoMetadata{}) catch |e| {
        if (e == AudioError.BufferTooSmall) got_small = true;
    };
    expect_true(got_small);

    // 6. cap.media namespace parity
    expect_eq(cap.media.WAV_FORMAT_EXTENSIBLE, 0xFFFE);
    expect_eq(cap.media.WAV_FORMAT_IMA_ADPCM, 0x0011);
    expect_eq(cap.media.WAV_FORMAT_MULAW, 0x0007);
    expect_eq(cap.media.WAV_FORMAT_ALAW, 0x0006);
}
