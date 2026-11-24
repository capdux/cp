// Origin: Pure-CP Stdlib (ISO/IEC 14496-3 AAC ADTS/ASC/ESDS/LOAS-LATM & RFC 3640 AAC-hbr RTP Engine) | Created: 2026-11-22
// 100% pure CP, zero dynamic heap allocation, zero external FFI dependencies, caller-slice AAC audio core:
//   - 100% backwards-compatible ADTS/ASC parser (`ADTS_HEADER_SIZE`, `AdtsFrame`, `AdtsIter`, `parseAudioSpecificConfig`, `summarizeAdts`)
//   - ISO/IEC 14496-3 ADTS 7-byte & 9-byte CRC16-IBM (poly 0x8005) encoder, verifier & noise-resilient resync iterator (`AdtsFrameIter`)
//   - Full `AudioSpecificConfig` (ASC) codec: extended AOT (>=32), explicit 24-bit sample rate, GASpecificConfig,
//     HE-AAC SBR/PS extension signaling, ADTS <-> ASC conversion, and MP4 `esds` FullBox encoder/decoder
//   - ISO/IEC 14496-3 §1.7.3 LOAS (`0x2B7`) sync iterator/encoder & LATM `AudioMuxElement` / `StreamMuxConfig` parser
//   - RFC 3640 MPEG4-GENERIC (`AAC-hbr`) multi-AU RTP packetizer, zero-copy depacketizer & SDP `a=fmtp` formatter

// ============================================================================
// 1. Constants, Lookup Tables & Error Set
// ============================================================================

pub const ADTS_HEADER_SIZE: usize = 7;
pub const ADTS_HEADER_CRC_SIZE: usize = 9;
pub const AAC_SAMPLES_PER_BLOCK: u64 = 1024;

/// ISO/IEC 14496-3 ADTS CRC-16/IBM generator polynomial (`0x8005`).
pub const AAC_CRC16_IBM_POLY: u16 = 0x8005;

/// ISO/IEC 14496-3 §1.7.3 LOAS `AudioSyncStream` 11-bit syncword (`0x2B7`).
pub const LOAS_SYNC_WORD: u16 = 0x2B7;

/// MP4 `esds` Elementary Stream Descriptor tags (ISO/IEC 14496-1).
pub const ESDS_TAG_ES_DESCR: u8 = 0x03;
pub const ESDS_TAG_DECODER_CONFIG: u8 = 0x04;
pub const ESDS_TAG_DECODER_SPECIFIC: u8 = 0x05;
pub const ESDS_TAG_SL_CONFIG: u8 = 0x06;

/// ISO/IEC 14496-3 standard 13-entry sampling frequency table (Hz).
pub const AAC_SAMPLE_RATES: [13]u32 = [
    96000,
    88200,
    64000,
    48000,
    44100,
    32000,
    24000,
    22050,
    16000,
    12000,
    11025,
    8000,
    7350,
];

/// Error set returned by AAC ADTS, ASC, MP4 `esds`, LOAS/LATM, and RFC 3640 RTP operations.
pub err AacError {
    BufferTooSmall,
    TruncatedData,
    InvalidAdtsSync,
    InvalidAdtsHeader,
    CrcMismatch,
    InvalidSampleRate,
    InvalidChannelConfig,
    InvalidObjectType,
    InvalidAsc,
    InvalidEsds,
    InvalidLoasSync,
    InvalidLatm,
    InvalidRtpPacket,
    MtuTooSmall,
    SpanOverflow,
    InvalidParameter,
}

pub enum AacProfile: u8 {
    unknown = 0,
    main = 1,
    lc = 2,
    ssr = 3,
    ltp = 4,
    sbr = 5,
}

pub fn aacSampleRate(idx: u8): u32? {
    if (idx < 13) {
        return AAC_SAMPLE_RATES[idx as usize];
    }
    return null;
}

/// Reverse lookup: returns the ISO/IEC 14496-3 sampling frequency index (`0..12`) for `hz`, or `null`.
pub fn aacSampleRateIndex(hz: u32): u8? {
    let i: usize = 0;
    while (i < 13) : (i += 1) {
        if (AAC_SAMPLE_RATES[i] == hz) {
            return i as u8;
        }
    }
    return null;
}

pub fn aacChannelCount(cfg: u8): u8? {
    if (cfg >= 1 && cfg <= 6) {
        return cfg;
    }
    if (cfg == 7) {
        return 8;
    }
    return null;
}

pub fn aacProfileFromObjectType(object_type: u8): AacProfile {
    if (object_type == 1) return AacProfile.main;
    if (object_type == 2) return AacProfile.lc;
    if (object_type == 3) return AacProfile.ssr;
    if (object_type == 4) return AacProfile.ltp;
    if (object_type == 5) return AacProfile.sbr;
    return AacProfile.unknown;
}

// ============================================================================
// 2. Backwards-Compatible Legacy Types & Functions (`AdtsFrame`, `AdtsIter`, `AudioSpecificConfig`)
// ============================================================================

pub struct AdtsFrame {
    is_mpeg2: bool,
    protection_absent: bool,
    object_type: u8,
    profile: AacProfile,
    sample_rate_idx: u8,
    sample_rate_hz: u32,
    channel_config: u8,
    channels: u8,
    frame_length: u16,
    num_raw_data_blocks: u8,
    crc: u16?,
    payload: []const u8,

    fn samples(): u64 {
        return (_.num_raw_data_blocks as u64) * AAC_SAMPLES_PER_BLOCK;
    }
}

pub struct AudioSpecificConfig {
    object_type: u8,
    profile: AacProfile,
    sample_rate_idx: u8,
    sample_rate_hz: u32,
    channel_config: u8,
    channels: u8,
}

pub struct AdtsStats {
    frame_count: u64,
    total_samples: u64,
    sample_rate_hz: u32,
    channels: u8,
    total_bytes: usize,
    duration_ms: u64,
}

