// Source: cp/cap/test/test_audio_production_deep.cp
// Comprehensive Production-Grade Verification Suite for cap:audio & cap.audio

const std = @import("std");
import { test, expect_eq } from "cap:test";
import cap from "cap";
import {
    AudioError,
    WavAudio,
    WavFloatAudio,
    WavCodec,
    SpeechSegment,
    EnergyVad,
    AudioDsp,
} from "cap:audio";

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

fn testWavCodecRoundtripAndChunks(): !void {
    const alloc = std.heap.page_allocator;

    // 1. Lossless PCM16 roundtrip with negative, zero, and extreme values
    const pcm: [6]i16 = [-32768, -12345, -1, 0, 12345, 32767];
    const encoded = WavCodec.encodePcm16(alloc, &pcm, 16000, 1)?;
    defer alloc.free(encoded);
    expect_eq(encoded.len, 44 + 12);

    let decoded = WavCodec.decodePcm16(alloc, encoded)?;
    defer decoded.deinit();
    expect_eq(decoded.sample_rate, 16000);
    expect_eq(decoded.channels, 1);
    expect_eq(decoded.bits_per_sample, 16);
    expect_eq(decoded.samples.len, 6);
    for (i of 0..6) {
        expect_eq(decoded.samples[i], pcm[i]);
    }

    // Duration check: 16000 frames @ 16kHz stereo => 1000 ms
    let one_sec: [32000]i16 = [0; 32000];
    const wav_1s = WavCodec.encodePcm16(alloc, &one_sec, 16000, 2)?;
    defer alloc.free(wav_1s);
    let dec_1s = WavCodec.decodePcm16(alloc, wav_1s)?;
    defer dec_1s.deinit();
    expect_eq(dec_1s.durationMs(), 1000);

    // 2. Skip JUNK + odd-sized LIST chunk between fmt and data
    // Header (12) + fmt (24) + JUNK (8 + 4 = 12) + LIST (8 + 3 + 1 pad = 12) + data (8 + 4 = 12) = 72 bytes
    let custom: [72]u8 = [0; 72];
    // RIFF header
    custom[0] = 'R'; custom[1] = 'I'; custom[2] = 'F'; custom[3] = 'F';
    appendU32Le(&custom, 4, 64); // 72 - 8
    custom[8] = 'W'; custom[9] = 'A'; custom[10] = 'V'; custom[11] = 'E';
    // fmt chunk
    custom[12] = 'f'; custom[13] = 'm'; custom[14] = 't'; custom[15] = ' ';
    appendU32Le(&custom, 16, 16);
    appendU16Le(&custom, 20, 1);     // PCM
    appendU16Le(&custom, 22, 1);     // mono
    appendU32Le(&custom, 24, 8000);  // 8kHz
    appendU32Le(&custom, 28, 16000); // byte rate
    appendU16Le(&custom, 32, 2);     // block align
    appendU16Le(&custom, 34, 16);    // bits per sample
    // JUNK chunk (4 bytes payload)
    custom[36] = 'J'; custom[37] = 'U'; custom[38] = 'N'; custom[39] = 'K';
    appendU32Le(&custom, 40, 4);
    custom[44] = 0xAA; custom[45] = 0xBB; custom[46] = 0xCC; custom[47] = 0xDD;
    // LIST chunk (odd payload of 3 bytes + 1 RIFF padding byte)
    custom[48] = 'L'; custom[49] = 'I'; custom[50] = 'S'; custom[51] = 'T';
    appendU32Le(&custom, 52, 3);
    custom[56] = 1; custom[57] = 2; custom[58] = 3; custom[59] = 0;
    // data chunk (2 samples: 1024, -2048)
    custom[60] = 'd'; custom[61] = 'a'; custom[62] = 't'; custom[63] = 'a';
    appendU32Le(&custom, 64, 4);
    appendU16Le(&custom, 68, @as(u16, @bitCast(1024 as i16)));
    appendU16Le(&custom, 70, @as(u16, @bitCast(-2048 as i16)));

    let parsed_junk = WavCodec.decodePcm16(alloc, &custom)?;
    defer parsed_junk.deinit();
    expect_eq(parsed_junk.sample_rate, 8000);
    expect_eq(parsed_junk.samples.len, 2);
    expect_eq(parsed_junk.samples[0], 1024);
    expect_eq(parsed_junk.samples[1], -2048);

    // 3. Reject invalid header, truncated buffer, and non-PCM16 (Float32) format
    const bad_hdr: [16]u8 = ['N', 'O', 'P', 'E', 8, 0, 0, 0, 'W', 'A', 'V', 'E', 0, 0, 0, 0];
    let got_invalid_hdr = false;
    * = WavCodec.decodePcm16(alloc, &bad_hdr) catch e => {
        if (std.mem.eql(u8, @errorName(e), "InvalidHeader")) got_invalid_hdr = true;
    };
    expect_eq(got_invalid_hdr, true);

    let got_truncated = false;
    * = WavCodec.decodePcm16(alloc, encoded[0..encoded.len - 4]) catch e => {
        if (std.mem.eql(u8, @errorName(e), "TruncatedData")) got_truncated = true;
    };
    expect_eq(got_truncated, true);

    const f32_samples: [2]f32 = [0.25, -0.5];
    const f32_wav = WavCodec.encodeF32(alloc, &f32_samples, 16000, 1)?;
    defer alloc.free(f32_wav);

    let got_unsupported = false;
    * = WavCodec.decodePcm16(alloc, f32_wav) catch e => {
        if (std.mem.eql(u8, @errorName(e), "UnsupportedFormat")) got_unsupported = true;
    };
    expect_eq(got_unsupported, true);

    let dec_f32 = WavCodec.decodeF32(alloc, f32_wav)?;
    defer dec_f32.deinit();
    expect_eq(dec_f32.samples.len, 2);
    expect_eq(approxF32(dec_f32.samples[0], 0.25, 0.0001), true);
    expect_eq(approxF32(dec_f32.samples[1], -0.5, 0.0001), true);
}

