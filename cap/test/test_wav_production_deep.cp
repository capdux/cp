// Source: cp/cap/test/test_wav_production_deep.cp
// Comprehensive Production-Grade Verification Suite for cap:wav & cap.wav

const std = @import("std");
import { test, expect_eq } from "cap:test";
import cap from "cap";
import {
    WavError,
    SampleFormat,
    WavAudio,
    decode,
    encode,
    toMono,
    resampleLinear,
    peakAmplitude,
    normalizePeak,
} from "cap:wav";

fn approxF32(a: f32, b: f32, eps: f32): bool {
    return @abs(a - b) <= eps;
}

fn approxF64(a: f64, b: f64, eps: f64): bool {
    return @abs(a - b) <= eps;
}

fn appendU16Le(buf: []u8, off: usize, val: u16) {
    buf[off] = (val & 0xFF) as u8;
    buf[off + 1] = ((val >> 8) & 0xFF) as u8;
}

fn appendU32Le(buf: []u8, off: usize, val: u32) {
    buf[off] = (val & 0xFF) as u8;
    buf[off + 1] = ((val >> 8) & 0xFF) as u8;
    buf[off + 2] = ((val >> 16) & 0xFF) as u8;
    buf[off + 3] = ((val >> 24) & 0xFF) as u8;
}

fn testAllFiveFormatsRoundtrip(): !void {
    let gpa: std.heap.DebugAllocator(.{}) = .init;
    defer std.debug.assert(gpa.deinit() == .ok);
    const alloc = gpa.allocator();
    let raw: [6]f32 = [-1.0, -0.5, -0.125, 0.0, 0.375, 0.95];
    const src = WavAudio {
        sample_rate: 16000,
        channels: 2,
        format: SampleFormat.Float32,
        samples_f32: &raw,
    };
    expect_eq(src.frameCount(), 3);

    // 1. Pcm8 (8-bit unsigned offset 128)
    const b8 = encode(alloc, src, SampleFormat.Pcm8)?;
    defer alloc.free(b8);
    let d8 = decode(alloc, b8)?;
    defer d8.deinit(alloc);
    expect_eq(d8.format == SampleFormat.Pcm8, true);
    expect_eq(d8.frameCount(), 3);
    for (i of 0..6) {
        expect_eq(approxF32(d8.samples_f32[i], raw[i], 0.02), true);
    }

    // 2. Pcm16
    const b16 = encode(alloc, src, SampleFormat.Pcm16)?;
    defer alloc.free(b16);
    let d16 = decode(alloc, b16)?;
    defer d16.deinit(alloc);
    expect_eq(d16.format == SampleFormat.Pcm16, true);
    expect_eq(d16.samples_f32.len, 6);
    for (i of 0..6) {
        expect_eq(approxF32(d16.samples_f32[i], raw[i], 0.0002), true);
    }

    // 3. Pcm24 (3-byte little-endian two's complement)
    const b24 = encode(alloc, src, SampleFormat.Pcm24)?;
    defer alloc.free(b24);
    expect_eq(b24.len, 44 + 6 * 3);
    let d24 = decode(alloc, b24)?;
    defer d24.deinit(alloc);
    expect_eq(d24.format == SampleFormat.Pcm24, true);
    for (i of 0..6) {
        expect_eq(approxF32(d24.samples_f32[i], raw[i], 0.00001), true);
    }

    // 4. Pcm32
    const b32 = encode(alloc, src, SampleFormat.Pcm32)?;
    defer alloc.free(b32);
    expect_eq(b32.len, 44 + 6 * 4);
    let d32 = decode(alloc, b32)?;
    defer d32.deinit(alloc);
    expect_eq(d32.format == SampleFormat.Pcm32, true);
    for (i of 0..6) {
        expect_eq(approxF32(d32.samples_f32[i], raw[i], 0.000001), true);
    }

    // 5. Float32 + saturation clamping on out-of-range values
    let over: [3]f32 = [-1.75, 0.25, 2.5];
    const over_src = WavAudio {
        sample_rate: 48000,
        channels: 1,
        format: SampleFormat.Float32,
        samples_f32: &over,
    };
    const bf32 = encode(alloc, over_src, SampleFormat.Float32)?;
    defer alloc.free(bf32);
    let df32 = decode(alloc, bf32)?;
    defer df32.deinit(alloc);
    expect_eq(df32.format == SampleFormat.Float32, true);
    expect_eq(approxF32(df32.samples_f32[0], -1.0, 0.000001), true);
    expect_eq(approxF32(df32.samples_f32[1], 0.25, 0.000001), true);
    expect_eq(approxF32(df32.samples_f32[2], 1.0, 0.000001), true);
}

