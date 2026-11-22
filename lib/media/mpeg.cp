// Origin: Pure-CP Stdlib (MPEG-1/2/2.5 Layer I/II/III, CRC16, Xing/LAME/VBRI TOC Seek, Layer III Side-Info & RFC 2250 MPA RTP) | Created: 2026-11-22
// 100% pure CP, zero dynamic heap allocation, zero external FFI dependencies, caller-slice MP3/MPEG audio core:
//   - 100% backwards-compatible `MpegVersion`, `MpegLayer`, `MpegChannelMode`, `MpegFrameHeader`, `MpegFrame`,
//     `MpegFrameIter`, `XingHeader`, `MpegSummary`, `skipId3v2`, `parseMpegFrameHeader`, `isMpegAudio`, `parseXingHeader`, `summarizeMpeg`
//   - Full MPEG-1/2/2.5 Layer I/II/III header encoder (`encodeMp3FrameHeader`), ISO/IEC 11172-3 CRC16-IBM (`0x8005`)
//     verifier/stamper, trailing ID3v1 detection, and noise-resilient lookahead resync scanner (`Mp3StreamIter`)
//   - Full Xing/Info/LAME (100-byte TOC + Gapless `encoder_delay`/`encoder_padding`) & Fraunhofer VBRI parser,
//     O(1) `seekMp3ByteOffsetByMs` linear TOC/CBR seeking, and `scanMp3Stream` min/max/avg bitrate summary
//   - Layer III Side-Info bitstream parser (`main_data_begin`, `private_bits`, `scfsi`, `part2_3_length`, `big_values`, `global_gain`)
//   - RFC 2250 MPA RTP multi-frame aggregation/fragmentation packetizer (`packetizeMp3Rtp`) & depacketizer (`Mp3RtpDepacketizer`)

// ============================================================================
// 1. Constants, Error Set & Legacy Backwards-Compatible Types
// ============================================================================

/// ISO/IEC 11172-3 MPEG Audio CRC-16/IBM generator polynomial (`0x8005`).
pub const MP3_CRC16_POLY: u16 = 0x8005;

/// Trailing ID3v1 metadata tag size in bytes (`128`).
pub const MP3_ID3V1_TAG_SIZE: usize = 128;

/// Xing VBR 100-entry byte-offset TOC table size (`100`).
pub const MP3_XING_TOC_SIZE: usize = 100;

/// RFC 2250 §3.5 MPA RTP payload-specific header size in bytes (`4`: MBZ16 + Frag_offset16).
pub const MPA_RTP_HEADER_SIZE: usize = 4;

/// Error set returned by MPEG/MP3 encoding, CRC verification, VBR/Side-Info parsing, and RFC 2250 RTP routines.
pub err MpegError {
    BufferTooSmall,
    TruncatedData,
    InvalidHeader,
    InvalidBitrate,
    InvalidSampleRate,
    InvalidVersion,
    InvalidLayer,
    CrcMismatch,
    InvalidXingHeader,
    InvalidVbriHeader,
    InvalidSideInfo,
    InvalidRtpPacket,
    MtuTooSmall,
    SpanOverflow,
    InvalidParameter,
}

/// MPEG 音频版本枚举（对应帧头 bits 20..19）
pub enum MpegVersion: u8 {
    v2_5 = 0,
    reserved = 1,
    v2 = 2,
    v1 = 3,
}

/// MPEG 音频层级枚举（对应帧头 bits 18..17）
pub enum MpegLayer: u8 {
    reserved = 0,
    layer3 = 1,
    layer2 = 2,
    layer1 = 3,
}

/// MPEG 音频声道模式枚举（对应帧头 bits 7..6）
pub enum MpegChannelMode: u8 {
    stereo = 0,
    joint_stereo = 1,
    dual_channel = 2,
    mono = 3,
}

/// 解析后的 4 字节 MPEG 音频帧头与派生参数
pub struct MpegFrameHeader {
    version: MpegVersion,
    layer: MpegLayer,
    has_crc: bool,
    bitrate_idx: u8,
    bitrate_kbps: u32,
    sample_rate_idx: u8,
    sample_rate: u32,
    padding: bool,
    private_bit: bool,
    channel_mode: MpegChannelMode,
    mode_extension: u8,
    copyright: bool,
    original: bool,
    emphasis: u8,
    samples_per_frame: u32,
    frame_size: u32,

    /// 输出声道数（mono=1，其余模式=2）
    fn channels(): u8 {
        return if (_.channel_mode == MpegChannelMode.mono) 1 else 2;
    }

    /// 单帧时长（微秒）
    fn frameDurationUs(): u64 {
        return ((_.samples_per_frame as u64) * 1000000) / (_.sample_rate as u64);
    }
}

/// 零拷贝单帧 MPEG 音频视图
pub struct MpegFrame {
    offset: usize,
    header: MpegFrameHeader,
    data: []const u8,
}

/// Xing / Info / VBRI VBR 扩展头解析结果（向后兼容简版视图）
pub struct XingHeader {
    is_cbr: bool,
    is_vbri: bool,
    frames: u32?,
    bytes: u32?,
    quality: u32?,
}

/// MPEG 音频流汇总信息（向后兼容简版汇总）
pub struct MpegSummary {
    version: MpegVersion,
    layer: MpegLayer,
    sample_rate: u32,
    channels: u8,
    total_frames: u32,
    duration_ms: u64,
    avg_bitrate_kbps: u32,
    is_vbr: bool,
    has_xing: bool,
}

/// 零堆分配 MPEG 音频帧流式迭代器（自动跳过前置 ID3v2 标签）
pub struct MpegFrameIter {
    data: []const u8,
    pos: usize = 0,

    static fn init(data: []const u8): MpegFrameIter {
        return MpegFrameIter {
            data: data,
            pos: skipId3v2(data),
        };
    }

