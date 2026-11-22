// Origin: Pure-CP Stdlib (RFC 7845 OpusHead/OpusTags, RFC 6716 Opus Codec Framing/LBRR-FEC/DTX & RFC 7587 WebRTC RTP) | Created: 2026-11-22
// 100% pure CP, zero dynamic heap allocation, zero external FFI dependencies, caller-slice Opus audio core:
//   - 100% backwards-compatible RFC 7845 `OpusHead`/`OpusTags` & RFC 6716 `OpusToc` parsers
//   - Complete RFC 6716 Opus Packet parser & repacker for Code 0 / 1 / 2 / 3 (CBR/VBR, 1/2B frame lengths, padding & <=120ms guard)
//   - RFC 6716 §4.2.7 SILK/Hybrid bit-level VAD, In-Band FEC (LBRR) & DTX silence diagnostics (`OpusPacketDiagnostics`)
//   - RFC 7587 WebRTC Opus SDP `fmtp` parser/formatter (`minptime`, `maxplaybackrate`, `maxaveragebitrate`, `stereo`, `sprop-stereo`, `cbr`, `useinbandfec`, `usedtx`)
//   - `OpusRtpPacketizer` & `OpusRtpUnpacker`: 48kHz timestamp tracking, post-DTX RTP Marker bit, ptime aggregation & multi-frame splitting

// ============================================================================
// 1. Constants, Legacy Enums, Error Set & Legacy RFC 7845 / 6716 Structs
// ============================================================================

/// RFC 7845 OpusHead 魔数签名（8 字节）
pub const OPUS_HEAD_MAGIC: []const u8 = "OpusHead";

/// RFC 7845 OpusTags 魔数签名（8 字节）
pub const OPUS_TAGS_MAGIC: []const u8 = "OpusTags";

/// Opus 内部恒定解码采样率（48000 Hz）
pub const OPUS_SAMPLE_RATE: u32 = 48000;

/// RFC 6716 单包最大音频时长（120 ms = 120000 us）
pub const OPUS_MAX_PACKET_DURATION_US: u32 = 120000;

/// RFC 6716 §3.2.1 Code 2/3 single-frame self-delimiting maximum length (`1275` bytes).
pub const OPUS_MAX_FRAME_LEN: usize = 1275;

/// RFC 6716 §3.2.5 maximum frames per Opus packet (`48`, corresponding to 48 x 2.5ms = 120ms).
pub const OPUS_MAX_FRAMES_PER_PACKET: usize = 48;

/// Error set returned by RFC 6716 Opus framing/repacking and RFC 7587 WebRTC RTP/SDP routines.
pub err OpusError {
    EmptyPacket,
    TruncatedPacket,
    InvalidToc,
    InvalidFrameCount,
    DurationExceeds120Ms,
    InvalidFrameLength,
    InvalidPadding,
    UnevenCbrFrames,
    BufferTooSmall,
    SpanOverflow,
    FrameTooLarge,
    MixedTocConfig,
    InvalidFmtp,
    InvalidParameter,
}

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
    config: u8 = 0;
    mode: OpusMode = OpusMode.silk;
    bandwidth: OpusBandwidth = OpusBandwidth.narrowband;
    frame_duration_us: u32 = 20000;
    is_stereo: bool = false;
    frame_count_code: u8 = 0;
    frame_count: u8 = 1;
    is_vbr: bool = false;
    has_padding: bool = false;
    packet_duration_us: u32 = 20000;

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

