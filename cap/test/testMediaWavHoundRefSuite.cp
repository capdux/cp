// Origin: Pure-CP Stdlib Test Suite (lib/media/audioWav.cp) | Created: 2026-11-23
// Reference-hardened against ruuda/hound (Apache-2.0): src/read.rs, src/write.rs, src/lib.rs
// 25+ ported boundary, roundtrip & malformed-input regression cases covering:
//   - U8<->I8 bijective conversion, narrowSampleToI8/I16/I24 range guards, defaultWavChannelMask
//   - PCMWAVEFORMAT (16B), WAVEFORMATEX (18B), WAVEFORMATEXTENSIBLE (40B), nonstandard-01 (PCM w/ 40B fmt)
//   - Unknown chunk skipping including odd-length chunks with 1-byte RIFF pad alignment
//   - valid_bits_per_sample == 0 fallback (hound nonstandard-02.wav) & non-multiple-of-8 20-bit MSB decoding
//   - 24-bit in 3-byte packed & 4-byte padded containers (arecord S24_LE / hound s24_wav_write)
//   - 8/16/24/32-bit Int & 32-bit IEEE Float exact lossless write -> read roundtrips
//   - Wide-read & format-mismatch guards, short-write & wide-write guards, flushHeader() checkpointing
//   - Infinite-stream header (`0xFFFFFFFF`), duration/len agreement, and malformed/truncated header defense matrix

import { test, expect_eq, expect_true } from "cap:test";
import {
    WAV_FORMAT_PCM,
    WAV_FORMAT_IEEE_FLOAT,
    WAV_FORMAT_EXTENSIBLE,
    AudioError,
    WavSampleFormat,
    WavInfoMetadata,
    WavCuePoint,
    parseWavContainer,
    defaultWavChannelMask,
    WavStreamConfig,
    encodeWavInfiniteHeader,
    WavStreamWriter,
    wavU8ToSignedI8,
    wavSignedI8ToU8,
    narrowSampleToI8,
    narrowSampleToI16,
    narrowSampleToI24,
    readWavSampleI24From3,
    readWavSampleI24From4,
    decodeWavSamplesI32,
    decodeWavSamplesI16,
    decodeWavSamplesF32,
} from "cap:media";

fn wU16(b: []u8, off: usize, v: u16) {
    b[off] = (v & 0xFF) as u8;
    b[off + 1] = ((v >> 8) & 0xFF) as u8;
}

fn wU32(b: []u8, off: usize, v: u32) {
    b[off] = (v & 0xFF) as u8;
    b[off + 1] = ((v >> 8) & 0xFF) as u8;
    b[off + 2] = ((v >> 16) & 0xFF) as u8;
    b[off + 3] = ((v >> 24) & 0xFF) as u8;
}

fn wTag(b: []u8, off: usize, a: u8, c1: u8, c2: u8, c3: u8) {
    b[off] = a;
    b[off + 1] = c1;
    b[off + 2] = c2;
    b[off + 3] = c3;
}

fn wSubGuid(b: []u8, off: usize, tag: u16) {
    const tail: [14]u8 = [0x00, 0x00, 0x00, 0x00, 0x10, 0x00, 0x80, 0x00, 0x00, 0xAA, 0x00, 0x38, 0x9B, 0x71];
    wU16(b, off, tag);
    let i: usize = 0;
    while (i < 14) : (i += 1) {
        b[off + 2 + i] = tail[i];
    }
}

// ============================================================================
// 1. Hound Primitives: U8<->I8 Bijection, Narrowing Range Guards & Channel Mask
// ============================================================================