fn parseAdtsFrameAt(data: []const u8, pos: usize): AdtsFrame? {
    if (pos > data.len || data.len - pos < ADTS_HEADER_SIZE) {
        return null;
    }

    const b0 = data[pos];
    const b1 = data[pos + 1];
    const b2 = data[pos + 2];
    const b3 = data[pos + 3];
    const b4 = data[pos + 4];
    const b5 = data[pos + 5];
    const b6 = data[pos + 6];

    // 12-bit syncword 0xFFF
    if (b0 != 0xFF || (b1 & 0xF0) != 0xF0) {
        return null;
    }

    // 2-bit layer must be 00
    if ((b1 & 0x06) != 0) {
        return null;
    }

    const is_mpeg2 = (b1 & 0x08) != 0;
    const protection_absent = (b1 & 0x01) != 0;

    let header_size: usize = ADTS_HEADER_SIZE;
    if (!protection_absent) {
        header_size = ADTS_HEADER_CRC_SIZE;
    }

    const sample_rate_idx: u8 = (b2 >> 2) & 0x0F;
    const sample_rate_hz = aacSampleRate(sample_rate_idx) ?? return null;

    const frame_length_u16: u16 = (((b3 & 0x03) as u16) << 11) |
        ((b4 as u16) << 3) |
        (((b5 >> 5) & 0x07) as u16);
    const frame_length: usize = frame_length_u16 as usize;

    if (frame_length < header_size || frame_length > data.len - pos) {
        return null;
    }

    // ADTS 2-bit profile_ObjectType stores (audioObjectType - 1)
    const object_type: u8 = ((b2 >> 6) & 0x03) + 1;
    const profile = aacProfileFromObjectType(object_type);

    const channel_config: u8 = (((b2 & 0x01) << 2) | ((b3 >> 6) & 0x03)) as u8;
    const channels: u8 = aacChannelCount(channel_config) ?? 0;

    const num_raw_data_blocks: u8 = (b6 & 0x03) + 1;

    let crc: u16? = null;
    if (!protection_absent) {
        const c0 = data[pos + 7];
        const c1 = data[pos + 8];
        crc = ((c0 as u16) << 8) | (c1 as u16);
    }

    const payload_start = pos + header_size;
    const payload_end = pos + frame_length;

    return AdtsFrame{
        .is_mpeg2 = is_mpeg2,
        .protection_absent = protection_absent,
        .object_type = object_type,
        .profile = profile,
        .sample_rate_idx = sample_rate_idx,
        .sample_rate_hz = sample_rate_hz,
        .channel_config = channel_config,
        .channels = channels,
        .frame_length = frame_length_u16,
        .num_raw_data_blocks = num_raw_data_blocks,
        .crc = crc,
        .payload = data[payload_start..payload_end],
    };
}

pub fn isAdts(data: []const u8): bool {
    if (_frame := parseAdtsFrameAt(data, 0)) {
        return true;
    }
    return false;
}

pub struct AdtsIter {
    data: []const u8,
    pos: usize,
    done: bool,

    static fn init(data: []const u8): AdtsIter {
        return AdtsIter{
            .data = data,
            .pos = 0,
            .done = false,
        };
    }

    fn next(*_): AdtsFrame? {
        if (_.done) {
            return null;
        }
        if (frame := parseAdtsFrameAt(_.data, _.pos)) {
            _.pos += frame.frame_length as usize;
            return frame;
        }
        _.done = true;
        return null;
    }
}

struct BitReader {
    data: []const u8,
    bit_pos: usize,

    fn bitsRemaining(): usize {
        const total: usize = _.data.len * 8;
        if (_.bit_pos >= total) return 0;
        return total - _.bit_pos;
    }

    fn readBits(*_, count: u8): u32? {
        if (count == 0 || count > 24) {
            return null;
        }
        const total_bits: usize = _.data.len * 8;
        const need: usize = count as usize;
        if (_.bit_pos > total_bits || need > total_bits - _.bit_pos) {
            return null;
        }

        let acc: u32 = 0;
        let i: u8 = 0;
        while (i < count) {
            const byte_idx = _.bit_pos >> 3;
            const shift: u3 = (7 - (_.bit_pos & 7)) as u3;
            const bit: u32 = ((_.data[byte_idx] >> shift) & 0x01) as u32;
            acc = (acc << 1) | bit;
            _.bit_pos += 1;
            i += 1;
        }
        return acc;
    }
}

struct BitWriter {
    buf: []u8 = [];
    bit_pos: usize = 0;

    static fn init(out: []u8): BitWriter {
        let i: usize = 0;
        while (i < out.len) : (i += 1) {
            out[i] = 0;
        }
        return BitWriter{
            buf: out,
            bit_pos: 0,
        };
    }

    fn writeBits(*_, value: u32, count: u8): bool ! AacError {
        if (count == 0) return true;
        if (count > 32) return AacError.InvalidParameter;
        const need_bytes: usize = (bit_pos + (count as usize) + 7) >> 3;
        if (need_bytes > buf.len) return AacError.BufferTooSmall;

        let rem: u8 = count;
        while (rem > 0) {
            rem -= 1;
            const bit: u8 = ((value >> (@as(u5, @intCast(rem)))) & 1) as u8;
            if (bit != 0) {
                const byte_idx: usize = bit_pos >> 3;
                const shift: u3 = @as(u3, @intCast(7 - (bit_pos & 7)));
                buf[byte_idx] |= (@as(u8, 1) << shift);
            }
            bit_pos += 1;
        }
        return true;
    }

    fn byteLen(): usize {
        return (bit_pos + 7) >> 3;
    }
}

pub fn parseAudioSpecificConfig(data: []const u8): AudioSpecificConfig? {
    if (data.len < 2) {
        return null;
    }

    let br = BitReader{
        .data = data,
        .bit_pos = 0,
    };

    const raw_obj = br.readBits(5) ?? return null;
    let object_type: u8 = raw_obj as u8;
    if (object_type == 31) {
        const ext = br.readBits(6) ?? return null;
        object_type = (32 + ext) as u8;
    }
    if (object_type == 0) {
        return null;
    }

    const freq_idx_u32 = br.readBits(4) ?? return null;
    const sample_rate_idx: u8 = freq_idx_u32 as u8;
    let sample_rate_hz: u32 = 0;
    if (sample_rate_idx == 15) {
        const explicit_sr = br.readBits(24) ?? return null;
        if (explicit_sr == 0) {
            return null;
        }
        sample_rate_hz = explicit_sr;
    } else {
        sample_rate_hz = aacSampleRate(sample_rate_idx) ?? return null;
    }

    const ch_cfg_u32 = br.readBits(4) ?? return null;
    const channel_config: u8 = ch_cfg_u32 as u8;
    if (channel_config > 7) {
        return null;
    }
    const channels: u8 = aacChannelCount(channel_config) ?? 0;

    return AudioSpecificConfig{
        .object_type = object_type,
        .profile = aacProfileFromObjectType(object_type),
        .sample_rate_idx = sample_rate_idx,
        .sample_rate_hz = sample_rate_hz,
        .channel_config = channel_config,
        .channels = channels,
    };
}

pub fn summarizeAdts(data: []const u8): AdtsStats {
    let it = AdtsIter.init(data);
    let frame_count: u64 = 0;
    let total_samples: u64 = 0;
    let sample_rate_hz: u32 = 0;
    let channels: u8 = 0;
    let total_bytes: usize = 0;

    while (frame := it.next()) {
        if (frame_count == 0) {
            sample_rate_hz = frame.sample_rate_hz;
            channels = frame.channels;
        }
        frame_count += 1;
        total_samples += frame.samples();
        total_bytes += frame.frame_length as usize;
    }

    let duration_ms: u64 = 0;
    if (sample_rate_hz > 0) {
        const sr: u64 = sample_rate_hz as u64;
        const secs: u64 = total_samples / sr;
        const rem: u64 = total_samples % sr;
        duration_ms = secs * (1000 as u64) + (rem * (1000 as u64)) / sr;
    }

    return AdtsStats{
        .frame_count = frame_count,
        .total_samples = total_samples,
        .sample_rate_hz = sample_rate_hz,
        .channels = channels,
        .total_bytes = total_bytes,
        .duration_ms = duration_ms,
    };
}