fn copyBytes(dst: []u8, dst_off: usize, src: []const u8) {
    let i: usize = 0;
    while (i < src.len) : (i += 1) {
        dst[dst_off + i] = src[i];
    }
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
    if ((version & 0xF0) != 0) return null;

    const channels = data[9];
    if (channels == 0) return null;

    const pre_skip = readU16LE(data, 10);
    const input_sample_rate = readU32LE(data, 12);
    const output_gain_raw = readU16LE(data, 16);
    const output_gain: i16 = output_gain_raw as i16;
    const family = data[18];

    if (family == 0) {
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

/// Helper mapping any 5-bit RFC 6716 TOC `config` (`0..31`) to frame duration in microseconds.
pub fn opusConfigDurationUs(config: u8): u32 {
    if (config < 12) {
        const sub = config & 0x03;
        if (sub == 0) return 10000;
        if (sub == 1) return 20000;
        if (sub == 2) return 40000;
        return 60000;
    }
    if (config < 16) {
        return if ((config & 0x01) == 0) 10000 else 20000;
    }
    const sub = config & 0x03;
    if (sub == 0) return 2500;
    if (sub == 1) return 5000;
    if (sub == 2) return 10000;
    return 20000;
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
    const frame_duration_us: u32 = opusConfigDurationUs(config);

    if (config < 12) {
        mode = OpusMode.silk;
        if (config < 4) {
            bandwidth = OpusBandwidth.narrowband;
        } else if (config < 8) {
            bandwidth = OpusBandwidth.mediumband;
        } else {
            bandwidth = OpusBandwidth.wideband;
        }
    } else if (config < 16) {
        mode = OpusMode.hybrid;
        bandwidth = if (config < 14) OpusBandwidth.superwideband else OpusBandwidth.fullband;
    } else {
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

// ============================================================================
// 2. Complete RFC 6716 Opus Packet Frame Parser & Repacker (Code 0/1/2/3)
// ============================================================================

/// Zero-copy slice & offset descriptor for one Opus frame inside a packet or slab.
pub struct OpusFrameSlice {
    offset: usize = 0;
    len: usize = 0;
    data: []const u8 = "";
}

/// Summary returned by `parseOpusPacketFrames`.
pub struct OpusPacketParsed {
    toc: OpusToc = OpusToc{};
    frame_count: usize = 0;
    padding_bytes: usize = 0;
    total_samples_48k: u32 = 0;
    total_duration_us: u32 = 0;
}

/// Decoded RFC 6716 §3.2.1 1-or-2-byte Opus frame length field.
pub struct OpusLenDecoded {
    frame_len: usize = 0;
    bytes_read: usize = 0;
}

/// Encodes an RFC 6716 §3.2.1 Opus frame length (`0..251` -> 1B, `252..1275` -> 2B) into `out`.
pub fn encodeOpusFrameLen(frame_len: usize, out: []u8): usize ! OpusError {
    if (frame_len > OPUS_MAX_FRAME_LEN) return OpusError.FrameTooLarge;
    if (frame_len < 252) {
        if (out.len < 1) return OpusError.BufferTooSmall;
        out[0] = frame_len as u8;
        return 1;
    }
    if (out.len < 2) return OpusError.BufferTooSmall;
    const rem: usize = frame_len - 252;
    out[0] = ((rem & 3) + 252) as u8;
    out[1] = (rem >> 2) as u8;
    return 2;
}

/// Decodes an RFC 6716 §3.2.1 Opus frame length (`0..1275`) from `data`.
pub fn decodeOpusFrameLen(data: []const u8): OpusLenDecoded ! OpusError {
    if (data.len == 0) return OpusError.TruncatedPacket;
    const b0: usize = data[0] as usize;
    if (b0 < 252) {
        return OpusLenDecoded{ frame_len: b0, bytes_read: 1 };
    }
    if (data.len < 2) return OpusError.TruncatedPacket;
    const b1: usize = data[1] as usize;
    return OpusLenDecoded{
        frame_len: b1 * 4 + b0,
        bytes_read: 2,
    };
}

/// Parses an RFC 6716 Opus packet (Code 0, 1, 2, or 3 CBR/VBR + optional padding)
/// into caller-provided `out_frames` with zero dynamic heap allocation and strict <=120ms validation.
pub fn parseOpusPacketFrames(packet: []const u8, out_frames: []OpusFrameSlice): OpusPacketParsed ! OpusError {
    if (packet.len == 0) return OpusError.EmptyPacket;
    const code: u8 = packet[0] & 0x03;
    const cfg: u8 = packet[0] >> 3;
    if (code == 3) {
        if (packet.len < 2) return OpusError.TruncatedPacket;
        const m: u8 = packet[1] & 0x3F;
        if (m == 0 || m > (OPUS_MAX_FRAMES_PER_PACKET as u8)) return OpusError.InvalidFrameCount;
        if (opusConfigDurationUs(cfg) * (m as u32) > OPUS_MAX_PACKET_DURATION_US) {
            return OpusError.DurationExceeds120Ms;
        }
    }
    const toc = parseOpusToc(packet) orelse return OpusError.InvalidToc;
    const fcnt: usize = toc.frame_count as usize;
    if (out_frames.len < fcnt) return OpusError.SpanOverflow;

    let pad_bytes: usize = 0;

    if (code == 0) {
        const flen: usize = packet.len - 1;
        out_frames[0] = OpusFrameSlice{ offset: 1, len: flen, data: packet[1..packet.len] };
    } else if (code == 1) {
        const rem: usize = packet.len - 1;
        if ((rem & 1) != 0) return OpusError.UnevenCbrFrames;
        const flen: usize = rem / 2;
        if (flen > OPUS_MAX_FRAME_LEN) return OpusError.InvalidFrameLength;
        out_frames[0] = OpusFrameSlice{ offset: 1, len: flen, data: packet[1..(flen + 1)] };
        out_frames[1] = OpusFrameSlice{ offset: flen + 1, len: flen, data: packet[(flen + 1)..packet.len] };
    } else if (code == 2) {
        const l0 = decodeOpusFrameLen(packet[1..packet.len])?;
        const f0_start: usize = l0.bytes_read + 1;
        if (f0_start + l0.frame_len > packet.len) return OpusError.TruncatedPacket;
        const f1_start: usize = f0_start + l0.frame_len;
        const f1_len: usize = packet.len - f1_start;
        if (f1_len > OPUS_MAX_FRAME_LEN) return OpusError.InvalidFrameLength;
        out_frames[0] = OpusFrameSlice{ offset: f0_start, len: l0.frame_len, data: packet[f0_start..f1_start] };
        out_frames[1] = OpusFrameSlice{ offset: f1_start, len: f1_len, data: packet[f1_start..packet.len] };
    } else {
        // Code 3: arbitrary M frames, optional padding, CBR or VBR
        let pos: usize = 2;
        if (toc.has_padding) {
            while (true) {
                if (pos >= packet.len) return OpusError.InvalidPadding;
                const p: usize = packet[pos] as usize;
                pos += 1;
                if (p == 255) {
                    pad_bytes += 254;
                } else {
                    pad_bytes += p;
                    break;
                }
            }
            if (pos + pad_bytes > packet.len) return OpusError.InvalidPadding;
        }
        const end: usize = packet.len - pad_bytes;

        if (!toc.is_vbr) {
            const avail: usize = end - pos;
            if (avail % fcnt != 0) return OpusError.UnevenCbrFrames;
            const flen: usize = avail / fcnt;
            if (flen > OPUS_MAX_FRAME_LEN) return OpusError.InvalidFrameLength;
            let i: usize = 0;
            while (i < fcnt) : (i += 1) {
                const off: usize = pos + i * flen;
                out_frames[i] = OpusFrameSlice{ offset: off, len: flen, data: packet[off..(off + flen)] };
            }
        } else {
            // Decode M-1 frame lengths first, then slice payloads
            let lens: [48]usize = [0; 48];
            let sum_lens: usize = 0;
            let i: usize = 0;
            while (i + 1 < fcnt) : (i += 1) {
                const ld = decodeOpusFrameLen(packet[pos..end])?;
                pos += ld.bytes_read;
                lens[i] = ld.frame_len;
                sum_lens += ld.frame_len;
            }
            if (pos + sum_lens > end) return OpusError.TruncatedPacket;
            const last_len: usize = (end - pos) - sum_lens;
            if (last_len > OPUS_MAX_FRAME_LEN) return OpusError.InvalidFrameLength;
            lens[fcnt - 1] = last_len;

            let dpos: usize = pos;
            i = 0;
            while (i < fcnt) : (i += 1) {
                const fl = lens[i];
                out_frames[i] = OpusFrameSlice{ offset: dpos, len: fl, data: packet[dpos..(dpos + fl)] };
                dpos += fl;
            }
        }
    }

    return OpusPacketParsed{
        toc: toc,
        frame_count: fcnt,
        padding_bytes: pad_bytes,
        total_samples_48k: toc.totalSamples(),
        total_duration_us: toc.packet_duration_us,
    };
}

/// Options controlling `repackOpusFrames`.
pub struct OpusRepackOptions {
    config: u8 = 28; // CELT FB 20ms default
    is_stereo: bool = true;
    force_code3: bool = false;
    target_packet_len: usize = 0;
}

/// Repacks 1..48 Opus frames into the most compact RFC 6716 Code 0/1/2/3 packet
/// (supporting explicit CBR/VBR detection, Code 3 zero-padding to `target_packet_len`, and <=120ms guard).
pub fn repackOpusFrames(
    frames: []const OpusFrameSlice,
    opts: OpusRepackOptions,
    out: []u8,
): usize ! OpusError {
    if (frames.len == 0 || frames.len > OPUS_MAX_FRAMES_PER_PACKET) return OpusError.InvalidFrameCount;
    if (opts.config > 31) return OpusError.InvalidToc;
    const total_dur: u32 = opusConfigDurationUs(opts.config) * (frames.len as u32);
    if (total_dur > OPUS_MAX_PACKET_DURATION_US) return OpusError.DurationExceeds120Ms;

    const toc_base: u8 = (opts.config << 3) | (if (opts.is_stereo) 0x04 else 0x00);

    // Check if all frames are equal length & validate max frame len for multi-frame packets
    let all_equal: bool = true;
    let payload_sum: usize = 0;
    let i: usize = 0;
    while (i < frames.len) : (i += 1) {
        if (frames.len > 1 && frames[i].data.len > OPUS_MAX_FRAME_LEN) {
            return OpusError.FrameTooLarge;
        }
        if (frames[i].data.len != frames[0].data.len) {
            all_equal = false;
        }
        payload_sum += frames[i].data.len;
    }

    const need_padding: bool = opts.target_packet_len > 0;

    // Fast path: Code 0 (single frame, no forced code3, no padding needed)
    if (frames.len == 1 && !opts.force_code3 && (!need_padding || opts.target_packet_len <= payload_sum + 1)) {
        const total: usize = payload_sum + 1;
        if (out.len < total) return OpusError.BufferTooSmall;
        out[0] = toc_base;
        copyBytes(out, 1, frames[0].data);
        return total;
    }

    // Fast path: Code 1 (2 equal-length frames, no padding)
    if (frames.len == 2 && all_equal && !opts.force_code3 && !need_padding) {
        const total: usize = payload_sum + 1;
        if (out.len < total) return OpusError.BufferTooSmall;
        out[0] = toc_base | 0x01;
        copyBytes(out, 1, frames[0].data);
        copyBytes(out, frames[0].data.len + 1, frames[1].data);
        return total;
    }

    // Fast path: Code 2 (2 unequal-length frames, no padding)
    if (frames.len == 2 && !all_equal && !opts.force_code3 && !need_padding) {
        let len_buf: [2]u8 = [0, 0];
        const lb_len = encodeOpusFrameLen(frames[0].data.len, len_buf[0..2])?;
        const total: usize = payload_sum + lb_len + 1;
        if (out.len < total) return OpusError.BufferTooSmall;
        out[0] = toc_base | 0x02;
        copyBytes(out, 1, len_buf[0..lb_len]);
        copyBytes(out, lb_len + 1, frames[0].data);
        copyBytes(out, lb_len + 1 + frames[0].data.len, frames[1].data);
        return total;
    }

    // General path: Code 3 (CBR or VBR, with optional padding to reach target_packet_len)
    const is_vbr: bool = !all_equal;
    let vbr_hdr_bytes: usize = 0;
    if (is_vbr) {
        let k: usize = 0;
        while (k + 1 < frames.len) : (k += 1) {
            vbr_hdr_bytes += if (frames[k].data.len < 252) 1 else 2;
        }
    }
    const base_len: usize = payload_sum + vbr_hdr_bytes + 2;
    let pad_indicator_bytes: usize = 0;
    let pad_trailing_bytes: usize = 0;
    let has_pad: bool = false;

    if (opts.target_packet_len > base_len) {
        has_pad = true;
        const extra: usize = opts.target_packet_len - base_len;
        // Each 255 indicator byte accounts for 255 total bytes (1 indicator + 254 trailing zeros);
        // final indicator byte k (0..254) accounts for (1 + k) total bytes.
        const full_255: usize = (extra - 1) / 255;
        const rem_pad: usize = extra - full_255 * 255 - 1;
        pad_indicator_bytes = full_255 + 1;
        pad_trailing_bytes = full_255 * 254 + rem_pad;
    }

    const total_len: usize = base_len + pad_indicator_bytes + pad_trailing_bytes;
    if (out.len < total_len) return OpusError.BufferTooSmall;

    out[0] = toc_base | 0x03;
    let fc: u8 = (frames.len as u8) & 0x3F;
    if (is_vbr) fc |= 0x80;
    if (has_pad) fc |= 0x40;
    out[1] = fc;

    let pos: usize = 2;
    if (has_pad) {
        let rem_trail: usize = pad_trailing_bytes;
        while (rem_trail >= 254 && pad_indicator_bytes > 1) {
            out[pos] = 255;
            pos += 1;
            rem_trail -= 254;
            pad_indicator_bytes -= 1;
        }
        out[pos] = rem_trail as u8;
        pos += 1;
    }

    if (is_vbr) {
        let k: usize = 0;
        while (k + 1 < frames.len) : (k += 1) {
            const wn = encodeOpusFrameLen(frames[k].data.len, out[pos..out.len])?;
            pos += wn;
        }
    }

    i = 0;
    while (i < frames.len) : (i += 1) {
        copyBytes(out, pos, frames[i].data);
        pos += frames[i].data.len;
    }

    let z: usize = 0;
    while (z < pad_trailing_bytes) : (z += 1) {
        out[pos + z] = 0;
    }
    return pos + pad_trailing_bytes;
}

// ============================================================================
// 3. RFC 6716 §4.2.7 SILK/Hybrid Bit-Level VAD, LBRR In-Band FEC & DTX Diagnostics
// ============================================================================

/// Bit-level voice activity (VAD), In-Band FEC (LBRR), and DTX classification for one Opus frame.
pub struct OpusFrameSignalInfo {
    is_dtx: bool = false;
    has_vad: bool = false;
    has_lbrr_fec: bool = false;
    silk_subframes: u8 = 0;
}

/// Inspects a single Opus frame's SILK/Hybrid header bits (RFC 6716 §4.2.7) for VAD, LBRR FEC, and DTX.
pub fn inspectOpusFrameSignal(toc: OpusToc, frame_data: []const u8): OpusFrameSignalInfo {
    if (frame_data.len == 0) {
        return OpusFrameSignalInfo{ is_dtx: true, has_vad: false, has_lbrr_fec: false, silk_subframes: 0 };
    }
    if (toc.mode == OpusMode.celt) {
        // CELT-only frames do not carry SILK LBRR FEC; <= 1-byte empty/DTX frame indicates silence
        return OpusFrameSignalInfo{
            is_dtx: frame_data.len <= 1,
            has_vad: frame_data.len > 1,
            has_lbrr_fec: false,
            silk_subframes: 0,
        };
    }

    // SILK-only or Hybrid: 1 subframe for 10/20ms, 2 for 40ms, 3 for 60ms
    let subframes: u8 = 1;
    if (toc.frame_duration_us == 40000) subframes = 2;
    if (toc.frame_duration_us == 60000) subframes = 3;

    const ch_cnt: usize = toc.channels() as usize;
    const first_byte: u8 = frame_data[0];
    let bit_pos: usize = 0;
    let any_vad: bool = false;
    let any_lbrr: bool = false;

    let ch: usize = 0;
    while (ch < ch_cnt) : (ch += 1) {
        let sf: u8 = 0;
        while (sf < subframes) : (sf += 1) {
            if (bit_pos < 8) {
                const shift: u3 = @as(u3, @intCast(7 - bit_pos));
                if (((first_byte >> shift) & 1) != 0) any_vad = true;
            }
            bit_pos += 1;
        }
        if (bit_pos < 8) {
            const shift: u3 = @as(u3, @intCast(7 - bit_pos));
            if (((first_byte >> shift) & 1) != 0) any_lbrr = true;
        }
        bit_pos += 1;
    }

    const is_dtx_frame: bool = !any_vad && !any_lbrr && frame_data.len <= 2;
    return OpusFrameSignalInfo{
        is_dtx: is_dtx_frame,
        has_vad: any_vad,
        has_lbrr_fec: any_lbrr,
        silk_subframes: subframes,
    };
}

/// Complete single-call diagnostics summary for an incoming or outgoing Opus packet.
pub struct OpusPacketDiagnostics {
    mode: OpusMode = OpusMode.silk;
    bandwidth: OpusBandwidth = OpusBandwidth.narrowband;
    channels: u8 = 1;
    config: u8 = 0;
    frame_count: u8 = 0;
    frame_duration_us: u32 = 0;
    total_duration_us: u32 = 0;
    total_samples_48k: u32 = 0;
    is_vbr: bool = false;
    has_padding: bool = false;
    padding_bytes: usize = 0;
    is_dtx: bool = false;
    has_vad: bool = false;
    has_lbrr_fec: bool = false;
}

/// Diagnoses mode, bandwidth, duration, padding, DTX, VAD, and In-Band LBRR FEC for an Opus packet.
pub fn diagnoseOpusPacket(
    packet: []const u8,
    scratch_frames: []OpusFrameSlice,
): OpusPacketDiagnostics ! OpusError {
    const parsed = parseOpusPacketFrames(packet, scratch_frames)?;
    let all_dtx: bool = true;
    let any_vad: bool = false;
    let any_lbrr: bool = false;

    let i: usize = 0;
    while (i < parsed.frame_count) : (i += 1) {
        const sig = inspectOpusFrameSignal(parsed.toc, scratch_frames[i].data);
        if (!sig.is_dtx) all_dtx = false;
        if (sig.has_vad) any_vad = true;
        if (sig.has_lbrr_fec) any_lbrr = true;
    }

    return OpusPacketDiagnostics{
        mode: parsed.toc.mode,
        bandwidth: parsed.toc.bandwidth,
        channels: parsed.toc.channels(),
        config: parsed.toc.config,
        frame_count: parsed.toc.frame_count,
        frame_duration_us: parsed.toc.frame_duration_us,
        total_duration_us: parsed.total_duration_us,
        total_samples_48k: parsed.total_samples_48k,
        is_vbr: parsed.toc.is_vbr,
        has_padding: parsed.toc.has_padding,
        padding_bytes: parsed.padding_bytes,
        is_dtx: all_dtx,
        has_vad: any_vad,
        has_lbrr_fec: any_lbrr,
    };
}

// ============================================================================
// 4. RFC 7587 WebRTC Opus SDP `fmtp` & RTP Packetizer / Unpacker
// ============================================================================

/// RFC 7587 §6.1 WebRTC Opus SDP `a=fmtp` negotiation parameters.
pub struct OpusSdpFmtpConfig {
    payload_type: u8 = 111;
    minptime: u16 = 0;
    maxplaybackrate: u32 = 0;
    maxaveragebitrate: u32 = 0;
    has_stereo: bool = false;
    stereo: bool = false;
    has_sprop_stereo: bool = false;
    sprop_stereo: bool = false;
    has_cbr: bool = false;
    cbr: bool = false;
    has_useinbandfec: bool = false;
    useinbandfec: bool = false;
    has_usedtx: bool = false;
    usedtx: bool = false;
}

fn strEq(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) : (i += 1) {
        if (a[i] != b[i]) return false;
    }
    return true;
}

fn trimSpaces(s: []const u8): []const u8 {
    let start: usize = 0;
    let end: usize = s.len;
    while (start < end && (s[start] == ' ' || s[start] == '\t')) : (start += 1) {}
    while (end > start && (s[end - 1] == ' ' || s[end - 1] == '\t' || s[end - 1] == '\r' || s[end - 1] == '\n')) : (end -= 1) {}
    return s[start..end];
}

fn parseUintDec(s: []const u8): u32 ! OpusError {
    if (s.len == 0) return OpusError.InvalidFmtp;
    let acc: u64 = 0;
    let i: usize = 0;
    while (i < s.len) : (i += 1) {
        const c = s[i];
        if (c < '0' || c > '9') return OpusError.InvalidFmtp;
        acc = acc * 10 + ((c - '0') as u64);
        if (acc > 0xFFFFFFFF) return OpusError.InvalidFmtp;
    }
    return acc as u32;
}

/// Parses an RFC 7587 Opus SDP `a=fmtp:<pt> ...` line (or bare `k=v;k=v` parameter string).
pub fn parseOpusSdpFmtp(line: []const u8): OpusSdpFmtpConfig ! OpusError {
    let cfg = OpusSdpFmtpConfig{};
    let body = trimSpaces(line);
    if (body.len == 0) return OpusError.InvalidFmtp;

    // Optional "a=fmtp:<pt> " prefix
    if (body.len >= 7 && strEq(body[0..7], "a=fmtp:")) {
        let sp: usize = 7;
        while (sp < body.len && body[sp] != ' ') : (sp += 1) {}
        if (sp == 7) return OpusError.InvalidFmtp;
        const pt_val = parseUintDec(body[7..sp])?;
        if (pt_val > 127) return OpusError.InvalidFmtp;
        cfg.payload_type = pt_val as u8;
        body = if (sp < body.len) trimSpaces(body[(sp + 1)..body.len]) else "";
    }

    let pos: usize = 0;
    while (pos < body.len) {
        let semi: usize = pos;
        while (semi < body.len && body[semi] != ';') : (semi += 1) {}
        const token = trimSpaces(body[pos..semi]);
        pos = if (semi < body.len) (semi + 1) else body.len;
        if (token.len == 0) continue;

        let eq: usize = 0;
        while (eq < token.len && token[eq] != '=') : (eq += 1) {}
        if (eq == 0 || eq + 1 >= token.len) return OpusError.InvalidFmtp;
        const key = trimSpaces(token[0..eq]);
        const val = trimSpaces(token[(eq + 1)..token.len]);
        const num = parseUintDec(val)?;

        if (strEq(key, "minptime")) {
            cfg.minptime = num as u16;
        } else if (strEq(key, "maxplaybackrate")) {
            cfg.maxplaybackrate = num;
        } else if (strEq(key, "maxaveragebitrate")) {
            cfg.maxaveragebitrate = num;
        } else if (strEq(key, "stereo")) {
            cfg.has_stereo = true;
            cfg.stereo = num != 0;
        } else if (strEq(key, "sprop-stereo")) {
            cfg.has_sprop_stereo = true;
            cfg.sprop_stereo = num != 0;
        } else if (strEq(key, "cbr")) {
            cfg.has_cbr = true;
            cfg.cbr = num != 0;
        } else if (strEq(key, "useinbandfec")) {
            cfg.has_useinbandfec = true;
            cfg.useinbandfec = num != 0;
        } else if (strEq(key, "usedtx")) {
            cfg.has_usedtx = true;
            cfg.usedtx = num != 0;
        }
    }
    return cfg;
}

fn appendStr(out: []u8, pos: usize, s: []const u8): usize ! OpusError {
    if (pos + s.len > out.len) return OpusError.BufferTooSmall;
    copyBytes(out, pos, s);
    return pos + s.len;
}

fn appendU32Dec(out: []u8, pos: usize, v: u32): usize ! OpusError {
    let digits: [10]u8 = [0; 10];
    let n: usize = 0;
    let x: u32 = v;
    if (x == 0) {
        digits[0] = '0';
        n = 1;
    } else {
        while (x > 0) {
            digits[n] = ('0' as u8) + ((x % 10) as u8);
            x /= 10;
            n += 1;
        }
    }
    if (pos + n > out.len) return OpusError.BufferTooSmall;
    let i: usize = 0;
    while (i < n) : (i += 1) {
        out[pos + i] = digits[n - 1 - i];
    }
    return pos + n;
}

fn appendFmtpParam(out: []u8, pos: usize, first: *bool, key: []const u8, val: u32): usize ! OpusError {
    let cur: usize = pos;
    if (!first.*) {
        cur = appendStr(out, cur, ";")?;
    } else {
        first.* = false;
    }
    cur = appendStr(out, cur, key)?;
    cur = appendStr(out, cur, "=")?;
    return appendU32Dec(out, cur, val);
}

/// Formats a canonical RFC 7587 `a=fmtp:<pt> ...` SDP line into caller-provided `out`.
pub fn formatOpusSdpFmtp(cfg: OpusSdpFmtpConfig, out: []u8): usize ! OpusError {
    let pos: usize = 0;
    pos = appendStr(out, pos, "a=fmtp:")?;
    pos = appendU32Dec(out, pos, cfg.payload_type as u32)?;
    pos = appendStr(out, pos, " ")?;
    let first: bool = true;
    if (cfg.minptime > 0) pos = appendFmtpParam(out, pos, *first, "minptime", cfg.minptime as u32)?;
    if (cfg.maxplaybackrate > 0) pos = appendFmtpParam(out, pos, *first, "maxplaybackrate", cfg.maxplaybackrate)?;
    if (cfg.maxaveragebitrate > 0) pos = appendFmtpParam(out, pos, *first, "maxaveragebitrate", cfg.maxaveragebitrate)?;
    if (cfg.has_stereo) pos = appendFmtpParam(out, pos, *first, "stereo", if (cfg.stereo) 1 else 0)?;
    if (cfg.has_sprop_stereo) pos = appendFmtpParam(out, pos, *first, "sprop-stereo", if (cfg.sprop_stereo) 1 else 0)?;
    if (cfg.has_cbr) pos = appendFmtpParam(out, pos, *first, "cbr", if (cfg.cbr) 1 else 0)?;
    if (cfg.has_useinbandfec) pos = appendFmtpParam(out, pos, *first, "useinbandfec", if (cfg.useinbandfec) 1 else 0)?;
    if (cfg.has_usedtx) pos = appendFmtpParam(out, pos, *first, "usedtx", if (cfg.usedtx) 1 else 0)?;
    return pos;
}

/// RTP header metadata emitted per Opus packet by `OpusRtpPacketizer`.
pub struct OpusRtpEmitMeta {
    seq: u16 = 0;
    timestamp_48k: u32 = 0;
    marker: bool = false;
    samples_48k: u32 = 0;
    is_dtx: bool = false;
    has_lbrr_fec: bool = false;
}

/// RFC 7587 WebRTC Opus RTP packetizer managing 48kHz RTP timestamp progression,
/// post-DTX talkspurt `Marker` bit signaling, and multi-packet `ptime` aggregation.
pub struct OpusRtpPacketizer {
    seq: u16 = 0;
    timestamp_48k: u32 = 0;
    in_dtx_silence: bool = true;
    packets_sent: u64 = 0;
    dtx_packets_seen: u64 = 0;

    static fn init(initial_seq: u16, initial_ts_48k: u32): OpusRtpPacketizer {
        return OpusRtpPacketizer{
            seq: initial_seq,
            timestamp_48k: initial_ts_48k,
            in_dtx_silence: true,
            packets_sent: 0,
            dtx_packets_seen: 0,
        };
    }

    /// Computes RTP sequence, 48kHz timestamp, and talkspurt Marker bit for `opus_packet`,
    /// advancing internal state cleanly across 16-bit/32-bit wrap-around.
    fn nextPacketMeta(*_, opus_packet: []const u8, scratch_frames: []OpusFrameSlice): OpusRtpEmitMeta ! OpusError {
        const diag = diagnoseOpusPacket(opus_packet, scratch_frames)?;
        const cur_seq: u16 = seq;
        const cur_ts: u32 = timestamp_48k;
        let marker: bool = false;

        if (diag.is_dtx) {
            in_dtx_silence = true;
            dtx_packets_seen += 1;
        } else {
            if (in_dtx_silence) {
                marker = true; // First active voice packet after silence/start
                in_dtx_silence = false;
            }
        }

        seq = seq +% 1;
        timestamp_48k = timestamp_48k +% diag.total_samples_48k;
        packets_sent += 1;

        return OpusRtpEmitMeta{
            seq: cur_seq,
            timestamp_48k: cur_ts,
            marker: marker,
            samples_48k: diag.total_samples_48k,
            is_dtx: diag.is_dtx,
            has_lbrr_fec: diag.has_lbrr_fec,
        };
    }

    /// Aggregates two compatible Opus packets (same TOC `config` & `is_stereo`, e.g. 20ms + 20ms -> 40ms)
    /// into one compact Opus packet, enforcing RFC 6716 <=120ms duration cap.
    static fn aggregatePackets(
        pkt_a: []const u8,
        pkt_b: []const u8,
        scratch_a: []OpusFrameSlice,
        scratch_b: []OpusFrameSlice,
        scratch_all: []OpusFrameSlice,
        out_pkt: []u8,
    ): usize ! OpusError {
        const pa = parseOpusPacketFrames(pkt_a, scratch_a)?;
        const pb = parseOpusPacketFrames(pkt_b, scratch_b)?;
        if (pa.toc.config != pb.toc.config || pa.toc.is_stereo != pb.toc.is_stereo) {
            return OpusError.MixedTocConfig;
        }
        const total_cnt: usize = pa.frame_count + pb.frame_count;
        if (total_cnt > scratch_all.len) return OpusError.SpanOverflow;
        let i: usize = 0;
        while (i < pa.frame_count) : (i += 1) {
            scratch_all[i] = scratch_a[i];
        }
        let j: usize = 0;
        while (j < pb.frame_count) : (j += 1) {
            scratch_all[pa.frame_count + j] = scratch_b[j];
        }
        return repackOpusFrames(scratch_all[0..total_cnt], OpusRepackOptions{
            config: pa.toc.config,
            is_stereo: pa.toc.is_stereo,
        }, out_pkt);
    }
}

/// RFC 7587 WebRTC Opus RTP unpacker tracking DTX/LBRR-FEC statistics and splitting multi-frame
/// packets (e.g. 40ms = 2x20ms) into standalone Code 0 single-frame Opus packets.
pub struct OpusRtpUnpacker {
    packets_received: u64 = 0;
    fec_packets_seen: u64 = 0;
    dtx_packets_seen: u64 = 0;
    total_samples_48k: u64 = 0;

    static fn init(): OpusRtpUnpacker {
        return OpusRtpUnpacker{};
    }

    /// Observes an incoming Opus RTP payload and returns its diagnostics while updating stats.
    fn observePacket(*_, opus_packet: []const u8, scratch_frames: []OpusFrameSlice): OpusPacketDiagnostics ! OpusError {
        const diag = diagnoseOpusPacket(opus_packet, scratch_frames)?;
        packets_received += 1;
        total_samples_48k += diag.total_samples_48k as u64;
        if (diag.has_lbrr_fec) fec_packets_seen += 1;
        if (diag.is_dtx) dtx_packets_seen += 1;
        return diag;
    }

    /// Splits a multi-frame Opus packet into individual Code 0 single-frame Opus packets
    /// inside caller-provided `out_slab` and `out_spans`, returning the number of packets produced.
    static fn splitToSingleFramePackets(
        opus_packet: []const u8,
        scratch_frames: []OpusFrameSlice,
        out_slab: []u8,
        out_spans: []OpusFrameSlice,
    ): usize ! OpusError {
        const parsed = parseOpusPacketFrames(opus_packet, scratch_frames)?;
        if (out_spans.len < parsed.frame_count) return OpusError.SpanOverflow;
        const toc_code0: u8 = (parsed.toc.config << 3) | (if (parsed.toc.is_stereo) 0x04 else 0x00);
        let dst_off: usize = 0;
        let i: usize = 0;
        while (i < parsed.frame_count) : (i += 1) {
            const f = scratch_frames[i].data;
            const plen: usize = f.len + 1;
            if (dst_off + plen > out_slab.len) return OpusError.BufferTooSmall;
            out_slab[dst_off] = toc_code0;
            copyBytes(out_slab, dst_off + 1, f);
            out_spans[i] = OpusFrameSlice{
                offset: dst_off,
                len: plen,
                data: out_slab[dst_off..(dst_off + plen)],
            };
            dst_off += plen;
        }
        return parsed.frame_count;
    }
}