test("hound primitives: u8<->i8 bijection, narrowSampleToI8/I16/I24 & defaultWavChannelMask") {
    // u8_sign_conversion_is_bijective
    let u: u32 = 0;
    while (u <= 255) : (u += 1) {
        const b: u8 = u as u8;
        expect_eq(wavSignedI8ToU8(wavU8ToSignedI8(b)), b);
    }
    let s: i32 = -128;
    while (s <= 127) : (s += 1) {
        const si: i8 = s as i8;
        expect_eq(wavU8ToSignedI8(wavSignedI8ToU8(si)), si);
    }

    // verify_narrow_to_i8 / i16 / i24
    expect_eq(narrowSampleToI8(127) catch unreachable, 127);
    expect_eq(narrowSampleToI8(-128) catch unreachable, -128);
    let e_i8_hi = false;
    * = narrowSampleToI8(128) catch { e_i8_hi = true; };
    expect_true(e_i8_hi);
    let e_i8_lo = false;
    * = narrowSampleToI8(-129) catch { e_i8_lo = true; };
    expect_true(e_i8_lo);

    expect_eq(narrowSampleToI16(32767) catch unreachable, 32767);
    expect_eq(narrowSampleToI16(-32768) catch unreachable, -32768);
    let e_i16_hi = false;
    * = narrowSampleToI16(32768) catch { e_i16_hi = true; };
    expect_true(e_i16_hi);
    let e_i16_lo = false;
    * = narrowSampleToI16(-32769) catch { e_i16_lo = true; };
    expect_true(e_i16_lo);

    expect_eq(narrowSampleToI24(8388607) catch unreachable, 8388607);
    expect_eq(narrowSampleToI24(-8388608) catch unreachable, -8388608);
    let e_i24_hi = false;
    * = narrowSampleToI24(8388608) catch { e_i24_hi = true; };
    expect_true(e_i24_hi);
    let e_i24_lo = false;
    * = narrowSampleToI24(-8388609) catch { e_i24_lo = true; };
    expect_true(e_i24_lo);

    // verify_channel_mask
    expect_eq(defaultWavChannelMask(0), 0);
    expect_eq(defaultWavChannelMask(1), 1);
    expect_eq(defaultWavChannelMask(2), 3);
    expect_eq(defaultWavChannelMask(3), 7);
    expect_eq(defaultWavChannelMask(4), 0xF);
    expect_eq(defaultWavChannelMask(8), 0xFF);
    expect_eq(defaultWavChannelMask(16), 0xFFFF);
    expect_eq(defaultWavChannelMask(18), 0x3FFFF);
    expect_eq(defaultWavChannelMask(32), 0x3FFFF);
    expect_eq(defaultWavChannelMask(64), 0x3FFFF);
    expect_eq(defaultWavChannelMask(129), 0x3FFFF);
}

// ============================================================================
// 2. Unknown Chunks & Odd-Length Chunk 1-Byte Pad Alignment
// ============================================================================

test("hound reader: skips unknown chunks including odd-length chunks with 1-byte pad") {
    // Build RIFF/WAVE with: fmt (18B WAVEFORMATEX) + JUNK (5B odd + 1B pad) + PEAK (4B) + data (8B: [2, -3, 5, -7])
    let wav: [128]u8 = [0; 128];
    let p: usize = 0;
    wTag(wav[0..], p, 'R', 'I', 'F', 'F'); p += 4;
    const riff_sz_off = p; p += 4;
    wTag(wav[0..], p, 'W', 'A', 'V', 'E'); p += 4;

    // WAVEFORMATEX fmt chunk (18 bytes)
    wTag(wav[0..], p, 'f', 'm', 't', ' '); p += 4;
    wU32(wav[0..], p, 18); p += 4;
    wU16(wav[0..], p, WAV_FORMAT_PCM); p += 2;
    wU16(wav[0..], p, 1); p += 2; // mono
    wU32(wav[0..], p, 44100); p += 4;
    wU32(wav[0..], p, 88200); p += 4;
    wU16(wav[0..], p, 2); p += 2;
    wU16(wav[0..], p, 16); p += 2;
    wU16(wav[0..], p, 0); p += 2; // cbSize = 0

    // Odd-length unknown chunk 'JUNK' (size = 5, padded to 6 bytes on wire)
    wTag(wav[0..], p, 'J', 'U', 'N', 'K'); p += 4;
    wU32(wav[0..], p, 5); p += 4;
    wav[p] = 'a'; wav[p + 1] = 'b'; wav[p + 2] = 'c'; wav[p + 3] = 'd'; wav[p + 4] = 'e'; wav[p + 5] = 0;
    p += 6;

    // Even-length unknown chunk 'bext' (size = 4)
    wTag(wav[0..], p, 'b', 'e', 'x', 't'); p += 4;
    wU32(wav[0..], p, 4); p += 4;
    p += 4;

    // data chunk: [2, -3, 5, -7]
    wTag(wav[0..], p, 'd', 'a', 't', 'a'); p += 4;
    wU32(wav[0..], p, 8); p += 4;
    const exp: [4]i16 = [2, -3, 5, -7];
    let i: usize = 0;
    while (i < 4) : (i += 1) {
        wU16(wav[0..], p, @as(u16, @bitCast(exp[i])));
        p += 2;
    }
    wU32(wav[0..], riff_sz_off, (p - 8) as u32);

    let cues: [0]WavCuePoint = [];
    const info = parseWavContainer(wav[0..p], cues[0..]) catch unreachable;
    expect_eq(info.channels, 1);
    expect_eq(info.sample_rate, 44100);
    expect_eq(info.bits_per_sample, 16);
    expect_eq(info.sample_count, 4);
    expect_eq(info.duration_frames, 4);

    let decoded: [4]i16 = [0; 4];
    expect_eq(decodeWavSamplesI16(info, decoded[0..]) catch unreachable, 4);
    i = 0;
    while (i < 4) : (i += 1) {
        expect_eq(decoded[i], exp[i]);
    }
}