    /// 解析并返回下一帧；到达流尾或遇到无效帧头/截断帧时安全返回 `null`
    fn next(*_): MpegFrame? {
        if (_.pos >= _.data.len) return null;
        if (_.data.len - _.pos < 4) {
            _.pos = _.data.len;
            return null;
        }

        const rem = _.data[_.pos .. _.data.len];
        if (hdr := parseMpegFrameHeader(rem)) {
            const fsize: usize = hdr.frame_size as usize;
            if (fsize < 4 || fsize > _.data.len - _.pos) {
                _.pos = _.data.len;
                return null;
            }
            const start = _.pos;
            _.pos = start + fsize;
            return MpegFrame {
                offset: start,
                header: hdr,
                data: _.data[start .. start + fsize],
            };
        }

        _.pos = _.data.len;
        return null;
    }
}

fn readU16BE(data: []const u8, off: usize): u16 {
    return ((data[off] as u16) << 8) | (data[off + 1] as u16);
}

fn readU32BE(data: []const u8, off: usize): u32 {
    return ((data[off] as u32) << 24)
        | ((data[off + 1] as u32) << 16)
        | ((data[off + 2] as u32) << 8)
        | (data[off + 3] as u32);
}

fn copyBytes(dst: []u8, dst_off: usize, src: []const u8) {
    let i: usize = 0;
    while (i < src.len) : (i += 1) {
        dst[dst_off + i] = src[i];
    }
}

fn matchFourCC(data: []const u8, off: usize, a: u8, b: u8, c: u8, d: u8): bool {
    if (data.len < off + 4) return false;
    return data[off] == a && data[off + 1] == b && data[off + 2] == c && data[off + 3] == d;
}

/// 跳过可选的 ID3v2 前置标签（`"ID3"` + 4B synchsafe size + 可选 10B footer），返回音频帧起始偏移
pub fn skipId3v2(data: []const u8): usize {
    let pos: usize = 0;
    while (data.len - pos >= 10) {
        if (data[pos] != 'I' || data[pos + 1] != 'D' || data[pos + 2] != '3') {
            break;
        }
        if (data[pos + 3] == 0xFF || data[pos + 4] == 0xFF) {
            break;
        }
        const s0 = data[pos + 6];
        const s1 = data[pos + 7];
        const s2 = data[pos + 8];
        const s3 = data[pos + 9];
        if (((s0 | s1 | s2 | s3) & 0x80) != 0) {
            break;
        }
        const body_size: usize = (((s0 as usize) & 0x7F) << 21)
            | (((s1 as usize) & 0x7F) << 14)
            | (((s2 as usize) & 0x7F) << 7)
            | ((s3 as usize) & 0x7F);
        const footer_size: usize = if ((data[pos + 5] & 0x10) != 0) 10 else 0;
        const total: usize = body_size + footer_size + 10;
        if (total > data.len - pos) {
            return data.len;
        }
        pos += total;
    }
    return pos;
}

fn lookupBitrateKbps(version: MpegVersion, layer: MpegLayer, idx: u8): u32 {
    const i: usize = idx as usize;
    if (i == 0 || i >= 15) return 0;

    if (version == MpegVersion.v1) {
        if (layer == MpegLayer.layer1) {
            const tbl: [16]u32 = [0, 32, 64, 96, 128, 160, 192, 224, 256, 288, 320, 352, 384, 416, 448, 0];
            return tbl[i];
        } else if (layer == MpegLayer.layer2) {
            const tbl: [16]u32 = [0, 32, 48, 56, 64, 80, 96, 112, 128, 160, 192, 224, 256, 320, 384, 0];
            return tbl[i];
        } else {
            const tbl: [16]u32 = [0, 32, 40, 48, 56, 64, 80, 96, 112, 128, 160, 192, 224, 256, 320, 0];
            return tbl[i];
        }
    }

    if (layer == MpegLayer.layer1) {
        const tbl: [16]u32 = [0, 32, 48, 56, 64, 80, 96, 112, 128, 144, 160, 176, 192, 224, 256, 0];
        return tbl[i];
    }
    const tbl: [16]u32 = [0, 8, 16, 24, 32, 40, 48, 56, 64, 80, 96, 112, 128, 144, 160, 0];
    return tbl[i];
}

fn lookupSampleRate(version: MpegVersion, idx: u8): u32 {
    const i: usize = idx as usize;
    if (i >= 3) return 0;

    if (version == MpegVersion.v1) {
        const tbl: [3]u32 = [44100, 48000, 32000];
        return tbl[i];
    } else if (version == MpegVersion.v2) {
        const tbl: [3]u32 = [22050, 24000, 16000];
        return tbl[i];
    }
    const tbl: [3]u32 = [11025, 12000, 8000];
    return tbl[i];
}

/// 解析 4 字节 MPEG-1/2/2.5 Layer I/II/III 帧头并计算比特率、采样率、每帧采样数与精确帧字节长
pub fn parseMpegFrameHeader(data: []const u8): MpegFrameHeader? {
    if (data.len < 4) return null;

    const b0 = data[0];
    const b1 = data[1];
    const b2 = data[2];
    const b3 = data[3];

    if (b0 != 0xFF || (b1 & 0xE0) != 0xE0) return null;

    const ver_raw: u8 = (b1 >> 3) & 0x03;
    if (ver_raw == 1) return null;
    const version: MpegVersion = ver_raw as MpegVersion;

    const layer_raw: u8 = (b1 >> 1) & 0x03;
    if (layer_raw == 0) return null;
    const layer: MpegLayer = layer_raw as MpegLayer;

    const has_crc: bool = (b1 & 0x01) == 0;

    const bitrate_idx: u8 = (b2 >> 4) & 0x0F;
    if (bitrate_idx == 0 || bitrate_idx == 15) return null;

    const sr_idx: u8 = (b2 >> 2) & 0x03;
    if (sr_idx == 3) return null;

    const padding: bool = ((b2 >> 1) & 0x01) != 0;
    const private_bit: bool = (b2 & 0x01) != 0;

    const ch_raw: u8 = (b3 >> 6) & 0x03;
    const channel_mode: MpegChannelMode = ch_raw as MpegChannelMode;
    const mode_extension: u8 = (b3 >> 4) & 0x03;
    const copyright: bool = ((b3 >> 3) & 0x01) != 0;
    const original: bool = ((b3 >> 2) & 0x01) != 0;
    const emphasis: u8 = b3 & 0x03;

    const bitrate_kbps = lookupBitrateKbps(version, layer, bitrate_idx);
    const sample_rate = lookupSampleRate(version, sr_idx);
    if (bitrate_kbps == 0 || sample_rate == 0) return null;

    let samples_per_frame: u32 = 1152;
    if (layer == MpegLayer.layer1) {
        samples_per_frame = 384;
    } else if (layer == MpegLayer.layer2) {
        samples_per_frame = 1152;
    } else {
        samples_per_frame = if (version == MpegVersion.v1) 1152 else 576;
    }

    const bitrate_bps: u64 = (bitrate_kbps as u64) * 1000;
    const sr_u64: u64 = sample_rate as u64;
    const pad_u64: u64 = if (padding) 1 else 0;

    let frame_size_u64: u64 = 0;
    if (layer == MpegLayer.layer1) {
        frame_size_u64 = (((bitrate_bps * 12) / sr_u64) + pad_u64) * 4;
    } else {
        const coeff: u64 = (samples_per_frame as u64) / 8;
        frame_size_u64 = ((coeff * bitrate_bps) / sr_u64) + pad_u64;
    }

    if (frame_size_u64 < 4) return null;

    return MpegFrameHeader {
        version: version,
        layer: layer,
        has_crc: has_crc,
        bitrate_idx: bitrate_idx,
        bitrate_kbps: bitrate_kbps,
        sample_rate_idx: sr_idx,
        sample_rate: sample_rate,
        padding: padding,
        private_bit: private_bit,
        channel_mode: channel_mode,
        mode_extension: mode_extension,
        copyright: copyright,
        original: original,
        emphasis: emphasis,
        samples_per_frame: samples_per_frame,
        frame_size: frame_size_u64 as u32,
    };
}

