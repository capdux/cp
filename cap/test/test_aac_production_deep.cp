// Source: cp/cap/test/test_aac_production_deep.cp (Production deep tests for cap:aac)

import std from "std";
import { test, expect_eq } from "cap:test";
import {
    ADTS_HEADER_SIZE,
    ADTS_HEADER_CRC_SIZE,
    AAC_SAMPLES_PER_BLOCK,
    AAC_SAMPLE_RATES,
    AacProfile,
    aacSampleRate,
    aacChannelCount,
    aacProfileFromObjectType,
    AdtsFrame,
    AudioSpecificConfig,
    AdtsStats,
    isAdts,
    AdtsIter,
    parseAudioSpecificConfig,
    summarizeAdts,
} from "cap:media";

fn writeAdtsFrame(
    buf: []u8,
    off: usize,
    is_mpeg2: bool,
    protection_absent: bool,
    profile_code: u8,
    sr_idx: u8,
    ch_cfg: u8,
    raw_blocks_minus_1: u8,
    crc_val: u16,
    payload_len: usize,
): usize {
    let hdr_size: usize = ADTS_HEADER_SIZE;
    if (!protection_absent) {
        hdr_size = ADTS_HEADER_CRC_SIZE;
    }
    const frame_len: usize = hdr_size + payload_len;

    buf[off] = 0xFF;
    let b1: u8 = 0xF0;
    if (is_mpeg2) {
        b1 = b1 | 0x08;
    }
    if (protection_absent) {
        b1 = b1 | 0x01;
    }
    buf[off + 1] = b1;

    buf[off + 2] = (((profile_code & 0x03) << 6) | ((sr_idx & 0x0F) << 2) | ((ch_cfg >> 2) & 0x01));
    buf[off + 3] = (((ch_cfg & 0x03) << 6) | (((frame_len >> 11) & 0x03) as u8));
    buf[off + 4] = ((frame_len >> 3) & 0xFF) as u8;
    buf[off + 5] = ((((frame_len & 0x07) as u8) << 5) | (0x1F as u8));
    buf[off + 6] = ((0xFC as u8) | (raw_blocks_minus_1 & 0x03));

    if (!protection_absent) {
        buf[off + 7] = ((crc_val >> 8) & 0xFF) as u8;
        buf[off + 8] = (crc_val & 0xFF) as u8;
    }

    let i: usize = 0;
    while (i < payload_len) {
        buf[off + hdr_size + i] = (0x30 + (i & 0x0F)) as u8;
        i += 1;
    }

    return frame_len;
}

test("1. AAC_SAMPLE_RATES, aacSampleRate, aacChannelCount, and AacProfile lookup tables", () => {
    expect_eq(AAC_SAMPLE_RATES.len, 13);
    expect_eq(AAC_SAMPLE_RATES[0], 96000);
    expect_eq(AAC_SAMPLE_RATES[3], 48000);
    expect_eq(AAC_SAMPLE_RATES[4], 44100);
    expect_eq(AAC_SAMPLE_RATES[12], 7350);

    expect_eq(aacSampleRate(0) ?? 0, 96000);
    expect_eq(aacSampleRate(3) ?? 0, 48000);
    expect_eq(aacSampleRate(4) ?? 0, 44100);
    expect_eq(aacSampleRate(12) ?? 0, 7350);
    expect_eq(aacSampleRate(13) == null, true);
    expect_eq(aacSampleRate(15) == null, true);

    expect_eq(aacChannelCount(0) == null, true);
    expect_eq(aacChannelCount(1) ?? 0, 1);
    expect_eq(aacChannelCount(2) ?? 0, 2);
    expect_eq(aacChannelCount(6) ?? 0, 6);
    expect_eq(aacChannelCount(7) ?? 0, 8);
    expect_eq(aacChannelCount(8) == null, true);

    expect_eq(aacProfileFromObjectType(1), AacProfile.main);
    expect_eq(aacProfileFromObjectType(2), AacProfile.lc);
    expect_eq(aacProfileFromObjectType(3), AacProfile.ssr);
    expect_eq(aacProfileFromObjectType(4), AacProfile.ltp);
    expect_eq(aacProfileFromObjectType(5), AacProfile.sbr);
    expect_eq(aacProfileFromObjectType(99), AacProfile.unknown);
    expect_eq(AacProfile.lc as u8, 2);
});