// ============================================================================
// 3. ADTS CRC16-IBM (0x8005), Resync Scanner (`AdtsFrameIter`) & ADTS Encoders
// ============================================================================

fn copyBytes(dst: []u8, dst_off: usize, src: []const u8) {
    let i: usize = 0;
    while (i < src.len) : (i += 1) {
        dst[dst_off + i] = src[i];
    }
}

fn crc16IbmUpdate(init_crc: u16, data: []const u8): u16 {
    let crc: u16 = init_crc;
    let i: usize = 0;
    while (i < data.len) : (i += 1) {
        crc ^= (data[i] as u16) << 8;
        let b: usize = 0;
        while (b < 8) : (b += 1) {
            if ((crc & 0x8000) != 0) {
                crc = (crc << 1) ^ AAC_CRC16_IBM_POLY;
            } else {
                crc = crc << 1;
            }
        }
    }
    return crc;
}

/// Computes ISO/IEC 14496-3 ADTS CRC-16/IBM (polynomial `0x8005`, init `0xFFFF`)
/// over the 7-byte ADTS header and raw AAC payload.
pub fn computeAdtsCrc16(header_7: []const u8, payload: []const u8): u16 {
    const h_len: usize = if (header_7.len < 7) header_7.len else 7;
    let crc: u16 = crc16IbmUpdate(0xFFFF, header_7[0..h_len]);
    crc = crc16IbmUpdate(crc, payload);
    return crc;
}

/// Verifies the embedded 16-bit CRC of a 9-byte-header ADTS frame (`protection_absent == false`).
pub fn verifyAdtsFrameCrc(raw_frame: []const u8): bool ! AacError {
    const opt = parseAdtsFrameAt(raw_frame, 0);
    const frame = opt orelse return AacError.InvalidAdtsHeader;
    if (frame.protection_absent) {
        return true;
    }
    const stored: u16 = frame.crc orelse return AacError.InvalidAdtsHeader;
    const computed: u16 = computeAdtsCrc16(raw_frame[0..7], frame.payload);
    return stored == computed;
}

/// Configuration for encoding an ADTS frame header.
pub struct AdtsEncodeConfig {
    is_mpeg2: bool = false;
    object_type: u8 = 2; // AAC-LC
    sample_rate_hz: u32 = 48000;
    channel_config: u8 = 2;
    num_raw_data_blocks: u8 = 1;
    buffer_fullness: u16 = 0x7FF; // VBR
}

fn writeAdtsHeader7(
    cfg: AdtsEncodeConfig,
    protection_absent: bool,
    total_frame_len: usize,
    out: []u8,
): usize ! AacError {
    if (cfg.object_type < 1 || cfg.object_type > 4) return AacError.InvalidObjectType;
    const sr_idx = aacSampleRateIndex(cfg.sample_rate_hz) orelse return AacError.InvalidSampleRate;
    if (cfg.channel_config > 7) return AacError.InvalidChannelConfig;
    if (cfg.num_raw_data_blocks < 1 || cfg.num_raw_data_blocks > 4) return AacError.InvalidParameter;
    if (total_frame_len > 8191) return AacError.InvalidAdtsHeader;
    if (out.len < 7) return AacError.BufferTooSmall;

    const prof_code: u8 = cfg.object_type - 1;
    const flen: u16 = total_frame_len as u16;
    const bf: u16 = cfg.buffer_fullness & 0x7FF;

    out[0] = 0xFF;
    let b1: u8 = 0xF0;
    if (cfg.is_mpeg2) b1 |= 0x08;
    if (protection_absent) b1 |= 0x01;
    out[1] = b1;

    out[2] = ((prof_code & 0x03) << 6)
        | ((sr_idx & 0x0F) << 2)
        | ((cfg.channel_config >> 2) & 0x01);
    out[3] = ((cfg.channel_config & 0x03) << 6) | (((flen >> 11) & 0x03) as u8);
    out[4] = ((flen >> 3) & 0xFF) as u8;
    out[5] = (((flen & 0x07) as u8) << 5) | (((bf >> 6) & 0x1F) as u8);
    out[6] = (((bf & 0x3F) as u8) << 2) | ((cfg.num_raw_data_blocks - 1) & 0x03);
    return 7;
}

/// Encodes a Raw AAC Access Unit into a 7-byte-header ADTS frame (`protection_absent = true`).
pub fn encodeAdtsFrame(cfg: AdtsEncodeConfig, payload: []const u8, out: []u8): usize ! AacError {
    const total_len: usize = payload.len + ADTS_HEADER_SIZE;
    if (out.len < total_len) return AacError.BufferTooSmall;
    * = writeAdtsHeader7(cfg, true, total_len, out[0..7])?;
    copyBytes(out, ADTS_HEADER_SIZE, payload);
    return total_len;
}

/// Encodes a Raw AAC Access Unit into a 9-byte-header CRC16-protected ADTS frame (`protection_absent = false`).
pub fn encodeAdtsFrameWithCrc(cfg: AdtsEncodeConfig, payload: []const u8, out: []u8): usize ! AacError {
    const total_len: usize = payload.len + ADTS_HEADER_CRC_SIZE;
    if (out.len < total_len) return AacError.BufferTooSmall;
    * = writeAdtsHeader7(cfg, false, total_len, out[0..7])?;
    const crc: u16 = computeAdtsCrc16(out[0..7], payload);
    out[7] = ((crc >> 8) & 0xFF) as u8;
    out[8] = (crc & 0xFF) as u8;
    copyBytes(out, ADTS_HEADER_CRC_SIZE, payload);
    return total_len;
}

/// Noise-resilient ADTS frame scanner: skips leading garbage bytes, validates `0xFFF` syncword,
/// frame length, and lookahead syncword at the next frame boundary (when trailing bytes remain).
pub struct AdtsFrameIter {
    data: []const u8 = "";
    pos: usize = 0;
    bytes_skipped: usize = 0;
    frames_yielded: u64 = 0;

    static fn init(buf: []const u8): AdtsFrameIter {
        return AdtsFrameIter{
            data: buf,
            pos: 0,
            bytes_skipped: 0,
            frames_yielded: 0,
        };
    }