/// 探测缓冲区是否为有效 MPEG 音频流（自动跳过 ID3v2；若存在后续帧则同步校验连续同步字）
pub fn isMpegAudio(data: []const u8): bool {
    const start = skipId3v2(data);
    if (start >= data.len || data.len - start < 4) return false;

    if (hdr := parseMpegFrameHeader(data[start .. data.len])) {
        const fsize: usize = hdr.frame_size as usize;
        if (fsize > data.len - start) return false;

        const next_off = start + fsize;
        if (data.len - next_off >= 4) {
            return parseMpegFrameHeader(data[next_off .. data.len]) != null;
        }
        return true;
    }
    return false;
}

fn parseXingAt(frame_data: []const u8, off: usize): XingHeader? {
    if (frame_data.len < off + 8) return null;

    const is_xing = matchFourCC(frame_data, off, 'X', 'i', 'n', 'g');
    const is_info = matchFourCC(frame_data, off, 'I', 'n', 'f', 'o');
    if (!is_xing && !is_info) return null;

    const flags = readU32BE(frame_data, off + 4);
    let pos: usize = off + 8;

    let frames: u32? = null;
    let bytes: u32? = null;
    let quality: u32? = null;

    if ((flags & 0x0001) != 0) {
        if (frame_data.len - pos < 4) return null;
        frames = readU32BE(frame_data, pos);
        pos += 4;
    }
    if ((flags & 0x0002) != 0) {
        if (frame_data.len - pos < 4) return null;
        bytes = readU32BE(frame_data, pos);
        pos += 4;
    }
    if ((flags & 0x0004) != 0) {
        if (frame_data.len - pos < 100) return null;
        pos += 100;
    }
    if ((flags & 0x0008) != 0) {
        if (frame_data.len - pos < 4) return null;
        quality = readU32BE(frame_data, pos);
        pos += 4;
    }

    return XingHeader {
        is_cbr: is_info,
        is_vbri: false,
        frames: frames,
        bytes: bytes,
        quality: quality,
    };
}

/// 在 MPEG-1/2 Layer III 帧中解析 `"Xing"` / `"Info"` 或 `"VBRI"` VBR 扩展头（向后兼容接口）
pub fn parseXingHeader(frame_data: []const u8, header: MpegFrameHeader): XingHeader? {
    if (header.layer != MpegLayer.layer3) return null;

    const is_mono = header.channel_mode == MpegChannelMode.mono;
    let side_off: usize = 36;
    if (header.version == MpegVersion.v1) {
        side_off = if (is_mono) 21 else 36;
    } else {
        side_off = if (is_mono) 13 else 21;
    }

    if (xing := parseXingAt(frame_data, side_off)) {
        return xing;
    }
    if (header.has_crc) {
        if (xing := parseXingAt(frame_data, side_off + 2)) {
            return xing;
        }
    }

    if (frame_data.len >= 36 + 18 && matchFourCC(frame_data, 36, 'V', 'B', 'R', 'I')) {
        const quality_u16 = readU16BE(frame_data, 36 + 8);
        const total_bytes = readU32BE(frame_data, 36 + 10);
        const total_frames = readU32BE(frame_data, 36 + 14);
        return XingHeader {
            is_cbr: false,
            is_vbri: true,
            frames: total_frames,
            bytes: total_bytes,
            quality: quality_u16 as u32,
        };
    }

    return null;
}

/// 汇总 MPEG 音频流元信息与时长（向后兼容接口）
pub fn summarizeMpeg(data: []const u8): MpegSummary? {
    let it = MpegFrameIter.init(data);
    const first = it.next() ?? return null;

    const sr = first.header.sample_rate;
    const spf = first.header.samples_per_frame;
    const ch = first.header.channels();

    if (xing := parseXingHeader(first.data, first.header)) {
        if (total_frames := xing.frames) {
            const total_samples: u64 = (total_frames as u64) * (spf as u64);
            const duration_ms: u64 = (total_samples * 1000) / (sr as u64);
            let avg_kbps: u32 = first.header.bitrate_kbps;
            if (total_bytes := xing.bytes) {
                if (duration_ms > 0) {
                    avg_kbps = (((total_bytes as u64) * 8) / duration_ms) as u32;
                }
            }
            return MpegSummary {
                version: first.header.version,
                layer: first.header.layer,
                sample_rate: sr,
                channels: ch,
                total_frames: total_frames,
                duration_ms: duration_ms,
                avg_bitrate_kbps: avg_kbps,
                is_vbr: !xing.is_cbr,
                has_xing: true,
            };
        }
    }

    let frame_count: u32 = 1;
    let total_samples: u64 = spf as u64;
    let bitrate_sum: u64 = first.header.bitrate_kbps as u64;
    let is_vbr: bool = false;
    const first_bitrate = first.header.bitrate_kbps;

    while (frame := it.next()) {
        frame_count += 1;
        total_samples += frame.header.samples_per_frame as u64;
        bitrate_sum += frame.header.bitrate_kbps as u64;
        if (frame.header.bitrate_kbps != first_bitrate) {
            is_vbr = true;
        }
    }

    const duration_ms: u64 = (total_samples * 1000) / (sr as u64);
    const avg_kbps: u32 = (bitrate_sum / (frame_count as u64)) as u32;

    return MpegSummary {
        version: first.header.version,
        layer: first.header.layer,
        sample_rate: sr,
        channels: ch,
        total_frames: frame_count,
        duration_ms: duration_ms,
        avg_bitrate_kbps: avg_kbps,
        is_vbr: is_vbr,
        has_xing: false,
    };
}