fn testSkipUnknownAndOddChunks(): !void {
    let gpa: std.heap.DebugAllocator(.{}) = .init;
    defer std.debug.assert(gpa.deinit() == .ok);
    const alloc = gpa.allocator();
    // Header(12) + fmt(24) + JUNK(8+4=12) + LIST(8+3+1pad=12) + fact(8+4=12) + data(8+4=12) = 84 bytes
    let buf: [84]u8 = [0; 84];
    buf[0] = 'R'; buf[1] = 'I'; buf[2] = 'F'; buf[3] = 'F';
    appendU32Le(&buf, 4, 76);
    buf[8] = 'W'; buf[9] = 'A'; buf[10] = 'V'; buf[11] = 'E';

    // fmt chunk
    buf[12] = 'f'; buf[13] = 'm'; buf[14] = 't'; buf[15] = ' ';
    appendU32Le(&buf, 16, 16);
    appendU16Le(&buf, 20, 1);     // PCM
    appendU16Le(&buf, 22, 1);     // mono
    appendU32Le(&buf, 24, 8000);  // 8kHz
    appendU32Le(&buf, 28, 16000); // byte rate
    appendU16Le(&buf, 32, 2);     // block align
    appendU16Le(&buf, 34, 16);    // 16-bit

    // JUNK chunk (4 bytes)
    buf[36] = 'J'; buf[37] = 'U'; buf[38] = 'N'; buf[39] = 'K';
    appendU32Le(&buf, 40, 4);
    buf[44] = 0xDE; buf[45] = 0xAD; buf[46] = 0xBE; buf[47] = 0xEF;

    // LIST chunk (odd 3-byte payload + 1 pad byte)
    buf[48] = 'L'; buf[49] = 'I'; buf[50] = 'S'; buf[51] = 'T';
    appendU32Le(&buf, 52, 3);
    buf[56] = 1; buf[57] = 2; buf[58] = 3; buf[59] = 0;

    // fact chunk (4 bytes)
    buf[60] = 'f'; buf[61] = 'a'; buf[62] = 'c'; buf[63] = 't';
    appendU32Le(&buf, 64, 4);
    appendU32Le(&buf, 68, 2);

    // data chunk (2 i16 samples: 16384 -> 0.5, -16384 -> -0.5)
    buf[72] = 'd'; buf[73] = 'a'; buf[74] = 't'; buf[75] = 'a';
    appendU32Le(&buf, 76, 4);
    appendU16Le(&buf, 80, @as(u16, @bitCast(16384 as i16)));
    appendU16Le(&buf, 82, @as(u16, @bitCast(-16384 as i16)));

    let decoded = decode(alloc, &buf)?;
    defer decoded.deinit(alloc);
    expect_eq(decoded.sample_rate, 8000);
    expect_eq(decoded.channels, 1);
    expect_eq(decoded.frameCount(), 2);
    expect_eq(approxF32(decoded.samples_f32[0], 0.5, 0.0001), true);
    expect_eq(approxF32(decoded.samples_f32[1], -0.5, 0.0001), true);
}

fn testMonoResampleAndPeakNormalize(): !void {
    let gpa: std.heap.DebugAllocator(.{}) = .init;
    defer std.debug.assert(gpa.deinit() == .ok);
    const alloc = gpa.allocator();

    // 480 frames @ 48000 Hz stereo (0.01 sec)
    const in_frames: usize = 480;
    let stereo = alloc.alloc(f32, in_frames * 2)?;
    defer alloc.free(stereo);
    for (i of 0..in_frames) {
        stereo[i * 2] = 0.2;
        stereo[i * 2 + 1] = 0.6;
    }

    const src = WavAudio {
        sample_rate: 48000,
        channels: 2,
        format: SampleFormat.Float32,
        samples_f32: stereo,
    };
    expect_eq(approxF64(src.durationSec(), 0.01, 0.0001), true);

    let mono = toMono(alloc, src)?;
    defer mono.deinit(alloc);
    expect_eq(mono.channels, 1);
    expect_eq(mono.frameCount(), 480);
    expect_eq(approxF32(mono.samples_f32[0], 0.4, 0.0001), true);

    let resampled = resampleLinear(alloc, mono, 16000)?;
    defer resampled.deinit(alloc);
    expect_eq(resampled.sample_rate, 16000);
    expect_eq(resampled.channels, 1);
    expect_eq(resampled.frameCount(), 160);
    expect_eq(approxF64(resampled.durationSec(), 0.01, 0.0001), true);

    const p_before = peakAmplitude(resampled);
    expect_eq(approxF32(p_before, 0.4, 0.0001), true);

    normalizePeak(&resampled, 0.8);
    const p_after = peakAmplitude(resampled);
    expect_eq(approxF32(p_after, 0.8, 0.0001), true);
}

