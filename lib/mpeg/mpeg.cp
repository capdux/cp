// Source: Absorbed from /data/dev/bun/src/ (Zero-Alloc MPEG-1/2/2.5 Layer I/II/III & Xing/Info/VBRI Parser)

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

/// Xing / Info / VBRI VBR 扩展头解析结果
pub struct XingHeader {
    is_cbr: bool,
    is_vbri: bool,
    frames: u32?,
    bytes: u32?,
    quality: u32?,
}

/// MPEG 音频流汇总信息（优先采信 Xing/Info/VBRI，否则基于帧遍历计算）
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
        // 版本与修订号不能为 0xFF，且 4 字节长度必须为合法 synchsafe（最高位为 0）
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

    // MPEG-2 / MPEG-2.5
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

    // 11-bit 同步字必须为 0xFFE
    if (b0 != 0xFF || (b1 & 0xE0) != 0xE0) return null;

    const ver_raw: u8 = (b1 >> 3) & 0x03;
    if (ver_raw == 1) return null; // reserved
    const version: MpegVersion = ver_raw as MpegVersion;

    const layer_raw: u8 = (b1 >> 1) & 0x03;
    if (layer_raw == 0) return null; // reserved
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
        // Layer III: MPEG-1 为 1152，MPEG-2 / 2.5 为 576
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
        // 100 字节 TOC 寻址表
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

/// 在 MPEG-1/2 Layer III 帧中解析 `"Xing"` / `"Info"` 或 `"VBRI"` VBR 扩展头
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

    // Fraunhofer VBRI 头固定位于帧偏移 36 处（共 26 字节头部）
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

/// 汇总 MPEG 音频流元信息与时长（优先采信 Xing/Info/VBRI，否则基于全量帧迭代估算）
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