// ============================================================================
// 2. MP3 Frame Header Encoder, CRC16-IBM (0x8005) & Noise-Resilient Resync Scanner
// ============================================================================

/// Parameters for encoding a 4-byte MPEG-1/2/2.5 Layer I/II/III frame header.
pub struct Mp3HeaderSpec {
    version: MpegVersion = MpegVersion.v1;
    layer: MpegLayer = MpegLayer.layer3;
    has_crc: bool = false;
    bitrate_kbps: u32 = 128;
    sample_rate: u32 = 44100;
    padding: bool = false;
    private_bit: bool = false;
    channel_mode: MpegChannelMode = MpegChannelMode.joint_stereo;
    mode_extension: u8 = 0;
    copyright: bool = false;
    original: bool = true;
    emphasis: u8 = 0;
}

/// Encodes a 4-byte MPEG-1/2/2.5 Layer I/II/III frame header into `out`.
pub fn encodeMp3FrameHeader(spec: Mp3HeaderSpec, out: []u8): usize ! MpegError {
    if (out.len < 4) return MpegError.BufferTooSmall;
    if (spec.version == MpegVersion.reserved) return MpegError.InvalidVersion;
    if (spec.layer == MpegLayer.reserved) return MpegError.InvalidLayer;

    let br_idx: u8 = 0;
    let bi: u8 = 1;
    while (bi < 15) : (bi += 1) {
        if (lookupBitrateKbps(spec.version, spec.layer, bi) == spec.bitrate_kbps) {
            br_idx = bi;
            break;
        }
    }
    if (br_idx == 0) return MpegError.InvalidBitrate;

    let sr_idx: u8 = 3;
    let si: u8 = 0;
    while (si < 3) : (si += 1) {
        if (lookupSampleRate(spec.version, si) == spec.sample_rate) {
            sr_idx = si;
            break;
        }
    }
    if (sr_idx >= 3) return MpegError.InvalidSampleRate;

    const ver_bits: u8 = spec.version as u8;
    const lay_bits: u8 = spec.layer as u8;
    const ch_bits: u8 = spec.channel_mode as u8;

    const crc_bit: u8 = if (spec.has_crc) 0x00 else 0x01;
    out[0] = 0xFF;
    out[1] = (((ver_bits & 0x03) << 3) | ((lay_bits & 0x03) << 1) | crc_bit) | 0xE0;
    out[2] = ((br_idx & 0x0F) << 4)
        | ((sr_idx & 0x03) << 2)
        | (if (spec.padding) 0x02 else 0x00)
        | (if (spec.private_bit) 0x01 else 0x00);
    out[3] = ((ch_bits & 0x03) << 6)
        | ((spec.mode_extension & 0x03) << 4)
        | (if (spec.copyright) 0x08 else 0x00)
        | (if (spec.original) 0x04 else 0x00)
        | (spec.emphasis & 0x03);
    return 4;
}

/// Returns the Layer III Side-Info length in bytes (`32`, `17`, or `9`), or `0` for non-Layer III.
pub fn mp3SideInfoLen(header: MpegFrameHeader): usize {
    if (header.layer != MpegLayer.layer3) return 0;
    const is_mono = header.channel_mode == MpegChannelMode.mono;
    if (header.version == MpegVersion.v1) {
        return if (is_mono) 17 else 32;
    }
    return if (is_mono) 9 else 17;
}

fn mp3Crc16Update(init_crc: u16, data: []const u8): u16 {
    let crc: u16 = init_crc;
    let i: usize = 0;
    while (i < data.len) : (i += 1) {
        crc ^= (data[i] as u16) << 8;
        let b: usize = 0;
        while (b < 8) : (b += 1) {
            if ((crc & 0x8000) != 0) {
                crc = (crc << 1) ^ MP3_CRC16_POLY;
            } else {
                crc = crc << 1;
            }
        }
    }
    return crc;
}

/// Computes ISO/IEC 11172-3 §2.4.3.1 CRC16-IBM (`0x8005`, init `0xFFFF`) over header bytes `[2..4]`
/// and Layer III Side-Info `[6..(6 + side_info_len)]`.
pub fn computeMp3Crc16(frame: []const u8, header: MpegFrameHeader): u16 ! MpegError {
    if (!header.has_crc) return 0;
    const side_len: usize = mp3SideInfoLen(header);
    const need: usize = side_len + 6;
    if (frame.len < need) return MpegError.TruncatedData;
    let crc: u16 = mp3Crc16Update(0xFFFF, frame[2..4]);
    if (side_len > 0) {
        crc = mp3Crc16Update(crc, frame[6..need]);
    }
    return crc;
}

/// Verifies the embedded 16-bit CRC at `frame[4..6]` when `has_crc == true`.
pub fn verifyMp3FrameCrc(frame: []const u8): bool ! MpegError {
    const hdr = parseMpegFrameHeader(frame) orelse return MpegError.InvalidHeader;
    if (!hdr.has_crc) return true;
    const computed = computeMp3Crc16(frame, hdr)?;
    const stored: u16 = readU16BE(frame, 4);
    return stored == computed;
}