test("2. AdtsIter decodes mixed 7B no-CRC and 9B CRC-protected MPEG-4/MPEG-2 frames", () => {
    let stream: [64]u8 = std.mem.zeroes([64]u8);
    // Frame 0: MPEG-4, protection_absent=true (7B), AAC-LC (code 1 -> obj 2), 44.1kHz (idx 4), stereo (2), 1 block, 5B payload
    const f0_len = writeAdtsFrame(stream[0..], 0, false, true, 1, 4, 2, 0, 0, 5);
    // Frame 1: MPEG-2, protection_absent=false (9B), AAC-Main (code 0 -> obj 1), 48kHz (idx 3), 7.1 (cfg 7 -> 8ch), 2 blocks, 4B payload, CRC 0xBEEF
    const f1_len = writeAdtsFrame(stream[0..], f0_len, true, false, 0, 3, 7, 1, 0xBEEF, 4);
    const total_len = f0_len + f1_len;

    expect_eq(isAdts(stream[0..total_len]), true);

    let it = AdtsIter.init(stream[0..total_len]);

    const f0 = it.next() ?? unreachable;
    expect_eq(f0.is_mpeg2, false);
    expect_eq(f0.protection_absent, true);
    expect_eq(f0.object_type, 2);
    expect_eq(f0.profile, AacProfile.lc);
    expect_eq(f0.sample_rate_idx, 4);
    expect_eq(f0.sample_rate_hz, 44100);
    expect_eq(f0.channel_config, 2);
    expect_eq(f0.channels, 2);
    expect_eq(f0.frame_length, 12);
    expect_eq(f0.num_raw_data_blocks, 1);
    expect_eq(f0.samples(), AAC_SAMPLES_PER_BLOCK);
    expect_eq(f0.crc == null, true);
    expect_eq(f0.payload.len, 5);
    expect_eq(f0.payload[0], 0x30);
    expect_eq(f0.payload[4], 0x34);

    const f1 = it.next() ?? unreachable;
    expect_eq(f1.is_mpeg2, true);
    expect_eq(f1.protection_absent, false);
    expect_eq(f1.object_type, 1);
    expect_eq(f1.profile, AacProfile.main);
    expect_eq(f1.sample_rate_idx, 3);
    expect_eq(f1.sample_rate_hz, 48000);
    expect_eq(f1.channel_config, 7);
    expect_eq(f1.channels, 8);
    expect_eq(f1.frame_length, 13);
    expect_eq(f1.num_raw_data_blocks, 2);
    expect_eq(f1.samples(), 2048);
    expect_eq(f1.crc ?? 0, 0xBEEF);
    expect_eq(f1.payload.len, 4);
    expect_eq(f1.payload[0], 0x30);
    expect_eq(f1.payload[3], 0x33);

    expect_eq(it.next() == null, true);
});

test("3. parseAudioSpecificConfig handles standard 2B, 5-bit escape (31), and 24-bit explicit sample rate", () => {
    // Case A: Standard 2B AAC-LC (2), 44100Hz (idx 4), Stereo (2)
    // bits: 00010 (2) | 0100 (4) | 0010 (2) | 000 -> 0x12 0x10
    const asc_std: [2]u8 = [0x12, 0x10];
    const cfg_std = parseAudioSpecificConfig(asc_std[0..]) ?? unreachable;
    expect_eq(cfg_std.object_type, 2);
    expect_eq(cfg_std.profile, AacProfile.lc);
    expect_eq(cfg_std.sample_rate_idx, 4);
    expect_eq(cfg_std.sample_rate_hz, 44100);
    expect_eq(cfg_std.channel_config, 2);
    expect_eq(cfg_std.channels, 2);

    // Case B: Standard 2B HE-AAC SBR (5), 48000Hz (idx 3), 7.1 (cfg 7 -> 8ch)
    // bits: 00101 (5) | 0011 (3) | 0111 (7) | 000 -> 0x29 0xB8
    const asc_sbr: [2]u8 = [0x29, 0xB8];
    const cfg_sbr = parseAudioSpecificConfig(asc_sbr[0..]) ?? unreachable;
    expect_eq(cfg_sbr.object_type, 5);
    expect_eq(cfg_sbr.profile, AacProfile.sbr);
    expect_eq(cfg_sbr.sample_rate_hz, 48000);
    expect_eq(cfg_sbr.channel_config, 7);
    expect_eq(cfg_sbr.channels, 8);

    // Case C: 5-bit escape (31) + 6-bit ext (5 => object_type 37), 48kHz (idx 3), Mono (1)
    // bits: 11111 (5) | 000101 (6) | 0011 (4) | 0001 (4) | 00000 (5)
    // b0 = 11111_000 = 0xF8
    // b1 = 101_0011_0 = 0xA6
    // b2 = 001_00000 = 0x20
    const asc_esc: [3]u8 = [0xF8, 0xA6, 0x20];
    const cfg_esc = parseAudioSpecificConfig(asc_esc[0..]) ?? unreachable;
    expect_eq(cfg_esc.object_type, 37);
    expect_eq(cfg_esc.profile, AacProfile.unknown);
    expect_eq(cfg_esc.sample_rate_idx, 3);
    expect_eq(cfg_esc.sample_rate_hz, 48000);
    expect_eq(cfg_esc.channel_config, 1);
    expect_eq(cfg_esc.channels, 1);

    // Case D: Explicit 24-bit sample rate (freq_idx = 15), AAC-LC (2), 50000Hz (0x00C350), Stereo (2)
    // bits: 00010 (5) | 1111 (4) | 00000000_11000011_01010000 (24) | 0010 (4) | 000 (3)
    // b0 = 00010_111 = 0x17
    // b1 = 1_0000000 = 0x80
    // b2 = 0_1100001 = 0x61
    // b3 = 1_0101000 = 0xA8
    // b4 = 0_0010_000 = 0x10
    const asc_exp: [5]u8 = [0x17, 0x80, 0x61, 0xA8, 0x10];
    const cfg_exp = parseAudioSpecificConfig(asc_exp[0..]) ?? unreachable;
    expect_eq(cfg_exp.object_type, 2);
    expect_eq(cfg_exp.profile, AacProfile.lc);
    expect_eq(cfg_exp.sample_rate_idx, 15);
    expect_eq(cfg_exp.sample_rate_hz, 50000);
    expect_eq(cfg_exp.channel_config, 2);
    expect_eq(cfg_exp.channels, 2);

    // Invalid ASC cases: truncated, object_type 0, reserved freq_idx 13, reserved channel_config 9
    const asc_short: [1]u8 = [0x12];
    expect_eq(parseAudioSpecificConfig(asc_short[0..]) == null, true);
    const asc_obj0: [2]u8 = [0x02, 0x10];
    expect_eq(parseAudioSpecificConfig(asc_obj0[0..]) == null, true);
    const asc_bad_sr: [2]u8 = [0x16, 0x90]; // obj=2, sr_idx=13 (1101)
    expect_eq(parseAudioSpecificConfig(asc_bad_sr[0..]) == null, true);
    const asc_bad_ch: [2]u8 = [0x12, 0x48]; // obj=2, sr_idx=4, ch=9 (1001)
    expect_eq(parseAudioSpecificConfig(asc_bad_ch[0..]) == null, true);
});

