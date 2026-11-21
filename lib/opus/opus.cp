// Source: Absorbed from /data/dev/bun/src/ (Zero-Alloc Opus RFC 7845 & RFC 6716 TOC Parser)

/// RFC 7845 OpusHead 魔数签名（8 字节）
pub const OPUS_HEAD_MAGIC: []const u8 = "OpusHead";

/// RFC 7845 OpusTags 魔数签名（8 字节）
pub const OPUS_TAGS_MAGIC: []const u8 = "OpusTags";

/// Opus 内部恒定解码采样率（48000 Hz）
pub const OPUS_SAMPLE_RATE: u32 = 48000;

/// RFC 6716 单包最大音频时长（120 ms = 120000 us）
pub const OPUS_MAX_PACKET_DURATION_US: u32 = 120000;

/// RFC 6716 Opus 编码模式
pub enum OpusMode: u8 {
    silk = 0,
    hybrid = 1,
    celt = 2,
}

/// RFC 6716 Opus 音频带宽
pub enum OpusBandwidth: u8 {
    narrowband = 0,
    mediumband = 1,
    wideband = 2,
    superwideband = 3,
    fullband = 4,
}

/// RFC 7845 Section 5.1 零拷贝 OpusHead 标识头视图
pub struct OpusHead {
    version: u8,
    channels: u8,
    pre_skip: u16,
    input_sample_rate: u32,
    output_gain: i16,
    channel_mapping_family: u8,
    stream_count: u8,
    coupled_count: u8,
    channel_mapping: []const u8,

    /// 输出增益（dB，由 Q7.8 定点数换算：`output_gain / 256.0`）
    fn outputGainDb(): f64 {
        return (_.output_gain as f64) / 256.0;
    }

    /// 预跳过采样数对应的微秒时长（基于 48 kHz）
    fn preSkipUs(): u64 {
        return ((_.pre_skip as u64) * 1000000) / (OPUS_SAMPLE_RATE as u64);
    }
}

/// RFC 7845 Section 5.2 零拷贝单条 OpusTags 注释条目（`KEY=VALUE`）
pub struct OpusTag {
    raw: []const u8,
    key: []const u8,
    value: []const u8,
}

/// RFC 7845 Section 5.2 零堆分配 OpusTags 用户注释流式迭代器
pub struct OpusCommentIter {
    data: []const u8,
    pos: usize = 0,
    remaining: u32 = 0,

    static fn init(comments_data: []const u8, count: u32): OpusCommentIter {
        return OpusCommentIter {
            data: comments_data,
            pos: 0,
            remaining: count,
        };
    }

    /// 读取下一条 `KEY=VALUE` 注释；到达末尾或遇到截断条目时安全返回 `null`
    fn next(*_): OpusTag? {
        if (_.remaining == 0) return null;
        if (_.data.len - _.pos < 4) {
            _.remaining = 0;
            return null;
        }

        const entry_len: usize = readU32LE(_.data, _.pos) as usize;
        const start: usize = _.pos + 4;
        if (entry_len > _.data.len - start) {
            _.remaining = 0;
            return null;
        }

        const raw = _.data[start .. start + entry_len];
        _.pos = start + entry_len;
        _.remaining -= 1;

        let eq_idx: usize = raw.len;
        let i: usize = 0;
        while (i < raw.len) {
            if (raw[i] == '=') {
                eq_idx = i;
                break;
            }
            i += 1;
        }

        if (eq_idx < raw.len) {
            return OpusTag {
                raw: raw,
                key: raw[0..eq_idx],
                value: raw[eq_idx + 1 .. raw.len],
            };
        }

        return OpusTag {
            raw: raw,
            key: raw,
            value: raw[raw.len .. raw.len],
        };
    }
}

/// RFC 7845 Section 5.2 零拷贝 OpusTags 注释头视图
pub struct OpusTagsInfo {
    vendor: []const u8,
    user_comment_count: u32,
    comments_data: []const u8,

    /// 创建零拷贝用户注释迭代器
    fn comments(): OpusCommentIter {
        return OpusCommentIter.init(_.comments_data, _.user_comment_count);
    }
}