/// Computes and writes the 16-bit CRC into `frame[4..6]` for a CRC-protected MP3 frame.
pub fn stampMp3FrameCrc(frame: []u8): u16 ! MpegError {
    const hdr = parseMpegFrameHeader(frame) orelse return MpegError.InvalidHeader;
    if (!hdr.has_crc) return 0;
    const crc = computeMp3Crc16(frame, hdr)?;
    frame[4] = ((crc >> 8) & 0xFF) as u8;
    frame[5] = (crc & 0xFF) as u8;
    return crc;
}

/// Returns `true` when `data` ends with a 128-byte ID3v1 `"TAG"` trailer.
pub fn hasTrailingId3v1(data: []const u8): bool {
    if (data.len < MP3_ID3V1_TAG_SIZE) return false;
    const off: usize = data.len - MP3_ID3V1_TAG_SIZE;
    return data[off] == 'T' && data[off + 1] == 'A' && data[off + 2] == 'G';
}

/// Noise-resilient MP3 stream iterator: skips leading ID3v2 tags, ignores trailing 128-byte ID3v1 tag,
/// skips leading garbage bytes, and validates next-frame syncword lookahead.
pub struct Mp3StreamIter {
    data: []const u8 = "";
    pos: usize = 0;
    audio_end: usize = 0;
    bytes_skipped: usize = 0;
    frames_yielded: u64 = 0;

    static fn init(buf: []const u8): Mp3StreamIter {
        const start = skipId3v2(buf);
        const end = if (hasTrailingId3v1(buf) && buf.len - MP3_ID3V1_TAG_SIZE >= start) (buf.len - MP3_ID3V1_TAG_SIZE) else buf.len;
        return Mp3StreamIter{
            data: buf,
            pos: start,
            audio_end: end,
            bytes_skipped: 0,
            frames_yielded: 0,
        };
    }

    fn next(*_): MpegFrame? {
        while (pos + 4 <= audio_end) {
            if (hdr := parseMpegFrameHeader(data[pos..audio_end])) {
                const fsize: usize = hdr.frame_size as usize;
                if (fsize >= 4 && pos + fsize <= audio_end) {
                    const next_pos: usize = pos + fsize;
                    // Lookahead validation when another frame follows
                    const ok_next: bool = (next_pos == audio_end)
                        || (next_pos + 4 <= audio_end && parseMpegFrameHeader(data[next_pos..audio_end]) != null);
                    if (ok_next) {
                        const start: usize = pos;
                        pos = next_pos;
                        frames_yielded += 1;
                        return MpegFrame{
                            offset: start,
                            header: hdr,
                            data: data[start..next_pos],
                        };
                    }
                }
            }
            pos += 1;
            bytes_skipped += 1;
        }
        return null;
    }
}

// ============================================================================
// 3. Full Xing/Info/LAME (TOC + Gapless), VBRI, O(1) TOC Seeking & Stream Scan
// ============================================================================

/// Full Xing / Info VBR header with 100-entry byte-offset TOC and LAME Gapless delay/padding.
pub struct Mp3XingHeader {
    is_info: bool = false;
    flags: u32 = 0;
    has_frames: bool = false;
    frames: u32 = 0;
    has_bytes: bool = false;
    bytes: u32 = 0;
    has_toc: bool = false;
    toc: [100]u8 = [0; 100];
    has_quality: bool = false;
    quality: u32 = 0;
    has_lame: bool = false;
    lame_version: [9]u8 = [0; 9];
    encoder_delay: u16 = 0;
    encoder_padding: u16 = 0;
}

/// Parses a full Xing / Info + optional LAME header (with 100-byte TOC and 12-bit/12-bit Gapless delay/padding).
pub fn parseMp3XingHeader(frame_data: []const u8, header: MpegFrameHeader): Mp3XingHeader ! MpegError {
    if (header.layer != MpegLayer.layer3) return MpegError.InvalidXingHeader;
    const base_off: usize = mp3SideInfoLen(header) + 4;
    let off: usize = base_off;
    if (!(matchFourCC(frame_data, off, 'X', 'i', 'n', 'g') || matchFourCC(frame_data, off, 'I', 'n', 'f', 'o'))) {
        if (header.has_crc) {
            off = base_off + 2;
        }
        if (!(matchFourCC(frame_data, off, 'X', 'i', 'n', 'g') || matchFourCC(frame_data, off, 'I', 'n', 'f', 'o'))) {
            return MpegError.InvalidXingHeader;
        }
    }
    if (off + 8 > frame_data.len) return MpegError.TruncatedData;

    let res = Mp3XingHeader{
        is_info: matchFourCC(frame_data, off, 'I', 'n', 'f', 'o'),
        flags: readU32BE(frame_data, off + 4),
    };
    let pos: usize = off + 8;

    if ((res.flags & 0x0001) != 0) {
        if (pos + 4 > frame_data.len) return MpegError.TruncatedData;
        res.has_frames = true;
        res.frames = readU32BE(frame_data, pos);
        pos += 4;
    }
    if ((res.flags & 0x0002) != 0) {
        if (pos + 4 > frame_data.len) return MpegError.TruncatedData;
        res.has_bytes = true;
        res.bytes = readU32BE(frame_data, pos);
        pos += 4;
    }
    if ((res.flags & 0x0004) != 0) {
        if (pos + MP3_XING_TOC_SIZE > frame_data.len) return MpegError.TruncatedData;
        res.has_toc = true;
        let ti: usize = 0;
        while (ti < MP3_XING_TOC_SIZE) : (ti += 1) {
            res.toc[ti] = frame_data[pos + ti];
        }
        pos += MP3_XING_TOC_SIZE;
    }
    if ((res.flags & 0x0008) != 0) {
        if (pos + 4 > frame_data.len) return MpegError.TruncatedData;
        res.has_quality = true;
        res.quality = readU32BE(frame_data, pos);
        pos += 4;
    }

    // Optional LAME tag (starts immediately after Xing fields: "LAME" + 5B version, +21..+23 = 12b delay | 12b padding)
    if (pos + 24 <= frame_data.len && matchFourCC(frame_data, pos, 'L', 'A', 'M', 'E')) {
        res.has_lame = true;
        let li: usize = 0;
        while (li < 9) : (li += 1) {
            res.lame_version[li] = frame_data[pos + li];
        }
        const b0: u16 = frame_data[pos + 21] as u16;
        const b1: u16 = frame_data[pos + 22] as u16;
        const b2: u16 = frame_data[pos + 23] as u16;
        res.encoder_delay = (b0 << 4) | ((b1 >> 4) & 0x0F);
        res.encoder_padding = ((b1 & 0x0F) << 8) | b2;
    }
    return res;
}

