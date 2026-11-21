// Source: Absorbed from /data/dev/bun/src/ (Zero-Alloc AAC ADTS Frame & AudioSpecificConfig Parser)

pub const ADTS_HEADER_SIZE: usize = 7;
pub const ADTS_HEADER_CRC_SIZE: usize = 9;
pub const AAC_SAMPLES_PER_BLOCK: u64 = 1024;

// ISO/IEC 14496-3 standard 13-entry sampling frequency table (Hz)
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

    fn init(data: []const u8): AdtsIter {
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