test("4. summarizeAdts aggregates multi-frame stream samples, bytes, and duration_ms accurately", () => {
    // 5 frames @ 48000 Hz, 2ch, each with 1 raw_data_block (1024 samples) -> 5120 samples
    // duration_ms = (5120 * 1000) / 48000 = 106 ms
    let buf: [128]u8 = std.mem.zeroes([128]u8);
    let pos: usize = 0;
    let i: usize = 0;
    while (i < 5) {
        pos += writeAdtsFrame(buf[0..], pos, false, true, 1, 3, 2, 0, 0, 3);
        i += 1;
    }

    const stats = summarizeAdts(buf[0..pos]);
    expect_eq(stats.frame_count, 5);
    expect_eq(stats.total_samples, 5120);
    expect_eq(stats.sample_rate_hz, 48000);
    expect_eq(stats.channels, 2);
    expect_eq(stats.total_bytes, pos);
    expect_eq(stats.duration_ms, 106);

    // Empty input produces zeroed stats safely
    const empty_stats = summarizeAdts(buf[0..0]);
    expect_eq(empty_stats.frame_count, 0);
    expect_eq(empty_stats.total_samples, 0);
    expect_eq(empty_stats.duration_ms, 0);
});

test("5. Malformed ADTS protection: bad syncword, non-zero layer, invalid sr_idx, and out-of-bounds frame_length", () => {
    let valid: [16]u8 = std.mem.zeroes([16]u8);
    const vlen = writeAdtsFrame(valid[0..], 0, false, true, 1, 4, 2, 0, 0, 5);
    expect_eq(isAdts(valid[0..vlen]), true);

    // A: Bad syncword
    let bad_sync = valid;
    bad_sync[0] = 0xFE;
    expect_eq(isAdts(bad_sync[0..vlen]), false);

    // B: Non-zero layer (bits 2..1 of byte 1)
    let bad_layer = valid;
    bad_layer[1] = bad_layer[1] | 0x02;
    expect_eq(isAdts(bad_layer[0..vlen]), false);

    // C: Invalid sample rate index (13)
    let bad_sr = valid;
    bad_sr[2] = (bad_sr[2] & 0xC3) | (13 << 2);
    expect_eq(isAdts(bad_sr[0..vlen]), false);

    // D: Out-of-bounds frame_length (slice shorter than frame_length)
    expect_eq(isAdts(valid[0..(vlen - 1)]), false);

    // E: frame_length smaller than header size (e.g. 6 < 7)
    let tiny_len = valid;
    tiny_len[3] = tiny_len[3] & 0xFC;
    tiny_len[4] = 0;
    tiny_len[5] = ((6 << 5) | 0x1F) as u8;
    expect_eq(isAdts(tiny_len[0..vlen]), false);

    // F: 9B CRC frame with frame_length == 8 (< 9) rejected
    let crc_tiny: [16]u8 = std.mem.zeroes([16]u8);
    const clen = writeAdtsFrame(crc_tiny[0..], 0, false, false, 1, 4, 2, 0, 0x1234, 2);
    expect_eq(isAdts(crc_tiny[0..clen]), true);
    crc_tiny[3] = crc_tiny[3] & 0xFC;
    crc_tiny[4] = 1; // 1 << 3 = 8
    crc_tiny[5] = 0x1F;
    expect_eq(isAdts(crc_tiny[0..clen]), false);
});