// ============================================================================
// 3. WAVE_FORMAT_EXTENSIBLE 0 valid_bits Fallback, 20-bit Non-Multiple-of-8 & Nonstandard-01
// ============================================================================

test("hound reader: 0 valid_bits fallback, 20-bit MSB-aligned decoding & nonstandard-01 40B PCM") {
    let cues: [0]WavCuePoint = [];

    // 3a. nonstandard-02.wav: WAVE_FORMAT_EXTENSIBLE 32-bit stereo with valid_bits_per_sample = 0 -> fallback to 32
    let wav0: [128]u8 = [0; 128];
    let p: usize = 0;
    wTag(wav0[0..], p, 'R', 'I', 'F', 'F'); p += 4;
    const rsz0 = p; p += 4;
    wTag(wav0[0..], p, 'W', 'A', 'V', 'E'); p += 4;
    wTag(wav0[0..], p, 'f', 'm', 't', ' '); p += 4;
    wU32(wav0[0..], p, 40); p += 4;
    wU16(wav0[0..], p, WAV_FORMAT_EXTENSIBLE); p += 2;
    wU16(wav0[0..], p, 2); p += 2;
    wU32(wav0[0..], p, 48000); p += 4;
    wU32(wav0[0..], p, 384000); p += 4;
    wU16(wav0[0..], p, 8); p += 2;
    wU16(wav0[0..], p, 32); p += 2;
    wU16(wav0[0..], p, 22); p += 2;
    wU16(wav0[0..], p, 0); p += 2; // valid_bits_per_sample = 0 -> must fallback to 32!
    wU32(wav0[0..], p, 3); p += 4;
    wSubGuid(wav0[0..], p, WAV_FORMAT_PCM); p += 16;
    wTag(wav0[0..], p, 'd', 'a', 't', 'a'); p += 4;
    wU32(wav0[0..], p, 16); p += 4;
    const exp32: [4]i32 = [19, -229373, 33587161, -2147483497];
    let i: usize = 0;
    while (i < 4) : (i += 1) {
        wU32(wav0[0..], p, @as(u32, @bitCast(exp32[i])));
        p += 4;
    }
    wU32(wav0[0..], rsz0, (p - 8) as u32);

    const info0 = parseWavContainer(wav0[0..p], cues[0..]) catch unreachable;
    expect_eq(info0.valid_bits_per_sample, 32);
    expect_eq(info0.sample_format, WavSampleFormat.s32_le);
    let out32: [4]i32 = [0; 4];
    expect_eq(decodeWavSamplesI32(info0, out32[0..]) catch unreachable, 4);
    i = 0;
    while (i < 4) : (i += 1) {
        expect_eq(out32[i], exp32[i]);
    }

    // 3b. Non-multiple-of-8: 20-bit valid_bits_per_sample MSB-aligned inside 24-bit (3-byte) container
    let wav20: [128]u8 = [0; 128];
    let w20 = WavStreamWriter.init(wav20[0..], WavStreamConfig{
        sample_format: WavSampleFormat.s24_le,
        sample_rate: 48000,
        channels: 1,
        valid_bits_per_sample: 20,
        auto_extensible: true,
    }, WavInfoMetadata{}) catch unreachable;
    // Encode 20-bit values [524287 (max 20-bit), -524288 (min 20-bit), -17, 0] shifted left by 4 into 24-bit container
    const s20_in: [4]i32 = [524287 << 4, -524288 << 4, -17 << 4, 0];
    * = w20.writeI24(s20_in[0..]) catch unreachable;
    const len20 = w20.finalize() catch unreachable;
    const info20 = parseWavContainer(wav20[0..len20], cues[0..]) catch unreachable;
    expect_true(info20.is_extensible);
    expect_eq(info20.bits_per_sample, 24);
    expect_eq(info20.valid_bits_per_sample, 20);
    let out20: [4]i32 = [0; 4];
    expect_eq(decodeWavSamplesI32(info20, out20[0..]) catch unreachable, 4);
    expect_eq(out20[0], 524287);
    expect_eq(out20[1], -524288);
    expect_eq(out20[2], -17);
    expect_eq(out20[3], 0);

    // 3c. nonstandard-01.wav: wFormatTag = PCM (1) with 40-byte fmt chunk & 24-bit samples [0, 0]
    let wav_ns: [128]u8 = [0; 128];
    p = 0;
    wTag(wav_ns[0..], p, 'R', 'I', 'F', 'F'); p += 4;
    const rsz_ns = p; p += 4;
    wTag(wav_ns[0..], p, 'W', 'A', 'V', 'E'); p += 4;
    wTag(wav_ns[0..], p, 'f', 'm', 't', ' '); p += 4;
    wU32(wav_ns[0..], p, 40); p += 4;
    wU16(wav_ns[0..], p, WAV_FORMAT_PCM); p += 2;
    wU16(wav_ns[0..], p, 2); p += 2;
    wU32(wav_ns[0..], p, 44100); p += 4;
    wU32(wav_ns[0..], p, 44100 * 6); p += 4;
    wU16(wav_ns[0..], p, 6); p += 2;
    wU16(wav_ns[0..], p, 24); p += 2;
    p += 24; // 24 extra zero bytes in 40B fmt
    wTag(wav_ns[0..], p, 'd', 'a', 't', 'a'); p += 4;
    wU32(wav_ns[0..], p, 6); p += 4;
    p += 6; // two zero 24-bit samples
    wU32(wav_ns[0..], rsz_ns, (p - 8) as u32);

    const info_ns = parseWavContainer(wav_ns[0..p], cues[0..]) catch unreachable;
    expect_eq(info_ns.bits_per_sample, 24);
    expect_eq(info_ns.bytes_per_sample, 3);
    let out_ns: [2]i32 = [9, 9];
    expect_eq(decodeWavSamplesI32(info_ns, out_ns[0..]) catch unreachable, 2);
    expect_eq(out_ns[0], 0);
    expect_eq(out_ns[1], 0);
}