fn testValidationAndErrorInterception(): !void {
    let gpa: std.heap.DebugAllocator(.{}) = .init;
    defer std.debug.assert(gpa.deinit() == .ok);
    const alloc = gpa.allocator();

    // 1. Invalid RIFF header
    const bad_riff: [12]u8 = ['B', 'A', 'D', '!', 4, 0, 0, 0, 'W', 'A', 'V', 'E'];
    let got_bad_riff = false;
    * = decode(alloc, &bad_riff) catch e => {
        if (std.mem.eql(u8, @errorName(e), "InvalidRiffHeader")) got_bad_riff = true;
    };
    expect_eq(got_bad_riff, true);

    // 2. Missing data chunk (RIFF + WAVE + fmt only = 36 bytes)
    let no_data: [36]u8 = [0; 36];
    no_data[0] = 'R'; no_data[1] = 'I'; no_data[2] = 'F'; no_data[3] = 'F';
    appendU32Le(&no_data, 4, 28);
    no_data[8] = 'W'; no_data[9] = 'A'; no_data[10] = 'V'; no_data[11] = 'E';
    no_data[12] = 'f'; no_data[13] = 'm'; no_data[14] = 't'; no_data[15] = ' ';
    appendU32Le(&no_data, 16, 16);
    appendU16Le(&no_data, 20, 1);
    appendU16Le(&no_data, 22, 1);
    appendU32Le(&no_data, 24, 16000);
    appendU32Le(&no_data, 28, 32000);
    appendU16Le(&no_data, 32, 2);
    appendU16Le(&no_data, 34, 16);

    let got_missing_data = false;
    * = decode(alloc, &no_data) catch e => {
        if (std.mem.eql(u8, @errorName(e), "MissingDataChunk")) got_missing_data = true;
    };
    expect_eq(got_missing_data, true);

    // 3. Truncated data
    let valid_s: [2]f32 = [0.25, -0.25];
    const valid_wav = encode(alloc, WavAudio {
        sample_rate: 16000,
        channels: 1,
        format: SampleFormat.Pcm16,
        samples_f32: &valid_s,
    }, SampleFormat.Pcm16)?;
    defer alloc.free(valid_wav);

    let got_truncated = false;
    * = decode(alloc, valid_wav[0..(valid_wav.len - 2)]) catch e => {
        if (std.mem.eql(u8, @errorName(e), "TruncatedData")) got_truncated = true;
    };
    expect_eq(got_truncated, true);

    // 4. Zero channels
    let got_zero_ch = false;
    * = encode(alloc, WavAudio {
        sample_rate: 16000,
        channels: 0,
        format: SampleFormat.Pcm16,
        samples_f32: &valid_s,
    }, SampleFormat.Pcm16) catch e => {
        if (std.mem.eql(u8, @errorName(e), "InvalidChannelCount")) got_zero_ch = true;
    };
    expect_eq(got_zero_ch, true);

    // 5. Zero sample rate
    let got_zero_sr = false;
    * = encode(alloc, WavAudio {
        sample_rate: 0,
        channels: 1,
        format: SampleFormat.Pcm16,
        samples_f32: &valid_s,
    }, SampleFormat.Pcm16) catch e => {
        if (std.mem.eql(u8, @errorName(e), "InvalidSampleRate")) got_zero_sr = true;
    };
    expect_eq(got_zero_sr, true);
}

fn testCapWavDynamicMountAndZeroLeak(): !void {
    let gpa: std.heap.DebugAllocator(.{}) = .init;
    defer std.debug.assert(gpa.deinit() == .ok);
    const alloc = gpa.allocator();
    let samples: [4]f32 = [-0.5, 0.5, -0.25, 0.25];
    const audio = cap.wav.WavAudio {
        sample_rate: 44100,
        channels: 2,
        format: cap.wav.SampleFormat.Pcm24,
        samples_f32: &samples,
    };

    const bytes = cap.wav.encode(alloc, audio, cap.wav.SampleFormat.Pcm24)?;
    defer alloc.free(bytes);

    let decoded = cap.wav.decode(alloc, bytes)?;
    defer decoded.deinit(alloc);
    expect_eq(decoded.sample_rate, 44100);
    expect_eq(decoded.channels, 2);
    expect_eq(decoded.frameCount(), 2);

    let mono = cap.wav.toMono(alloc, decoded)?;
    defer mono.deinit(alloc);
    expect_eq(mono.channels, 1);
    expect_eq(mono.frameCount(), 2);
    expect_eq(approxF32(cap.wav.peakAmplitude(mono), 0.0, 0.0001), true);
}

fn main() {
    test("1. All 5 Sample Formats (Pcm8/Pcm16/Pcm24/Pcm32/Float32) Encode & Decode Roundtrip", () => {
        testAllFiveFormatsRoundtrip()?;
    });
    test("2. Skip Unknown Chunks (JUNK/LIST/fact) & Odd-Byte Chunk Padding Alignment", () => {
        testSkipUnknownAndOddChunks()?;
    });
    test("3. toMono Stereo Downmix, resampleLinear (48k->16k) & peakAmplitude/normalizePeak", () => {
        testMonoResampleAndPeakNormalize()?;
    });
    test("4. Invalid RIFF/WAVE Header, Missing data, TruncatedData, 0-Channel & 0-SampleRate", () => {
        testValidationAndErrorInterception()?;
    });
    test("5. import cap from \"cap\"; cap.wav Dynamic Mounting & std.testing.allocator Zero-Leak", () => {
        testCapWavDynamicMountAndZeroLeak()?;
    });
}