fn testPcmConversionsAndStereoToMono(): !void {
    const alloc = std.heap.page_allocator;

    // 1. Saturation clamping in f32ToPcm16
    const floats: [6]f32 = [1.5, 1.0, 0.0, -1.0, -2.5, std.math.nan(f32)];
    const clamped = AudioDsp.f32ToPcm16(alloc, &floats)?;
    defer alloc.free(clamped);
    expect_eq(clamped[0], 32767);
    expect_eq(clamped[1], 32767);
    expect_eq(clamped[2], 0);
    expect_eq(clamped[3], -32768);
    expect_eq(clamped[4], -32768);
    expect_eq(clamped[5], 0);

    // 2. Roundtrip pcm16 -> f32 -> pcm16
    const orig: [5]i16 = [-32768, -16384, 0, 16384, 32767];
    const as_f32 = AudioDsp.pcm16ToF32(alloc, &orig)?;
    defer alloc.free(as_f32);
    expect_eq(approxF32(as_f32[0], -1.0, 0.00001), true);
    expect_eq(approxF32(as_f32[1], -0.5, 0.00001), true);
    expect_eq(approxF32(as_f32[2], 0.0, 0.00001), true);
    expect_eq(approxF32(as_f32[4], 1.0, 0.00001), true);

    const roundtrip = AudioDsp.f32ToPcm16(alloc, as_f32)?;
    defer alloc.free(roundtrip);
    for (i of 0..5) {
        expect_eq(roundtrip[i], orig[i]);
    }

    // 3. Stereo to Mono downmix without i16 overflow
    const stereo: [8]i16 = [1000, 3000, -32000, -32000, 32767, 32767, -500, 500];
    const mono = AudioDsp.stereoToMono(alloc, &stereo)?;
    defer alloc.free(mono);
    expect_eq(mono.len, 4);
    expect_eq(mono[0], 2000);
    expect_eq(mono[1], -32000);
    expect_eq(mono[2], 32767);
    expect_eq(mono[3], 0);
}

fn testSineWaveMetricsAndResample48kTo16k(): !void {
    const alloc = std.heap.page_allocator;

    // 1. 1s of 440Hz sine @ 16kHz, amplitude 0.5
    const sine16k = AudioDsp.generateSineWave(alloc, 440.0, 16000, 16000, 0.5)?;
    defer alloc.free(sine16k);

    const peak = AudioDsp.computePeak(sine16k);
    const rms = AudioDsp.computeRms(sine16k);
    const db = AudioDsp.computeRmsDb(sine16k);
    const zcr = AudioDsp.zeroCrossingRate(sine16k);

    const expected_rms: f64 = 0.5 / @sqrt(2.0);
    const expected_db: f64 = 20.0 * std.math.log10(expected_rms);
    const expected_zcr: f64 = (2.0 * 440.0) / 16000.0;

    expect_eq(approxF32(peak, 0.5, 0.001), true);
    expect_eq(approxF64(rms, expected_rms, 0.001), true);
    expect_eq(approxF64(db, expected_db, 0.05), true);
    expect_eq(approxF64(zcr, expected_zcr, 0.001), true);

    // Silence metering
    const silence: [100]f32 = [0.0; 100];
    expect_eq(approxF64(AudioDsp.computeRms(&silence), 0.0, 0.000001), true);
    expect_eq(approxF64(AudioDsp.computeRmsDb(&silence), -120.0, 0.000001), true);
    expect_eq(approxF32(AudioDsp.computePeak(&silence), 0.0, 0.000001), true);
    expect_eq(approxF64(AudioDsp.zeroCrossingRate(&silence), 0.0, 0.000001), true);

    // 2. 48kHz -> 16kHz linear resampling (Whisper ASR preprocessing)
    const sine48k = AudioDsp.generateSineWave(alloc, 440.0, 48000, 48000, 0.8)?;
    defer alloc.free(sine48k);
    const resampled = AudioDsp.resampleLinear(alloc, sine48k, 48000, 16000)?;
    defer alloc.free(resampled);

    expect_eq(resampled.len, 16000);
    const resampled_rms = AudioDsp.computeRms(resampled);
    const resampled_peak = AudioDsp.computePeak(resampled);
    expect_eq(approxF64(resampled_rms, 0.8 / @sqrt(2.0), 0.005), true);
    expect_eq(approxF32(resampled_peak, 0.8, 0.005), true);
}