    fn next(*_): AdtsFrame? {
        while (pos + ADTS_HEADER_SIZE <= data.len) {
            if (frame := parseAdtsFrameAt(data, pos)) {
                const next_pos: usize = pos + (frame.frame_length as usize);
                // Lookahead sync check when additional stream data follows
                if (next_pos == data.len || (next_pos + 2 <= data.len && data[next_pos] == 0xFF && (data[next_pos + 1] & 0xF0) == 0xF0)) {
                    pos = next_pos;
                    frames_yielded += 1;
                    return frame;
                }
            }
            pos += 1;
            bytes_skipped += 1;
        }
        return null;
    }
}

// ============================================================================
// 4. Full ISO/IEC 14496-3 AudioSpecificConfig (ASC), ADTS<->ASC & MP4 `esds`
// ============================================================================

/// Full ISO/IEC 14496-3 `AudioSpecificConfig` including `GASpecificConfig` and HE-AAC SBR/PS extensions.
pub struct AacAudioSpecificConfig {
    object_type: u8 = 2;
    sample_rate_idx: u8 = 3;
    sample_rate_hz: u32 = 48000;
    channel_config: u8 = 2;
    channels: u8 = 2;
    frame_length_960: bool = false;
    depends_on_core_coder: bool = false;
    extension_flag: bool = false;
    sbr_present: bool = false;
    ps_present: bool = false;
    ext_object_type: u8 = 0;
    ext_sample_rate_idx: u8 = 0;
    ext_sample_rate_hz: u32 = 0;
}

/// Encodes an `AacAudioSpecificConfig` (standard 2B, explicit 24-bit sample rate, extended AOT>=32,
/// and optional explicit HE-AAC SBR `0x2B7` / PS `0x548` extensions) into `out`.
pub fn encodeAudioSpecificConfig(cfg: AacAudioSpecificConfig, out: []u8): usize ! AacError {
    if (cfg.object_type == 0) return AacError.InvalidObjectType;
    if (cfg.channel_config > 7) return AacError.InvalidChannelConfig;

    let bw = BitWriter.init(out);
    if (cfg.object_type < 31) {
        * = bw.writeBits(cfg.object_type as u32, 5)?;
    } else {
        * = bw.writeBits(31, 5)?;
        * = bw.writeBits((cfg.object_type - 32) as u32, 6)?;
    }

    if (idx := aacSampleRateIndex(cfg.sample_rate_hz)) {
        * = bw.writeBits(idx as u32, 4)?;
    } else {
        if (cfg.sample_rate_hz == 0 || cfg.sample_rate_hz > 0xFFFFFF) return AacError.InvalidSampleRate;
        * = bw.writeBits(15, 4)?;
        * = bw.writeBits(cfg.sample_rate_hz, 24)?;
    }

    * = bw.writeBits(cfg.channel_config as u32, 4)?;

    // GASpecificConfig (3 bits: frameLengthFlag, dependsOnCoreCoder, extensionFlag)
    * = bw.writeBits(if (cfg.frame_length_960) 1 else 0, 1)?;
    * = bw.writeBits(if (cfg.depends_on_core_coder) 1 else 0, 1)?;
    * = bw.writeBits(if (cfg.extension_flag) 1 else 0, 1)?;

    // Optional explicit HE-AAC SBR / PS signaling (ISO/IEC 14496-3 §1.6.2.1)
    if (cfg.sbr_present) {
        * = bw.writeBits(0x2B7, 11)?; // syncExtensionType
        * = bw.writeBits(5, 5)?;      // extAudioObjectType = SBR (5)
        * = bw.writeBits(1, 1)?;      // sbrPresentFlag = 1
        const ext_hz: u32 = if (cfg.ext_sample_rate_hz != 0) cfg.ext_sample_rate_hz else (cfg.sample_rate_hz * 2);
        if (eidx := aacSampleRateIndex(ext_hz)) {
            * = bw.writeBits(eidx as u32, 4)?;
        } else {
            * = bw.writeBits(15, 4)?;
            * = bw.writeBits(ext_hz, 24)?;
        }
        if (cfg.ps_present) {
            * = bw.writeBits(0x548, 11)?; // PS syncExtensionType
            * = bw.writeBits(1, 1)?;      // psPresentFlag = 1
        }
    }
    return bw.byteLen();
}

/// Decodes a full ISO/IEC 14496-3 `AudioSpecificConfig` from `data` (including SBR/PS extensions).
pub fn decodeAudioSpecificConfig(data: []const u8): AacAudioSpecificConfig ! AacError {
    if (data.len < 2) return AacError.InvalidAsc;
    let br = BitReader{ .data = data, .bit_pos = 0 };

    const raw_aot = br.readBits(5) orelse return AacError.InvalidAsc;
    let aot: u8 = raw_aot as u8;
    if (aot == 31) {
        const ext = br.readBits(6) orelse return AacError.InvalidAsc;
        aot = (32 + ext) as u8;
    }
    if (aot == 0) return AacError.InvalidObjectType;

    const sr_idx: u8 = (br.readBits(4) orelse return AacError.InvalidAsc) as u8;
    let sr_hz: u32 = 0;
    if (sr_idx == 15) {
        sr_hz = br.readBits(24) orelse return AacError.InvalidSampleRate;
        if (sr_hz == 0) return AacError.InvalidSampleRate;
    } else {
        sr_hz = aacSampleRate(sr_idx) orelse return AacError.InvalidSampleRate;
    }

    const ch_cfg: u8 = (br.readBits(4) orelse return AacError.InvalidChannelConfig) as u8;
    if (ch_cfg > 7) return AacError.InvalidChannelConfig;
    const ch_cnt: u8 = aacChannelCount(ch_cfg) ?? 0;

    let asc = AacAudioSpecificConfig{
        object_type: aot,
        sample_rate_idx: sr_idx,
        sample_rate_hz: sr_hz,
        channel_config: ch_cfg,
        channels: ch_cnt,
    };

    // Parse GASpecificConfig (3 bits) when present
    if (br.bitsRemaining() >= 3) {
        asc.frame_length_960 = (br.readBits(1) orelse 0) != 0;
        asc.depends_on_core_coder = (br.readBits(1) orelse 0) != 0;
        asc.extension_flag = (br.readBits(1) orelse 0) != 0;
    }

    // Check for explicit HE-AAC SBR syncExtensionType (0x2B7)
    if (br.bitsRemaining() >= 16) {
        const sync_ext = br.readBits(11) orelse 0;
        if (sync_ext == 0x2B7) {
            const ext_aot = (br.readBits(5) orelse 0) as u8;
            asc.ext_object_type = ext_aot;
            if (ext_aot == 5 && br.bitsRemaining() >= 5) {
                const sbr_flag = (br.readBits(1) orelse 0) != 0;
                asc.sbr_present = sbr_flag;
                if (sbr_flag) {
                    const esr_idx = (br.readBits(4) orelse 0) as u8;
                    asc.ext_sample_rate_idx = esr_idx;
                    if (esr_idx == 15) {
                        asc.ext_sample_rate_hz = br.readBits(24) orelse 0;
                    } else {
                        asc.ext_sample_rate_hz = aacSampleRate(esr_idx) ?? 0;
                    }
                    if (br.bitsRemaining() >= 12) {
                        const ps_sync = br.readBits(11) orelse 0;
                        if (ps_sync == 0x548) {
                            asc.ps_present = (br.readBits(1) orelse 0) != 0;
                        }
                    }
                }
            }
        }
    }
    return asc;
}