// ============================================================================
// 4. 24-bit in 3-Byte Packed & 4-Byte Padded Container (hound s24_wav_write)
// ============================================================================

test("hound reader/writer: 24-bit 3-byte packed & 4-byte container sign extension & roundtrip") {
    let cues: [0]WavCuePoint = [];

    // 4a. 24-bit in 3-byte packed container (hound waveformatextensible-24bit-192kHz-mono.wav)
    const exp24_3: [4]i32 = [-17, 4194319, -6291437, 8355817];
    let buf3: [128]u8 = [0; 128];
    let w3 = WavStreamWriter.init(buf3[0..], WavStreamConfig{
        sample_format: WavSampleFormat.s24_le,
        sample_rate: 192000,
        channels: 1,
        auto_extensible: true,
    }, WavInfoMetadata{}) catch unreachable;
    * = w3.writeI24(exp24_3[0..]) catch unreachable;
    const len3 = w3.finalize() catch unreachable;
    const info3 = parseWavContainer(buf3[0..len3], cues[0..]) catch unreachable;
    expect_true(info3.is_extensible);
    expect_eq(info3.sample_rate, 192000);
    expect_eq(info3.bytes_per_sample, 3);
    expect_eq(info3.valid_bits_per_sample, 24);
    let out3: [4]i32 = [0; 4];
    expect_eq(decodeWavSamplesI32(info3, out3[0..]) catch unreachable, 4);
    let i: usize = 0;
    while (i < 4) : (i += 1) {
        expect_eq(out3[i], exp24_3[i]);
    }
    expect_eq(readWavSampleI24From3(info3.data_slice, 0), -17);

    // 4b. 24-bit in 4-byte container (hound s24_wav_write & waveformatextensible-24bit-4byte-48kHz-stereo.wav)
    const exp24_4: [4]i32 = [-96, 23052, 8388607, -8360672];
    let buf4: [128]u8 = [0; 128];
    let w4 = WavStreamWriter.init(buf4[0..], WavStreamConfig{
        sample_format: WavSampleFormat.s24_le,
        sample_rate: 48000,
        channels: 2,
        container_bytes_per_sample: 4,
        auto_extensible: true,
    }, WavInfoMetadata{}) catch unreachable;
    * = w4.writeI24(exp24_4[0..]) catch unreachable;
    const len4 = w4.finalize() catch unreachable;
    expect_eq(len4, 68 + 16); // 68B WAVEFORMATEXTENSIBLE header + 4 * 4B samples
    const info4 = parseWavContainer(buf4[0..len4], cues[0..]) catch unreachable;
    expect_true(info4.is_extensible);
    expect_eq(info4.channels, 2);
    expect_eq(info4.sample_rate, 48000);
    expect_eq(info4.bits_per_sample, 32);
    expect_eq(info4.valid_bits_per_sample, 24);
    expect_eq(info4.bytes_per_sample, 4);
    expect_eq(info4.channel_mask, 3);
    let out4: [4]i32 = [0; 4];
    expect_eq(decodeWavSamplesI32(info4, out4[0..]) catch unreachable, 4);
    i = 0;
    while (i < 4) : (i += 1) {
        expect_eq(out4[i], exp24_4[i]);
    }
    expect_eq(readWavSampleI24From4(info4.data_slice, 0), -96);
}