/// RFC 6716 Section 3.1/3.2 Opus TOC 分组头解码结果
pub struct OpusToc {
    config: u8,
    mode: OpusMode,
    bandwidth: OpusBandwidth,
    frame_duration_us: u32,
    is_stereo: bool,
    frame_count_code: u8,
    frame_count: u8,
    is_vbr: bool,
    has_padding: bool,
    packet_duration_us: u32,

    /// 通道数（单声道为 1，立体声为 2）
    fn channels(): u8 {
        return if (_.is_stereo) 2 else 1;
    }

    /// 单帧在 48 kHz 下的采样点数（120 / 240 / 480 / 960 / 1920 / 2880）
    fn samplesPerFrame(): u32 {
        return (_.frame_duration_us * 48) / 1000;
    }

    /// 整个 Opus 分组在 48 kHz 下的总采样点数
    fn totalSamples(): u32 {
        return _.samplesPerFrame() * (_.frame_count as u32);
    }
}

fn readU16LE(data: []const u8, off: usize): u16 {
    return (data[off] as u16) | ((data[off + 1] as u16) << 8);
}

fn readU32LE(data: []const u8, off: usize): u32 {
    return (data[off] as u32)
        | ((data[off + 1] as u32) << 8)
        | ((data[off + 2] as u32) << 16)
        | ((data[off + 3] as u32) << 24);
}

/// 检查数据是否以 8 字节 `"OpusHead"` 魔数开头
pub fn isOpusHead(data: []const u8): bool {
    if (data.len < 8) return false;
    return data[0] == 'O'
        && data[1] == 'p'
        && data[2] == 'u'
        && data[3] == 's'
        && data[4] == 'H'
        && data[5] == 'e'
        && data[6] == 'a'
        && data[7] == 'd';
}

/// 检查数据是否以 8 字节 `"OpusTags"` 魔数开头
pub fn isOpusTags(data: []const u8): bool {
    if (data.len < 8) return false;
    return data[0] == 'O'
        && data[1] == 'p'
        && data[2] == 'u'
        && data[3] == 's'
        && data[4] == 'T'
        && data[5] == 'a'
        && data[6] == 'g'
        && data[7] == 's';
}

/// 解析 RFC 7845 `OpusHead` 标识头（至少 19 字节；`family != 0` 时校验并切片通道映射表）
pub fn parseOpusHead(data: []const u8): OpusHead? {
    if (data.len < 19) return null;
    if (!isOpusHead(data)) return null;

    const version = data[8];
    // RFC 7845 Section 5.1.1: 主版本号（高 4 位）必须为 0
    if ((version & 0xF0) != 0) return null;

    const channels = data[9];
    if (channels == 0) return null;

    const pre_skip = readU16LE(data, 10);
    const input_sample_rate = readU32LE(data, 12);
    const output_gain_raw = readU16LE(data, 16);
    const output_gain: i16 = output_gain_raw as i16;
    const family = data[18];

    if (family == 0) {
        // RFC 7845 Section 5.1.1.1: family 0 仅允许单声道或立体声（1 或 2 通道）
        if (channels > 2) return null;
        const coupled: u8 = if (channels == 2) 1 else 0;
        return OpusHead {
            version: version,
            channels: channels,
            pre_skip: pre_skip,
            input_sample_rate: input_sample_rate,
            output_gain: output_gain,
            channel_mapping_family: 0,
            stream_count: 1,
            coupled_count: coupled,
            channel_mapping: data[19..19],
        };
    }

    // family != 0: 至少包含 stream_count(1B) + coupled_count(1B) + channel_mapping(channels B)
    const need_len: usize = (channels as usize) + 21;
    if (data.len < need_len) return null;

    const stream_count = data[19];
    const coupled_count = data[20];
    if (stream_count == 0) return null;
    if (coupled_count > stream_count) return null;

    const total_decoded: u16 = (stream_count as u16) + (coupled_count as u16);
    if (total_decoded > 255) return null;

    const mapping = data[21..need_len];
    let i: usize = 0;
    while (i < mapping.len) {
        const idx = mapping[i];
        if (idx != 255 && (idx as u16) >= total_decoded) {
            return null;
        }
        i += 1;
    }

    return OpusHead {
        version: version,
        channels: channels,
        pre_skip: pre_skip,
        input_sample_rate: input_sample_rate,
        output_gain: output_gain,
        channel_mapping_family: family,
        stream_count: stream_count,
        coupled_count: coupled_count,
        channel_mapping: mapping,
    };
}