/// Converts a parsed `AdtsFrame` header into an equivalent `AacAudioSpecificConfig`.
pub fn adtsToAudioSpecificConfig(frame: AdtsFrame): AacAudioSpecificConfig ! AacError {
    if (frame.object_type == 0) return AacError.InvalidObjectType;
    if (frame.sample_rate_hz == 0) return AacError.InvalidSampleRate;
    return AacAudioSpecificConfig{
        object_type: frame.object_type,
        sample_rate_idx: frame.sample_rate_idx,
        sample_rate_hz: frame.sample_rate_hz,
        channel_config: frame.channel_config,
        channels: frame.channels,
        frame_length_960: false,
        depends_on_core_coder: false,
        extension_flag: false,
    };
}

/// Builds a 7-byte (or 9-byte zero-CRC placeholder) ADTS header from an `AacAudioSpecificConfig`.
pub fn audioSpecificConfigToAdtsHeader(
    cfg: AacAudioSpecificConfig,
    payload_len: usize,
    with_crc: bool,
    out: []u8,
): usize ! AacError {
    const hdr_len: usize = if (with_crc) ADTS_HEADER_CRC_SIZE else ADTS_HEADER_SIZE;
    if (out.len < hdr_len) return AacError.BufferTooSmall;
    const enc_cfg = AdtsEncodeConfig{
        is_mpeg2: false,
        object_type: cfg.object_type,
        sample_rate_hz: cfg.sample_rate_hz,
        channel_config: cfg.channel_config,
        num_raw_data_blocks: 1,
        buffer_fullness: 0x7FF,
    };
    * = writeAdtsHeader7(enc_cfg, !with_crc, payload_len + hdr_len, out[0..7])?;
    if (with_crc) {
        out[7] = 0;
        out[8] = 0;
    }
    return hdr_len;
}

// --- MP4 `esds` FullBox Codec (ISO/IEC 14496-14) ---

fn writeBeU32(out: []u8, off: usize, v: u32) {
    out[off] = ((v >> 24) & 0xFF) as u8;
    out[off + 1] = ((v >> 16) & 0xFF) as u8;
    out[off + 2] = ((v >> 8) & 0xFF) as u8;
    out[off + 3] = (v & 0xFF) as u8;
}

fn readBeU32(buf: []const u8, off: usize): u32 {
    return ((buf[off] as u32) << 24)
        | ((buf[off + 1] as u32) << 16)
        | ((buf[off + 2] as u32) << 8)
        | (buf[off + 3] as u32);
}

fn encodeDescrLen(len: usize, out: []u8, off: usize): usize ! AacError {
    if (len < 128) {
        if (off + 1 > out.len) return AacError.BufferTooSmall;
        out[off] = len as u8;
        return 1;
    }
    if (len < 16384) {
        if (off + 2 > out.len) return AacError.BufferTooSmall;
        out[off] = (((len >> 7) & 0x7F) as u8) | 0x80;
        out[off + 1] = (len & 0x7F) as u8;
        return 2;
    }
    return AacError.InvalidEsds;
}

struct DescrLenDecoded {
    value: usize = 0;
    bytes: usize = 0;
}

fn decodeDescrLen(buf: []const u8, off: usize): DescrLenDecoded ! AacError {
    let val: usize = 0;
    let i: usize = 0;
    while (i < 4) : (i += 1) {
        if (off + i >= buf.len) return AacError.TruncatedData;
        const b: u8 = buf[off + i];
        val = (val << 7) | ((b & 0x7F) as usize);
        if ((b & 0x80) == 0) {
            return DescrLenDecoded{ value: val, bytes: i + 1 };
        }
    }
    return AacError.InvalidEsds;
}

/// Decoded MP4 `esds` (Elementary Stream Descriptor) box metadata and embedded `AudioSpecificConfig`.
pub struct Mp4EsdsInfo {
    es_id: u16 = 0;
    object_type_indication: u8 = 0;
    stream_type: u8 = 0;
    buffer_size_db: u32 = 0;
    max_bitrate: u32 = 0;
    avg_bitrate: u32 = 0;
    asc_raw: []const u8 = "";
    asc: AacAudioSpecificConfig = AacAudioSpecificConfig{};
}

/// Encodes an ISO/IEC 14496-14 MP4 `esds` FullBox wrapping `asc_bytes` for direct use in `mp4a` / `fmp4Mux`.
pub fn encodeMp4EsdsBox(
    asc_bytes: []const u8,
    max_bitrate: u32,
    avg_bitrate: u32,
    es_id: u16,
    out: []u8,
): usize ! AacError {
    if (asc_bytes.len < 2 || asc_bytes.len > 127) return AacError.InvalidAsc;
    // Tag 0x05: 1 + 1 + asc_bytes.len
    const dsi_total: usize = asc_bytes.len + 2;
    // Tag 0x04 body: 13 bytes fixed + dsi_total
    const dec_cfg_body: usize = dsi_total + 13;
    const dec_cfg_len_bytes: usize = if (dec_cfg_body < 128) 1 else 2;
    const dec_cfg_total: usize = dec_cfg_body + dec_cfg_len_bytes + 1;
    // Tag 0x06 SLConfig: 3 bytes (0x06, 0x01, 0x02)
    const sl_total: usize = 3;
    // Tag 0x03 body: 3 bytes (es_id + flags) + dec_cfg_total + sl_total
    const es_body: usize = dec_cfg_total + sl_total + 3;
    const es_len_bytes: usize = if (es_body < 128) 1 else 2;
    const box_size: usize = es_body + es_len_bytes + 13; // 12B FullBox hdr + 1B tag
    if (out.len < box_size) return AacError.BufferTooSmall;

    // FullBox header: size(4) + "esds"(4) + version/flags(4)
    writeBeU32(out, 0, box_size as u32);
    out[4] = 'e';
    out[5] = 's';
    out[6] = 'd';
    out[7] = 's';
    writeBeU32(out, 8, 0);

    let pos: usize = 12;
    // Tag 0x03 ES_Descriptor
    out[pos] = ESDS_TAG_ES_DESCR;
    pos += 1;
    pos += encodeDescrLen(es_body, out, pos)?;
    out[pos] = ((es_id >> 8) & 0xFF) as u8;
    out[pos + 1] = (es_id & 0xFF) as u8;
    out[pos + 2] = 0; // flags = 0
    pos += 3;

    // Tag 0x04 DecoderConfigDescriptor
    out[pos] = ESDS_TAG_DECODER_CONFIG;
    pos += 1;
    pos += encodeDescrLen(dec_cfg_body, out, pos)?;
    out[pos] = 0x40; // objectTypeIndication = MPEG-4 Audio
    out[pos + 1] = (0x05 << 2) | 0x01; // streamType = AudioStream (5), reserved = 1
    out[pos + 2] = 0;
    out[pos + 3] = 0;
    out[pos + 4] = 0; // bufferSizeDB
    writeBeU32(out, pos + 5, max_bitrate);
    writeBeU32(out, pos + 9, avg_bitrate);
    pos += 13;

    // Tag 0x05 DecoderSpecificInfo (AudioSpecificConfig)
    out[pos] = ESDS_TAG_DECODER_SPECIFIC;
    pos += 1;
    pos += encodeDescrLen(asc_bytes.len, out, pos)?;
    copyBytes(out, pos, asc_bytes);
    pos += asc_bytes.len;

    // Tag 0x06 SLConfigDescriptor (predefined = 2 for MP4)
    out[pos] = ESDS_TAG_SL_CONFIG;
    out[pos + 1] = 1;
    out[pos + 2] = 2;
    pos += 3;
    return pos;
}