// ============================================================================
// 5. Full Lossless Roundtrips (8-bit 16ch, 16-bit stereo, 24-bit 16ch, 32-bit Int & F32)
// ============================================================================

test("hound roundtrip: lossless i8(16ch), i16(2ch), i24(16ch), i32(2ch) & f32(2ch) + duration/len") {
    let cues: [0]WavCuePoint = [];

    // 5a. write_read_i8_is_lossless (16 channels -> auto WAVE_FORMAT_EXTENSIBLE, all 256 i8 values)
    let i8_src: [256]i8 = [0; 256];
    let k: usize = 0;
    while (k < 256) : (k += 1) {
        i8_src[k] = ((k as i32) - 128) as i8;
    }
    let b_i8: [512]u8 = [0; 512];
    let w_i8 = WavStreamWriter.init(b_i8[0..], WavStreamConfig{
        sample_format: WavSampleFormat.u8_pcm,
        sample_rate: 48000,
        channels: 16,
        auto_extensible: true,
    }, WavInfoMetadata{}) catch unreachable;
    * = w_i8.writeI8(i8_src[0..]) catch unreachable;
    expect_eq(w_i8.durationFrames(), 16);
    expect_eq(w_i8.sampleCount(), 256);
    const l_i8 = w_i8.finalize() catch unreachable;
    const p_i8 = parseWavContainer(b_i8[0..l_i8], cues[0..]) catch unreachable;
    expect_true(p_i8.is_extensible);
    expect_eq(p_i8.channel_mask, 0xFFFF);
    expect_eq((p_i8.channels as u64) * p_i8.duration_frames, p_i8.sample_count);
    let i8_dec: [256]i16 = [0; 256];
    expect_eq(decodeWavSamplesI16(p_i8, i8_dec[0..]) catch unreachable, 256);
    k = 0;
    while (k < 256) : (k += 1) {
        expect_eq(i8_dec[k], i8_src[k] as i16);
    }

    // 5b. write_read_i16_is_lossless (2ch -> standard 44B PCMWAVEFORMAT)
    let i16_src: [64]i16 = [0; 64];
    k = 0;
    while (k < 64) : (k += 1) {
        i16_src[k] = ((k as i32) * 511 - 16000) as i16;
    }
    let b_i16: [256]u8 = [0; 256];
    let w_i16 = WavStreamWriter.init(b_i16[0..], WavStreamConfig{
        sample_format: WavSampleFormat.s16_le,
        sample_rate: 44100,
        channels: 2,
        auto_extensible: true,
    }, WavInfoMetadata{}) catch unreachable;
    * = w_i16.writePcm16(i16_src[0..]) catch unreachable;
    const l_i16 = w_i16.finalize() catch unreachable;
    const p_i16 = parseWavContainer(b_i16[0..l_i16], cues[0..]) catch unreachable;
    expect_true(!p_i16.is_extensible);
    let i16_dec: [64]i16 = [0; 64];
    expect_eq(decodeWavSamplesI16(p_i16, i16_dec[0..]) catch unreachable, 64);
    k = 0;
    while (k < 64) : (k += 1) {
        expect_eq(i16_dec[k], i16_src[k]);
    }

    // 5c. write_read_i32_is_lossless
    const i32_src: [4]i32 = [19, -229373, 33587161, -2147483497];
    let b_i32: [128]u8 = [0; 128];
    let w_i32 = WavStreamWriter.init(b_i32[0..], WavStreamConfig{
        sample_format: WavSampleFormat.s32_le,
        sample_rate: 48000,
        channels: 2,
        auto_extensible: true,
    }, WavInfoMetadata{}) catch unreachable;
    * = w_i32.writeI32(i32_src[0..]) catch unreachable;
    const l_i32 = w_i32.finalize() catch unreachable;
    const p_i32 = parseWavContainer(b_i32[0..l_i32], cues[0..]) catch unreachable;
    expect_true(p_i32.is_extensible);
    let i32_dec: [4]i32 = [0; 4];
    expect_eq(decodeWavSamplesI32(p_i32, i32_dec[0..]) catch unreachable, 4);
    k = 0;
    while (k < 4) : (k += 1) {
        expect_eq(i32_dec[k], i32_src[k]);
    }

    // 5d. write_read_f32_is_lossless (hound [2.0, 3.0, -16411.0, 1019.0])
    const f32_src: [4]f32 = [2.0, 3.0, -16411.0, 1019.0];
    let b_f32: [128]u8 = [0; 128];
    let w_f32 = WavStreamWriter.init(b_f32[0..], WavStreamConfig{
        sample_format: WavSampleFormat.f32_le,
        sample_rate: 44100,
        channels: 1,
        auto_extensible: true,
    }, WavInfoMetadata{}) catch unreachable;
    * = w_f32.writeF32(f32_src[0..]) catch unreachable;
    const l_f32 = w_f32.finalize() catch unreachable;
    const p_f32 = parseWavContainer(b_f32[0..l_f32], cues[0..]) catch unreachable;
    let f32_dec: [4]f32 = [0.0; 4];
    expect_eq(decodeWavSamplesF32(p_f32, f32_dec[0..]) catch unreachable, 4);
    k = 0;
    while (k < 4) : (k += 1) {
        expect_eq(f32_dec[k], f32_src[k]);
    }
}