/// 解析 RFC 7845 `OpusTags` 注释头，返回零拷贝厂商字符串与注释迭代视图
pub fn parseOpusTags(data: []const u8): OpusTagsInfo? {
    if (data.len < 16) return null;
    if (!isOpusTags(data)) return null;

    const vendor_len: usize = readU32LE(data, 8) as usize;
    if (vendor_len > data.len - 12) return null;

    const count_off: usize = vendor_len + 12;
    if (data.len - count_off < 4) return null;

    const vendor = data[12..count_off];
    const user_comment_count = readU32LE(data, count_off);
    const comments_data = data[count_off + 4 .. data.len];

    return OpusTagsInfo {
        vendor: vendor,
        user_comment_count: user_comment_count,
        comments_data: comments_data,
    };
}

/// 解析 RFC 6716 Opus 分组 TOC 字节及帧计数（支持 c=0/1/2/3 与 120ms 上限校验）
pub fn parseOpusToc(packet: []const u8): OpusToc? {
    if (packet.len == 0) return null;

    const toc = packet[0];
    const config: u8 = toc >> 3;
    const is_stereo: bool = (toc & 0x04) != 0;
    const code: u8 = toc & 0x03;

    let mode: OpusMode = OpusMode.silk;
    let bandwidth: OpusBandwidth = OpusBandwidth.narrowband;
    let frame_duration_us: u32 = 20000;

    if (config < 12) {
        // SILK-only (0..11)
        mode = OpusMode.silk;
        if (config < 4) {
            bandwidth = OpusBandwidth.narrowband;
        } else if (config < 8) {
            bandwidth = OpusBandwidth.mediumband;
        } else {
            bandwidth = OpusBandwidth.wideband;
        }

        const sub = config & 0x03;
        if (sub == 0) {
            frame_duration_us = 10000;
        } else if (sub == 1) {
            frame_duration_us = 20000;
        } else if (sub == 2) {
            frame_duration_us = 40000;
        } else {
            frame_duration_us = 60000;
        }
    } else if (config < 16) {
        // Hybrid (12..15)
        mode = OpusMode.hybrid;
        if (config < 14) {
            bandwidth = OpusBandwidth.superwideband;
        } else {
            bandwidth = OpusBandwidth.fullband;
        }
        frame_duration_us = if ((config & 0x01) == 0) 10000 else 20000;
    } else {
        // CELT-only (16..31)
        mode = OpusMode.celt;
        if (config < 20) {
            bandwidth = OpusBandwidth.narrowband;
        } else if (config < 24) {
            bandwidth = OpusBandwidth.wideband;
        } else if (config < 28) {
            bandwidth = OpusBandwidth.superwideband;
        } else {
            bandwidth = OpusBandwidth.fullband;
        }

        const sub = config & 0x03;
        if (sub == 0) {
            frame_duration_us = 2500;
        } else if (sub == 1) {
            frame_duration_us = 5000;
        } else if (sub == 2) {
            frame_duration_us = 10000;
        } else {
            frame_duration_us = 20000;
        }
    }

    let frame_count: u8 = 1;
    let is_vbr: bool = false;
    let has_padding: bool = false;

    if (code == 0) {
        frame_count = 1;
    } else if (code == 1) {
        frame_count = 2;
        is_vbr = false;
    } else if (code == 2) {
        frame_count = 2;
        is_vbr = true;
    } else {
        // code == 3: 任意帧数，必须存在第 2 字节（帧计数字节）
        if (packet.len < 2) return null;
        const fc_byte = packet[1];
        is_vbr = (fc_byte & 0x80) != 0;
        has_padding = (fc_byte & 0x40) != 0;
        frame_count = fc_byte & 0x3F;
        if (frame_count == 0) return null;
    }

    const packet_duration_us: u32 = frame_duration_us * (frame_count as u32);
    if (packet_duration_us > OPUS_MAX_PACKET_DURATION_US) return null;

    return OpusToc {
        config: config,
        mode: mode,
        bandwidth: bandwidth,
        frame_duration_us: frame_duration_us,
        is_stereo: is_stereo,
        frame_count_code: code,
        frame_count: frame_count,
        is_vbr: is_vbr,
        has_padding: has_padding,
        packet_duration_us: packet_duration_us,
    };
}