/// Decodes an MP4 `esds` FullBox (or raw Tag `0x03` ES_Descriptor payload) and parses its embedded ASC.
pub fn decodeMp4EsdsBox(box_bytes: []const u8): Mp4EsdsInfo ! AacError {
    if (box_bytes.len < 16) return AacError.InvalidEsds;
    let pos: usize = 0;
    let end: usize = box_bytes.len;
    if (box_bytes[4] == 'e' && box_bytes[5] == 's' && box_bytes[6] == 'd' && box_bytes[7] == 's') {
        const declared: usize = readBeU32(box_bytes, 0) as usize;
        if (declared < 16 || declared > box_bytes.len) return AacError.InvalidEsds;
        end = declared;
        pos = 12;
    }

    if (pos >= end || box_bytes[pos] != ESDS_TAG_ES_DESCR) return AacError.InvalidEsds;
    pos += 1;
    const es_len = decodeDescrLen(box_bytes[0..end], pos)?;
    pos += es_len.bytes;
    if (pos + 3 > end) return AacError.TruncatedData;

    const es_id: u16 = ((box_bytes[pos] as u16) << 8) | (box_bytes[pos + 1] as u16);
    const es_flags: u8 = box_bytes[pos + 2];
    pos += 3;
    if ((es_flags & 0x80) != 0) pos += 2; // streamDependenceFlag
    if ((es_flags & 0x40) != 0) {
        if (pos >= end) return AacError.TruncatedData;
        pos += (box_bytes[pos] as usize) + 1; // URL_Flag
    }
    if ((es_flags & 0x20) != 0) pos += 2; // OCRstreamFlag

    if (pos >= end || box_bytes[pos] != ESDS_TAG_DECODER_CONFIG) return AacError.InvalidEsds;
    pos += 1;
    const dc_len = decodeDescrLen(box_bytes[0..end], pos)?;
    pos += dc_len.bytes;
    if (pos + 13 > end) return AacError.TruncatedData;

    const oti: u8 = box_bytes[pos];
    const stype: u8 = (box_bytes[pos + 1] >> 2) & 0x3F;
    const buf_db: u32 = ((box_bytes[pos + 2] as u32) << 16)
        | ((box_bytes[pos + 3] as u32) << 8)
        | (box_bytes[pos + 4] as u32);
    const max_br: u32 = readBeU32(box_bytes, pos + 5);
    const avg_br: u32 = readBeU32(box_bytes, pos + 9);
    pos += 13;

    if (pos >= end || box_bytes[pos] != ESDS_TAG_DECODER_SPECIFIC) return AacError.InvalidEsds;
    pos += 1;
    const dsi_len = decodeDescrLen(box_bytes[0..end], pos)?;
    pos += dsi_len.bytes;
    if (pos + dsi_len.value > end) return AacError.TruncatedData;

    const asc_slice = box_bytes[pos..(pos + dsi_len.value)];
    const parsed_asc = decodeAudioSpecificConfig(asc_slice)?;
    return Mp4EsdsInfo{
        es_id: es_id,
        object_type_indication: oti,
        stream_type: stype,
        buffer_size_db: buf_db,
        max_bitrate: max_br,
        avg_bitrate: avg_br,
        asc_raw: asc_slice,
        asc: parsed_asc,
    };
}

// ============================================================================
// 5. ISO/IEC 14496-3 §1.7.3 LOAS / LATM Streaming Sync & Parser
// ============================================================================

/// Zero-copy view of one LOAS `AudioSyncStream` frame wrapping an `AudioMuxElement`.
pub struct LoasFrameView {
    mux_element: []const u8 = "";
    frame_len: usize = 0;
    raw: []const u8 = "";
}

/// Encodes a LATM `AudioMuxElement` into a LOAS `AudioSyncStream` frame (`0x2B7` + 13-bit length).
pub fn encodeLoasFrame(mux_element: []const u8, out: []u8): usize ! AacError {
    if (mux_element.len > 8191) return AacError.InvalidLatm;
    const total: usize = mux_element.len + 3;
    if (out.len < total) return AacError.BufferTooSmall;
    const mlen: u16 = mux_element.len as u16;
    // 11-bit 0x2B7 = 0101_0110_111 -> byte0 = 0x56, top 3 bits of byte1 = 0xE0
    out[0] = 0x56;
    out[1] = (((mlen >> 8) & 0x1F) as u8) | 0xE0;
    out[2] = (mlen & 0xFF) as u8;
    copyBytes(out, 3, mux_element);
    return total;
}

/// Zero-copy LOAS `AudioSyncStream` iterator (scans for 11-bit syncword `0x2B7` + 13-bit length).
pub struct LoasFrameIter {
    data: []const u8 = "";
    pos: usize = 0;
    bytes_skipped: usize = 0;

    static fn init(buf: []const u8): LoasFrameIter {
        return LoasFrameIter{
            data: buf,
            pos: 0,
            bytes_skipped: 0,
        };
    }

    fn next(*_): LoasFrameView? ! AacError {
        while (pos + 3 <= data.len) {
            if (data[pos] == 0x56 && (data[pos + 1] & 0xE0) == 0xE0) {
                const mlen: usize = ((((data[pos + 1] & 0x1F) as u16) << 8) | (data[pos + 2] as u16)) as usize;
                if (pos + 3 + mlen <= data.len) {
                    const start: usize = pos;
                    const end: usize = pos + 3 + mlen;
                    pos = end;
                    return LoasFrameView{
                        mux_element: data[(start + 3)..end],
                        frame_len: mlen + 3,
                        raw: data[start..end],
                    };
                }
                return AacError.TruncatedData;
            }
            pos += 1;
            bytes_skipped += 1;
        }
        return null;
    }
}