// ============================================================================
// 6. Wide-Read, Format-Mismatch, Wide-Write, Flush Checkpointing & Infinite Header
// ============================================================================

test("hound guards: wide_read, format_mismatch, wide_write, flushHeader & infinite header") {
    let cues: [0]WavCuePoint = [];

    // 6a. wide_read_should_signal_error & sample_format_mismatch_should_signal_error
    let b24: [128]u8 = [0; 128];
    let w24 = WavStreamWriter.init(b24[0..], WavStreamConfig{
        sample_format: WavSampleFormat.s24_le,
        sample_rate: 48000,
        channels: 1,
        auto_extensible: true,
    }, WavInfoMetadata{}) catch unreachable;
    // wide_write_should_signal_error: 8388608 overflows 24-bit
    const bad24: [1]i32 = [8388608];
    let got_wide_w = false;
    * = w24.writeI24(bad24[0..]) catch |e| {
        if (e == AudioError.InvalidParameter) got_wide_w = true;
    };
    expect_true(got_wide_w);

    const ok24: [2]i32 = [17, -17];
    * = w24.writeI24(ok24[0..]) catch unreachable;
    const l24 = w24.finalize() catch unreachable;
    const p24 = parseWavContainer(b24[0..l24], cues[0..]) catch unreachable;

    let tmp16: [4]i16 = [0; 4];
    let got_wide_r = false;
    * = decodeWavSamplesI16(p24, tmp16[0..]) catch |e| {
        if (e == AudioError.InvalidParameter) got_wide_r = true;
    };
    expect_true(got_wide_r);

    let tmpf: [4]f32 = [0.0; 4];
    let got_fmt_mis1 = false;
    * = decodeWavSamplesF32(p24, tmpf[0..]) catch |e| {
        if (e == AudioError.UnsupportedFormat) got_fmt_mis1 = true;
    };
    expect_true(got_fmt_mis1);

    // Float container decoded as i32 -> UnsupportedFormat
    let bf: [128]u8 = [0; 128];
    let wf = WavStreamWriter.init(bf[0..], WavStreamConfig{
        sample_format: WavSampleFormat.f32_le,
        sample_rate: 44100,
        channels: 1,
    }, WavInfoMetadata{}) catch unreachable;
    const f1: [2]f32 = [0.25, -0.5];
    * = wf.writeF32(f1[0..]) catch unreachable;
    const lf = wf.finalize() catch unreachable;
    const pf = parseWavContainer(bf[0..lf], cues[0..]) catch unreachable;
    let tmp32: [4]i32 = [0; 4];
    let got_fmt_mis2 = false;
    * = decodeWavSamplesI32(pf, tmp32[0..]) catch |e| {
        if (e == AudioError.UnsupportedFormat) got_fmt_mis2 = true;
    };
    expect_true(got_fmt_mis2);

    // 6b. flush_should_produce_valid_file: checkpoint after 6 samples, then write 2 more without finalizing
    let bfl: [128]u8 = [0; 128];
    let wfl = WavStreamWriter.init(bfl[0..], WavStreamConfig{
        sample_format: WavSampleFormat.s16_le,
        sample_rate: 44100,
        channels: 2,
    }, WavInfoMetadata{}) catch unreachable;
    const s_init: [6]i16 = [2, 4, 5, 7, 11, 13];
    * = wfl.writePcm16(s_init[0..]) catch unreachable;
    const flushed_len = wfl.flushHeader() catch unreachable;
    // Append 2 more samples without flushing/finalizing
    const s_extra: [2]i16 = [17, 19];
    * = wfl.writePcm16(s_extra[0..]) catch unreachable;

    // Parsing the flushed prefix sees exactly the 6 checkpointed samples
    const p_fl = parseWavContainer(bfl[0..flushed_len], cues[0..]) catch unreachable;
    expect_eq(p_fl.sample_count, 6);
    let dec_fl: [6]i16 = [0; 6];
    expect_eq(decodeWavSamplesI16(p_fl, dec_fl[0..]) catch unreachable, 6);
    expect_eq(dec_fl[0], 2);
    expect_eq(dec_fl[5], 13);

    // 6c. encodeWavInfiniteHeader (hound into_header_for_infinite_file)
    let inf_pcm: [68]u8 = [0; 68];
    const inf_l1 = encodeWavInfiniteHeader(inf_pcm[0..], WavStreamConfig{
        sample_format: WavSampleFormat.s16_le,
        sample_rate: 16000,
        channels: 1,
    }) catch unreachable;
    expect_eq(inf_l1, 44);
    expect_eq(inf_pcm[4], 0xFF);
    expect_eq(inf_pcm[7], 0xFF);
    expect_eq(inf_pcm[40], 0xFF);
    expect_eq(inf_pcm[43], 0xFF);

    let inf_ext: [68]u8 = [0; 68];
    const inf_l2 = encodeWavInfiniteHeader(inf_ext[0..], WavStreamConfig{
        sample_format: WavSampleFormat.s24_le,
        sample_rate: 48000,
        channels: 4,
        auto_extensible: true,
    }) catch unreachable;
    expect_eq(inf_l2, 68);
    expect_eq(inf_ext[64], 0xFF);
    expect_eq(inf_ext[67], 0xFF);
}