/// Full Fraunhofer VBRI header (fixed offset 36 inside first MPEG-1 Layer III frame).
pub struct Mp3VbriHeader {
    version: u16 = 0;
    delay: u16 = 0;
    quality: u16 = 0;
    bytes: u32 = 0;
    frames: u32 = 0;
    toc_entries: u16 = 0;
    toc_scale: u16 = 0;
    toc_entry_bytes: u16 = 0;
    toc_frames_per_entry: u16 = 0;
}

/// Parses a Fraunhofer `VBRI` VBR header located at offset 36 of `frame_data`.
pub fn parseVbriHeader(frame_data: []const u8): Mp3VbriHeader ! MpegError {
    if (frame_data.len < 62) return MpegError.InvalidVbriHeader;
    if (!matchFourCC(frame_data, 36, 'V', 'B', 'R', 'I')) return MpegError.InvalidVbriHeader;
    return Mp3VbriHeader{
        version: readU16BE(frame_data, 40),
        delay: readU16BE(frame_data, 42),
        quality: readU16BE(frame_data, 44),
        bytes: readU32BE(frame_data, 46),
        frames: readU32BE(frame_data, 50),
        toc_entries: readU16BE(frame_data, 54),
        toc_scale: readU16BE(frame_data, 56),
        toc_entry_bytes: readU16BE(frame_data, 58),
        toc_frames_per_entry: readU16BE(frame_data, 60),
    };
}

/// O(1) byte-offset seek by millisecond timestamp using Xing 100-entry TOC linear interpolation (or CBR formula).
pub fn seekMp3ByteOffsetByMs(
    target_ms: u64,
    duration_ms: u64,
    total_bytes: u64,
    xing: Mp3XingHeader?,
): u64 {
    if (duration_ms == 0 || total_bytes == 0 || target_ms == 0) return 0;
    if (target_ms >= duration_ms) return total_bytes;

    if (xh := xing) {
        if (xh.has_toc) {
            const pct: f64 = ((target_ms as f64) * 100.0) / (duration_ms as f64);
            let idx: usize = pct as usize;
            if (idx > 99) idx = 99;
            const fa: f64 = xh.toc[idx] as f64;
            const fb: f64 = if (idx < 99) (xh.toc[idx + 1] as f64) else 256.0;
            const frac: f64 = pct - (idx as f64);
            const fx: f64 = fa + (fb - fa) * frac;
            return ((fx / 256.0) * (total_bytes as f64)) as u64;
        }
    }
    return (target_ms * total_bytes) / duration_ms;
}

/// Detailed MP3 stream scan summary including min/max/avg bitrate, VBR/Xing/VBRI flags & LAME Gapless trim.
pub struct Mp3StreamSummary {
    version: MpegVersion = MpegVersion.v1;
    layer: MpegLayer = MpegLayer.layer3;
    sample_rate: u32 = 0;
    channels: u8 = 0;
    total_frames: u32 = 0;
    total_audio_bytes: usize = 0;
    duration_ms: u64 = 0;
    avg_bitrate_kbps: u32 = 0;
    min_bitrate_kbps: u32 = 0;
    max_bitrate_kbps: u32 = 0;
    is_vbr: bool = false;
    has_xing: bool = false;
    has_vbri: bool = false;
    has_lame: bool = false;
    encoder_delay: u16 = 0;
    encoder_padding: u16 = 0;
    trimmed_samples: u64 = 0;
}

/// Scans an MP3 stream (handling ID3v2/ID3v1, Xing/LAME, VBRI, and full frame bitrate statistics).
pub fn scanMp3Stream(data: []const u8): Mp3StreamSummary ! MpegError {
    let it = Mp3StreamIter.init(data);
    const first = it.next() orelse return MpegError.InvalidHeader;

    let out = Mp3StreamSummary{
        version: first.header.version,
        layer: first.header.layer,
        sample_rate: first.header.sample_rate,
        channels: first.header.channels(),
        total_frames: 1,
        total_audio_bytes: first.data.len,
        min_bitrate_kbps: first.header.bitrate_kbps,
        max_bitrate_kbps: first.header.bitrate_kbps,
    };

    let xing_frames: u32 = 0;
    let xing_bytes: u32 = 0;
    if (xh := parseMp3XingHeader(first.data, first.header) catch null) {
        out.has_xing = true;
        out.is_vbr = !xh.is_info;
        if (xh.has_frames) xing_frames = xh.frames;
        if (xh.has_bytes) xing_bytes = xh.bytes;
        if (xh.has_lame) {
            out.has_lame = true;
            out.encoder_delay = xh.encoder_delay;
            out.encoder_padding = xh.encoder_padding;
        }
    } else if (vb := parseVbriHeader(first.data) catch null) {
        out.has_vbri = true;
        out.is_vbr = true;
        xing_frames = vb.frames;
        xing_bytes = vb.bytes;
    }

    let br_sum: u64 = first.header.bitrate_kbps as u64;
    let total_samples: u64 = first.header.samples_per_frame as u64;
    const first_br: u32 = first.header.bitrate_kbps;

    while (frame := it.next()) {
        out.total_frames += 1;
        out.total_audio_bytes += frame.data.len;
        const br = frame.header.bitrate_kbps;
        br_sum += br as u64;
        if (br < out.min_bitrate_kbps) out.min_bitrate_kbps = br;
        if (br > out.max_bitrate_kbps) out.max_bitrate_kbps = br;
        if (br != first_br) out.is_vbr = true;
        total_samples += frame.header.samples_per_frame as u64;
    }

    if ((out.has_xing || out.has_vbri) && xing_frames > 0) {
        out.total_frames = xing_frames;
        total_samples = (xing_frames as u64) * (first.header.samples_per_frame as u64);
    }

    const trim: u64 = (out.encoder_delay as u64) + (out.encoder_padding as u64);
    out.trimmed_samples = if (total_samples > trim) (total_samples - trim) else 0;
    out.duration_ms = (out.trimmed_samples * 1000) / (out.sample_rate as u64);

    if ((out.has_xing || out.has_vbri) && xing_bytes > 0 && out.duration_ms > 0) {
        out.avg_bitrate_kbps = (((xing_bytes as u64) * 8) / out.duration_ms) as u32;
    } else {
        out.avg_bitrate_kbps = (br_sum / (out.total_frames as u64)) as u32;
    }
    return out;
}