/// Parsed LATM `AudioMuxElement` metadata and extracted Access Unit length.
pub struct LatmParsedFrame {
    use_same_stream_mux: bool = false;
    audio_mux_version: u8 = 0;
    all_streams_same_time_framing: bool = true;
    num_sub_frames: u8 = 0;
    num_program: u8 = 0;
    num_layer: u8 = 0;
    has_asc: bool = false;
    asc: AacAudioSpecificConfig = AacAudioSpecificConfig{};
    payload_len: usize = 0;
}

/// Parses a LATM `AudioMuxElement` (`audioMuxVersion == 0`, `frameLengthType == 0`),
/// decoding embedded `AudioSpecificConfig`, `PayloadLengthInfo` (`0xFF` accumulator),
/// and extracting the bit-unaligned raw AAC Access Unit into caller-provided `out_au`.
pub fn parseLatmStreamMuxConfigBasic(mux_element: []const u8, out_au: []u8): LatmParsedFrame ! AacError {
    if (mux_element.len < 3) return AacError.InvalidLatm;
    let br = BitReader{ .data = mux_element, .bit_pos = 0 };

    let res = LatmParsedFrame{};
    const use_same: u32 = br.readBits(1) orelse return AacError.InvalidLatm;
    res.use_same_stream_mux = use_same != 0;

    if (!res.use_same_stream_mux) {
        const ver = (br.readBits(1) orelse return AacError.InvalidLatm) as u8;
        if (ver != 0) return AacError.InvalidLatm;
        res.audio_mux_version = ver;
        res.all_streams_same_time_framing = (br.readBits(1) orelse return AacError.InvalidLatm) != 0;
        res.num_sub_frames = (br.readBits(6) orelse return AacError.InvalidLatm) as u8;
        res.num_program = (br.readBits(4) orelse return AacError.InvalidLatm) as u8;
        res.num_layer = (br.readBits(3) orelse return AacError.InvalidLatm) as u8;

        // Embedded AudioSpecificConfig (5-bit AOT, 4-bit sr_idx, 4-bit ch_cfg, 3-bit GASpecificConfig)
        const aot: u8 = (br.readBits(5) orelse return AacError.InvalidLatm) as u8;
        const sr_idx: u8 = (br.readBits(4) orelse return AacError.InvalidLatm) as u8;
        let sr_hz: u32 = 0;
        if (sr_idx == 15) {
            sr_hz = br.readBits(24) orelse return AacError.InvalidSampleRate;
        } else {
            sr_hz = aacSampleRate(sr_idx) orelse return AacError.InvalidSampleRate;
        }
        const ch_cfg: u8 = (br.readBits(4) orelse return AacError.InvalidChannelConfig) as u8;
        const fl960 = (br.readBits(1) orelse return AacError.InvalidLatm) != 0;
        const dep = (br.readBits(1) orelse return AacError.InvalidLatm) != 0;
        const ext = (br.readBits(1) orelse return AacError.InvalidLatm) != 0;

        res.has_asc = true;
        res.asc = AacAudioSpecificConfig{
            object_type: aot,
            sample_rate_idx: sr_idx,
            sample_rate_hz: sr_hz,
            channel_config: ch_cfg,
            channels: aacChannelCount(ch_cfg) ?? 0,
            frame_length_960: fl960,
            depends_on_core_coder: dep,
            extension_flag: ext,
        };

        const frame_len_type = br.readBits(3) orelse return AacError.InvalidLatm;
        if (frame_len_type != 0) return AacError.InvalidLatm;
        * = br.readBits(8) orelse return AacError.InvalidLatm; // latmBufferFullness
        const other_data = br.readBits(1) orelse return AacError.InvalidLatm;
        if (other_data != 0) return AacError.InvalidLatm;
        const crc_present = br.readBits(1) orelse return AacError.InvalidLatm;
        if (crc_present != 0) {
            * = br.readBits(8) orelse return AacError.InvalidLatm;
        }
    }

    // PayloadLengthInfo: sum 8-bit values until byte < 255
    let payload_len: usize = 0;
    while (true) {
        const seg: u32 = br.readBits(8) orelse return AacError.TruncatedData;
        payload_len += seg as usize;
        if (seg < 255) break;
    }
    if (out_au.len < payload_len) return AacError.BufferTooSmall;
    if (br.bitsRemaining() < payload_len * 8) return AacError.TruncatedData;

    let i: usize = 0;
    while (i < payload_len) : (i += 1) {
        out_au[i] = (br.readBits(8) orelse return AacError.TruncatedData) as u8;
    }
    res.payload_len = payload_len;
    return res;
}

// ============================================================================
// 6. RFC 3640 MPEG4-GENERIC (`AAC-hbr`) RTP Packetizer, Depacketizer & SDP `fmtp`
// ============================================================================

/// Zero-copy span of one AAC Access Unit inside a slab or RFC 3640 RTP payload.
pub struct AacRtpAuSpan {
    offset: usize = 0;
    len: usize = 0;
    au_index: u16 = 0;

    fn slice(buf: []const u8): []const u8 {
        return buf[offset..(offset + len)];
    }
}

/// Zero-copy span of one packetized RFC 3640 RTP payload inside a caller-provided output slab.
pub struct AacRtpPacketSpan {
    offset: usize = 0;
    len: usize = 0;

    fn slice(buf: []const u8): []const u8 {
        return buf[offset..(offset + len)];
    }
}