// ============================================================================
// 7. Malformed & Truncated WAV Boundary Defense Matrix
// ============================================================================

test("hound malformed matrix: short fmt, zero ch/rate, bad byte_rate/cbSize/GUID, data before fmt, truncated data") {
    let cues: [0]WavCuePoint = [];

    // Helper: build a valid 44B PCM16 mono 44100Hz WAV with 4 bytes data, then mutate per test
    let base: [48]u8 = [0; 48];
    wTag(base[0..], 0, 'R', 'I', 'F', 'F');
    wU32(base[0..], 4, 40);
    wTag(base[0..], 8, 'W', 'A', 'V', 'E');
    wTag(base[0..], 12, 'f', 'm', 't', ' ');
    wU32(base[0..], 16, 16);
    wU16(base[0..], 20, WAV_FORMAT_PCM);
    wU16(base[0..], 22, 1);
    wU32(base[0..], 24, 44100);
    wU32(base[0..], 28, 88200);
    wU16(base[0..], 32, 2);
    wU16(base[0..], 34, 16);
    wTag(base[0..], 36, 'd', 'a', 't', 'a');
    wU32(base[0..], 40, 4);

    // Sanity: base is valid
    * = parseWavContainer(base[0..], cues[0..]) catch unreachable;

    // 7a. 0 channels -> InvalidHeader
    let m1 = base;
    wU16(m1[0..], 22, 0);
    let ok = false;
    * = parseWavContainer(m1[0..], cues[0..]) catch |e| { if (e == AudioError.InvalidHeader) ok = true; };
    expect_true(ok);

    // 7b. 0 sample_rate -> InvalidHeader
    let m2 = base;
    wU32(m2[0..], 24, 0);
    ok = false;
    * = parseWavContainer(m2[0..], cues[0..]) catch |e| { if (e == AudioError.InvalidHeader) ok = true; };
    expect_true(ok);

    // 7c. Non-multiple-of-8 bits_per_sample (12) -> InvalidHeader
    let m3 = base;
    wU16(m3[0..], 34, 12);
    ok = false;
    * = parseWavContainer(m3[0..], cues[0..]) catch |e| { if (e == AudioError.InvalidHeader) ok = true; };
    expect_true(ok);

    // 7d. Inconsistent byte_rate -> InvalidHeader
    let m4 = base;
    wU32(m4[0..], 28, 12345);
    ok = false;
    * = parseWavContainer(m4[0..], cues[0..]) catch |e| { if (e == AudioError.InvalidHeader) ok = true; };
    expect_true(ok);

    // 7e. Data length not multiple of block_align (3 bytes for 16-bit mono) -> TruncatedData
    let m5 = base;
    wU32(m5[0..], 40, 3);
    ok = false;
    * = parseWavContainer(m5[0..], cues[0..]) catch |e| { if (e == AudioError.TruncatedData) ok = true; };
    expect_true(ok);

    // 7f. Data chunk declares 100 bytes but file only has 4 -> TruncatedData
    let m6 = base;
    wU32(m6[0..], 40, 100);
    ok = false;
    * = parseWavContainer(m6[0..], cues[0..]) catch |e| { if (e == AudioError.TruncatedData) ok = true; };
    expect_true(ok);

    // 7g. Short fmt chunk (14 bytes < 16) -> InvalidHeader
    let m7: [34]u8 = [0; 34];
    wTag(m7[0..], 0, 'R', 'I', 'F', 'F');
    wU32(m7[0..], 4, 26);
    wTag(m7[0..], 8, 'W', 'A', 'V', 'E');
    wTag(m7[0..], 12, 'f', 'm', 't', ' ');
    wU32(m7[0..], 16, 14);
    ok = false;
    * = parseWavContainer(m7[0..], cues[0..]) catch |e| { if (e == AudioError.InvalidHeader) ok = true; };
    expect_true(ok);

    // 7h. Unknown SubFormat GUID in WAVE_FORMAT_EXTENSIBLE -> UnsupportedFormat
    let m8: [72]u8 = [0; 72];
    wTag(m8[0..], 0, 'R', 'I', 'F', 'F');
    wU32(m8[0..], 4, 64);
    wTag(m8[0..], 8, 'W', 'A', 'V', 'E');
    wTag(m8[0..], 12, 'f', 'm', 't', ' ');
    wU32(m8[0..], 16, 40);
    wU16(m8[0..], 20, WAV_FORMAT_EXTENSIBLE);
    wU16(m8[0..], 22, 1);
    wU32(m8[0..], 24, 48000);
    wU32(m8[0..], 28, 96000);
    wU16(m8[0..], 32, 2);
    wU16(m8[0..], 34, 16);
    wU16(m8[0..], 36, 22);
    wU16(m8[0..], 38, 16);
    wU32(m8[0..], 40, 1);
    m8[44] = 0xDE; m8[45] = 0xAD; // bogus GUID
    wTag(m8[0..], 60, 'd', 'a', 't', 'a');
    wU32(m8[0..], 64, 4);
    ok = false;
    * = parseWavContainer(m8[0..], cues[0..]) catch |e| { if (e == AudioError.UnsupportedFormat) ok = true; };
    expect_true(ok);

    // 7i. data chunk before fmt chunk -> InvalidHeader
    let m9: [48]u8 = [0; 48];
    wTag(m9[0..], 0, 'R', 'I', 'F', 'F');
    wU32(m9[0..], 4, 40);
    wTag(m9[0..], 8, 'W', 'A', 'V', 'E');
    wTag(m9[0..], 12, 'd', 'a', 't', 'a');
    wU32(m9[0..], 16, 4);
    wTag(m9[0..], 24, 'f', 'm', 't', ' ');
    wU32(m9[0..], 28, 16);
    ok = false;
    * = parseWavContainer(m9[0..], cues[0..]) catch |e| { if (e == AudioError.InvalidHeader) ok = true; };
    expect_true(ok);
}