// ============================================================================
// 4. Layer III Side-Info Bitstream Parser
// ============================================================================

struct Mp3BitReader {
    data: []const u8 = "";
    bit_pos: usize = 0;

    fn readBits(*_, count: usize): u32 ! MpegError {
        if (count == 0) return 0;
        if (count > 32 || bit_pos + count > data.len * 8) return MpegError.TruncatedData;
        let acc: u32 = 0;
        let i: usize = 0;
        while (i < count) : (i += 1) {
            const bi: usize = bit_pos >> 3;
            const sh: u3 = @as(u3, @intCast(7 - (bit_pos & 7)));
            acc = (acc << 1) | (((data[bi] >> sh) & 1) as u32);
            bit_pos += 1;
        }
        return acc;
    }

    fn skipBits(*_, count: usize): bool ! MpegError {
        if (bit_pos + count > data.len * 8) return MpegError.TruncatedData;
        bit_pos += count;
        return true;
    }
}

/// Per-granule, per-channel Layer III Side-Info fields (ISO/IEC 11172-3 §2.4.1.7).
pub struct Mp3GranuleChannelInfo {
    part2_3_length: u16 = 0;
    big_values: u16 = 0;
    global_gain: u8 = 0;
    scalefac_compress: u16 = 0;
    window_switching_flag: bool = false;
}

/// Decoded Layer III Side-Info header (`main_data_begin`, `private_bits`, `scfsi` & granule descriptors).
pub struct Mp3Layer3SideInfo {
    main_data_begin: u16 = 0;
    private_bits: u8 = 0;
    scfsi: [2]u8 = [0, 0];
    num_granules: u8 = 0;
    num_channels: u8 = 0;
    granules: [4]Mp3GranuleChannelInfo = [Mp3GranuleChannelInfo{}; 4];
}

/// Parses MPEG-1 / MPEG-2 / MPEG-2.5 Layer III Side-Info from `frame_data`.
pub fn parseLayer3SideInfo(frame_data: []const u8, header: MpegFrameHeader): Mp3Layer3SideInfo ! MpegError {
    if (header.layer != MpegLayer.layer3) return MpegError.InvalidSideInfo;
    const side_off: usize = if (header.has_crc) 6 else 4;
    const side_len: usize = mp3SideInfoLen(header);
    if (side_off + side_len > frame_data.len) return MpegError.TruncatedData;

    let br = Mp3BitReader{ data: frame_data[side_off..(side_off + side_len)], bit_pos: 0 };
    const is_v1: bool = header.version == MpegVersion.v1;
    const ch_cnt: u8 = header.channels();
    const gr_cnt: u8 = if (is_v1) 2 else 1;

    let info = Mp3Layer3SideInfo{
        num_granules: gr_cnt,
        num_channels: ch_cnt,
    };

    if (is_v1) {
        info.main_data_begin = (br.readBits(9)?) as u16;
        info.private_bits = (br.readBits(if (ch_cnt == 1) 5 else 3)?) as u8;
        let c: usize = 0;
        while (c < (ch_cnt as usize)) : (c += 1) {
            info.scfsi[c] = (br.readBits(4)?) as u8;
        }
    } else {
        info.main_data_begin = (br.readBits(8)?) as u16;
        info.private_bits = (br.readBits(if (ch_cnt == 1) 1 else 2)?) as u8;
    }

    let gr: usize = 0;
    while (gr < (gr_cnt as usize)) : (gr += 1) {
        let ch: usize = 0;
        while (ch < (ch_cnt as usize)) : (ch += 1) {
            const p23: u16 = (br.readBits(12)?) as u16;
            const bv: u16 = (br.readBits(9)?) as u16;
            const gg: u8 = (br.readBits(8)?) as u8;
            const sfc: u16 = (br.readBits(if (is_v1) 4 else 9)?) as u16;
            const wsf: bool = (br.readBits(1)?) != 0;
            info.granules[gr * 2 + ch] = Mp3GranuleChannelInfo{
                part2_3_length: p23,
                big_values: bv,
                global_gain: gg,
                scalefac_compress: sfc,
                window_switching_flag: wsf,
            };
            // Skip remainder of granule-channel record (25 bits in MPEG-1, 29 bits in MPEG-2/2.5)
            // MPEG-1 total per gr/ch = 59 bits; read so far = 12+9+8+4+1 = 34 -> 25 remaining
            // MPEG-2 total per gr/ch = 63 bits; read so far = 12+9+8+9+1 = 39 -> 24 remaining
            * = br.skipBits(if (is_v1) 25 else 24)?;
        }
    }
    return info;
}

// ============================================================================
// 5. RFC 2250 MPA (MPEG Audio) RTP Packetizer & Depacketizer
// ============================================================================

/// Zero-copy span of one RFC 2250 MPA RTP payload inside a caller-provided slab.
pub struct Mp3RtpSpan {
    offset: usize = 0;
    len: usize = 0;
    frag_offset: u16 = 0;

    fn slice(slab: []const u8): []const u8 {
        return slab[offset..(offset + len)];
    }
}