/// Packs one or more AAC Access Units (from `au_slab` via `au_spans`) into MTU-bounded
/// RFC 3640 `AAC-hbr` RTP payloads (`13-bit AU-size | 3-bit AU-Index` per AU).
/// Aggregates multiple small AUs per packet and fragments oversized AUs across packets.
pub fn packetizeAacRtpHbr(
    au_slab: []const u8,
    au_spans: []const AacRtpAuSpan,
    mtu: usize,
    out_buf: []u8,
    out_packets: []AacRtpPacketSpan,
): usize ! AacError {
    if (au_spans.len == 0) return AacError.InvalidParameter;
    if (mtu < 5) return AacError.MtuTooSmall;

    let pkt_count: usize = 0;
    let dst_off: usize = 0;
    let idx: usize = 0;

    while (idx < au_spans.len) {
        const first = au_spans[idx];
        if (first.offset + first.len > au_slab.len) return AacError.TruncatedData;

        // Oversized single AU -> fragment across MTU-sized single-AU packets
        if (first.len + 4 > mtu) {
            const max_chunk: usize = mtu - 4;
            let foff: usize = 0;
            while (foff < first.len) {
                if (pkt_count >= out_packets.len) return AacError.SpanOverflow;
                const rem: usize = first.len - foff;
                const take: usize = if (rem > max_chunk) max_chunk else rem;
                if (take > 8191) return AacError.InvalidRtpPacket;
                const plen: usize = take + 4;
                if (dst_off + plen > out_buf.len) return AacError.BufferTooSmall;

                out_buf[dst_off] = 0;
                out_buf[dst_off + 1] = 16; // 1 AU-header = 16 bits
                const hdr16: u16 = ((take as u16) << 3);
                out_buf[dst_off + 2] = ((hdr16 >> 8) & 0xFF) as u8;
                out_buf[dst_off + 3] = (hdr16 & 0xFF) as u8;
                copyBytes(out_buf, dst_off + 4, au_slab[(first.offset + foff)..(first.offset + foff + take)]);

                out_packets[pkt_count] = AacRtpPacketSpan{ offset: dst_off, len: plen };
                dst_off += plen;
                foff += take;
                pkt_count += 1;
            }
            idx += 1;
            continue;
        }

        // Aggregate as many contiguous AUs as fit within `mtu`
        let batch_end: usize = idx;
        let payload_sum: usize = 0;
        while (batch_end < au_spans.len) {
            const sp = au_spans[batch_end];
            if (sp.offset + sp.len > au_slab.len) return AacError.TruncatedData;
            if (sp.len > 8191) break;
            const next_cnt: usize = (batch_end - idx) + 1;
            const candidate_len: usize = payload_sum + sp.len + next_cnt * 2 + 2;
            if (candidate_len > mtu) break;
            payload_sum += sp.len;
            batch_end += 1;
        }

        if (pkt_count >= out_packets.len) return AacError.SpanOverflow;
        const cnt: usize = batch_end - idx;
        const total_pkt_len: usize = payload_sum + cnt * 2 + 2;
        if (dst_off + total_pkt_len > out_buf.len) return AacError.BufferTooSmall;

        const hdr_bits: u16 = (cnt * 16) as u16;
        out_buf[dst_off] = ((hdr_bits >> 8) & 0xFF) as u8;
        out_buf[dst_off + 1] = (hdr_bits & 0xFF) as u8;

        let hpos: usize = dst_off + 2;
        let dpos: usize = dst_off + 2 + cnt * 2;
        let k: usize = 0;
        while (k < cnt) : (k += 1) {
            const sp = au_spans[idx + k];
            const hdr16: u16 = ((sp.len as u16) << 3); // AU-Index / AU-Index-delta = 0
            out_buf[hpos] = ((hdr16 >> 8) & 0xFF) as u8;
            out_buf[hpos + 1] = (hdr16 & 0xFF) as u8;
            hpos += 2;
            copyBytes(out_buf, dpos, au_slab[sp.offset..(sp.offset + sp.len)]);
            dpos += sp.len;
        }

        out_packets[pkt_count] = AacRtpPacketSpan{ offset: dst_off, len: total_pkt_len };
        dst_off += total_pkt_len;
        pkt_count += 1;
        idx = batch_end;
    }
    return pkt_count;
}

/// Zero-copy RFC 3640 `AAC-hbr` RTP payload depacketizer.
/// Extracts all Access Unit spans (offsets into `rtp_payload`) into caller-provided `out_aus`.
pub fn depacketizeAacRtpHbr(rtp_payload: []const u8, out_aus: []AacRtpAuSpan): usize ! AacError {
    if (rtp_payload.len < 4) return AacError.InvalidRtpPacket;
    const au_hdr_bits: usize = (((rtp_payload[0] as u16) << 8) | (rtp_payload[1] as u16)) as usize;
    if (au_hdr_bits == 0 || (au_hdr_bits & 15) != 0) return AacError.InvalidRtpPacket;

    const au_count: usize = au_hdr_bits / 16;
    const hdr_bytes: usize = au_count * 2;
    if (hdr_bytes + 2 > rtp_payload.len) return AacError.TruncatedData;
    if (au_count > out_aus.len) return AacError.SpanOverflow;

    let hpos: usize = 2;
    let dpos: usize = hdr_bytes + 2;
    let running_idx: u16 = 0;
    let i: usize = 0;
    while (i < au_count) : (i += 1) {
        const hdr16: u16 = ((rtp_payload[hpos] as u16) << 8) | (rtp_payload[hpos + 1] as u16);
        hpos += 2;
        const au_size: usize = (hdr16 >> 3) as usize;
        const idx_field: u16 = hdr16 & 0x07;
        if (i == 0) {
            running_idx = idx_field;
        } else {
            running_idx += idx_field + 1;
        }
        if (dpos + au_size > rtp_payload.len) return AacError.TruncatedData;
        out_aus[i] = AacRtpAuSpan{
            offset: dpos,
            len: au_size,
            au_index: running_idx,
        };
        dpos += au_size;
    }
    return au_count;
}

fn appendAscii(out: []u8, pos: usize, s: []const u8): usize ! AacError {
    if (pos + s.len > out.len) return AacError.BufferTooSmall;
    copyBytes(out, pos, s);
    return pos + s.len;
}

fn appendDecU8(out: []u8, pos: usize, v: u8): usize ! AacError {
    let digits: [3]u8 = [0, 0, 0];
    let n: usize = 0;
    let x: u8 = v;
    if (x == 0) {
        digits[0] = '0';
        n = 1;
    } else {
        while (x > 0) {
            digits[n] = ('0' as u8) + (x % 10);
            x /= 10;
            n += 1;
        }
    }
    if (pos + n > out.len) return AacError.BufferTooSmall;
    let i: usize = 0;
    while (i < n) : (i += 1) {
        out[pos + i] = digits[n - 1 - i];
    }
    return pos + n;
}

/// Formats an RFC 3640 SDP `a=fmtp` attribute line for `AAC-hbr`:
/// `a=fmtp:<pt> streamtype=5;profile-level-id=1;mode=AAC-hbr;sizelength=13;indexlength=3;indexdeltalength=3;config=<HEX>`
pub fn formatAacSdpFmtp(payload_type: u8, asc_bytes: []const u8, out: []u8): usize ! AacError {
    if (asc_bytes.len < 2) return AacError.InvalidAsc;
    const hex_chars: [16]u8 = ['0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F'];
    let pos: usize = 0;
    pos = appendAscii(out, pos, "a=fmtp:")?;
    pos = appendDecU8(out, pos, payload_type)?;
    pos = appendAscii(out, pos, " streamtype=5;profile-level-id=1;mode=AAC-hbr;sizelength=13;indexlength=3;indexdeltalength=3;config=")?;
    if (pos + asc_bytes.len * 2 > out.len) return AacError.BufferTooSmall;
    let i: usize = 0;
    while (i < asc_bytes.len) : (i += 1) {
        const b = asc_bytes[i];
        out[pos] = hex_chars[(b >> 4) as usize];
        out[pos + 1] = hex_chars[(b & 0x0F) as usize];
        pos += 2;
    }
    return pos;
}