fn testEnergyVadTwoSpeechSegments(): !void {
    const alloc = std.heap.page_allocator;

    // 60 frames x 160 samples = 9600 samples (600ms @ 16kHz):
    // [0..10): silence
    // [10..20): speech 1 (1600..3200)
    // [20..35): silence (15 frames > hangover 2)
    // [35..45): speech 2 (5600..7200)
    // [45..60): silence
    const frame_len: usize = 160;
    const total_samples: usize = frame_len * 60;
    let stream = alloc.alloc(f32, total_samples)?;
    defer alloc.free(stream);
    for (i of 0..total_samples) {
        stream[i] = 0.0;
    }

    const burst1 = AudioDsp.generateSineWave(alloc, 440.0, 16000, frame_len * 10, 0.4)?;
    defer alloc.free(burst1);
    @memcpy(stream[frame_len * 10 .. frame_len * 20], burst1);

    const burst2 = AudioDsp.generateSineWave(alloc, 660.0, 16000, frame_len * 10, 0.3)?;
    defer alloc.free(burst2);
    @memcpy(stream[frame_len * 35 .. frame_len * 45], burst2);

    const vad = EnergyVad.init(frame_len, 0.05, 2);
    const segments = vad.detectSegments(alloc, stream)?;
    defer alloc.free(segments);

    expect_eq(segments.len, 2);
    expect_eq(segments[0].start_sample, frame_len * 10);
    expect_eq(segments[0].end_sample, frame_len * 22);
    expect_eq(segments[1].start_sample, frame_len * 35);
    expect_eq(segments[1].end_sample, frame_len * 47);
    expect_eq(segments[0].end_sample < segments[1].start_sample, true);
    expect_eq(segments[0].lenSamples(), frame_len * 12);
}

fn testCapAudioLazyMount(): !void {
    const alloc = std.heap.page_allocator;

    // End-to-end TTS/ASR pipeline via `cap.audio.*` lazy namespace mounting
    const raw48k = cap.audio.AudioDsp.generateSineWave(alloc, 440.0, 48000, 24000, 0.6)?;
    defer alloc.free(raw48k);

    const asr16k = cap.audio.AudioDsp.resampleLinear(alloc, raw48k, 48000, 16000)?;
    defer alloc.free(asr16k);
    expect_eq(asr16k.len, 8000);

    const pcm = cap.audio.AudioDsp.f32ToPcm16(alloc, asr16k)?;
    defer alloc.free(pcm);

    const wav_bytes = cap.audio.WavCodec.encodePcm16(alloc, pcm, 16000, 1)?;
    defer alloc.free(wav_bytes);

    let decoded = cap.audio.WavCodec.decodePcm16(alloc, wav_bytes)?;
    defer decoded.deinit();
    expect_eq(decoded.durationMs(), 500);

    const f32_back = cap.audio.AudioDsp.pcm16ToF32(alloc, decoded.samples)?;
    defer alloc.free(f32_back);

    const vad = cap.audio.EnergyVad.init(160, 0.05, 1);
    const segs = vad.detectSegments(alloc, f32_back)?;
    defer alloc.free(segs);
    expect_eq(segs.len, 1);
    expect_eq(segs[0].start_sample, 0);
    expect_eq(segs[0].end_sample, 8000);
}

fn main() {
    test("1. WavCodec PCM16 & Float32 Roundtrip + JUNK/LIST Chunk Skip + Error Handling", () => {
        testWavCodecRoundtripAndChunks()?;
    });
    test("2. pcm16ToF32 / f32ToPcm16 Saturation Clamping & stereoToMono Downmix", () => {
        testPcmConversionsAndStereoToMono()?;
    });
    test("3. generateSineWave 440Hz RMS/Peak/ZCR & 48kHz->16kHz Linear Resampling", () => {
        testSineWaveMetricsAndResample48kTo16k()?;
    });
    test("4. EnergyVad Dual Speech Segment Detection in Silence-Speech-Silence-Speech-Silence Stream", () => {
        testEnergyVadTwoSpeechSegments()?;
    });
    test("5. import cap from \"cap\"; cap.audio Automatic Dynamic Lazy Mounting", () => {
        testCapAudioLazyMount()?;
    });
}