/// Packetizes an MP3 stream into MTU-bounded RFC 2250 MPA RTP payloads (`[MBZ:16 | Frag_offset:16]`).
/// Aggregates multiple frames when fitting within `mtu` and fragments oversized frames with `Frag_offset`.
pub fn packetizeMp3Rtp(
    mp3_stream: []const u8,
    mtu: usize,
    out_buf: []u8,
    out_packets: []Mp3RtpSpan,
): usize ! MpegError {
    if (mtu <= MPA_RTP_HEADER_SIZE) return MpegError.MtuTooSmall;
    const max_pay: usize = mtu - MPA_RTP_HEADER_SIZE;

    let it = Mp3StreamIter.init(mp3_stream);
    let pkt_count: usize = 0;
    let dst_off: usize = 0;

    let cur_pkt_start: usize = 0;
    let cur_pkt_pay: usize = 0;
    let pkt_open: bool = false;

    while (frame := it.next()) {
        const fdata = frame.data;
        if (fdata.len > max_pay) {
            // Flush any open aggregated packet first
            if (pkt_open) {
                if (pkt_count >= out_packets.len) return MpegError.SpanOverflow;
                out_packets[pkt_count] = Mp3RtpSpan{
                    offset: cur_pkt_start,
                    len: cur_pkt_pay + MPA_RTP_HEADER_SIZE,
                    frag_offset: 0,
                };
                pkt_count += 1;
                pkt_open = false;
                cur_pkt_pay = 0;
            }
            // Fragment oversized frame across consecutive RFC 2250 packets
            let foff: usize = 0;
            while (foff < fdata.len) {
                if (pkt_count >= out_packets.len) return MpegError.SpanOverflow;
                const rem: usize = fdata.len - foff;
                const take: usize = if (rem > max_pay) max_pay else rem;
                const plen: usize = take + MPA_RTP_HEADER_SIZE;
                if (dst_off + plen > out_buf.len) return MpegError.BufferTooSmall;
                out_buf[dst_off] = 0;
                out_buf[dst_off + 1] = 0;
                out_buf[dst_off + 2] = ((foff >> 8) & 0xFF) as u8;
                out_buf[dst_off + 3] = (foff & 0xFF) as u8;
                copyBytes(out_buf, dst_off + MPA_RTP_HEADER_SIZE, fdata[foff..(foff + take)]);
                out_packets[pkt_count] = Mp3RtpSpan{
                    offset: dst_off,
                    len: plen,
                    frag_offset: foff as u16,
                };
                dst_off += plen;
                foff += take;
                pkt_count += 1;
            }
            continue;
        }

        // Flush open packet if adding this frame would exceed MTU
        if (pkt_open && cur_pkt_pay + fdata.len > max_pay) {
            if (pkt_count >= out_packets.len) return MpegError.SpanOverflow;
            out_packets[pkt_count] = Mp3RtpSpan{
                offset: cur_pkt_start,
                len: cur_pkt_pay + MPA_RTP_HEADER_SIZE,
                frag_offset: 0,
            };
            pkt_count += 1;
            pkt_open = false;
            cur_pkt_pay = 0;
        }

        if (!pkt_open) {
            if (dst_off + MPA_RTP_HEADER_SIZE + fdata.len > out_buf.len) return MpegError.BufferTooSmall;
            cur_pkt_start = dst_off;
            out_buf[dst_off] = 0;
            out_buf[dst_off + 1] = 0;
            out_buf[dst_off + 2] = 0;
            out_buf[dst_off + 3] = 0;
            dst_off += MPA_RTP_HEADER_SIZE;
            pkt_open = true;
            cur_pkt_pay = 0;
        }

        if (dst_off + fdata.len > out_buf.len) return MpegError.BufferTooSmall;
        copyBytes(out_buf, dst_off, fdata);
        dst_off += fdata.len;
        cur_pkt_pay += fdata.len;
    }

    if (pkt_open) {
        if (pkt_count >= out_packets.len) return MpegError.SpanOverflow;
        out_packets[pkt_count] = Mp3RtpSpan{
            offset: cur_pkt_start,
            len: cur_pkt_pay + MPA_RTP_HEADER_SIZE,
            frag_offset: 0,
        };
        pkt_count += 1;
    }
    return pkt_count;
}

/// Zero-heap RFC 2250 MPA RTP depacketizer supporting aggregated multi-frame packets and fragmented MP3 frames.
pub struct Mp3RtpDepacketizer {
    buf: []u8 = [];
    written: usize = 0;
    expected_frame_len: usize = 0;
    packets_received: u64 = 0;
    frames_dropped: u64 = 0;

    static fn init(storage: []u8): Mp3RtpDepacketizer ! MpegError {
        if (storage.len == 0) return MpegError.BufferTooSmall;
        return Mp3RtpDepacketizer{ buf: storage };
    }

    fn bytes(): []const u8 {
        return buf[0..written];
    }

    /// Feeds one RFC 2250 MPA RTP payload; returns `true` when complete MP3 frame(s) are ready in `bytes()`.
    fn pushRtpPacket(*_, rtp_payload: []const u8): bool ! MpegError {
        if (rtp_payload.len <= MPA_RTP_HEADER_SIZE) return MpegError.InvalidRtpPacket;
        const mbz: u16 = readU16BE(rtp_payload, 0);
        if (mbz != 0) return MpegError.InvalidRtpPacket;
        const frag_off: usize = readU16BE(rtp_payload, 2) as usize;
        const data = rtp_payload[MPA_RTP_HEADER_SIZE..rtp_payload.len];
        packets_received += 1;

        if (frag_off == 0) {
            if (expected_frame_len > 0 && written < expected_frame_len) {
                frames_dropped += 1;
            }
            if (data.len > buf.len) return MpegError.BufferTooSmall;
            copyBytes(buf, 0, data);
            written = data.len;
            if (hdr := parseMpegFrameHeader(data)) {
                expected_frame_len = hdr.frame_size as usize;
                if (written >= expected_frame_len) {
                    expected_frame_len = 0;
                    return true;
                }
                return false; // Fragmented frame in progress
            }
            return MpegError.InvalidHeader;
        }

        // Continuation fragment
        if (expected_frame_len == 0 || frag_off != written) {
            frames_dropped += 1;
            written = 0;
            expected_frame_len = 0;
            return false;
        }
        if (written + data.len > buf.len) return MpegError.BufferTooSmall;
        copyBytes(buf, written, data);
        written += data.len;
        if (written >= expected_frame_len) {
            expected_frame_len = 0;
            return true;
        }
        return false;
    }
}
