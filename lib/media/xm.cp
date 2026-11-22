// Origin: Pure-CP Stdlib (Zero-Alloc FastTracker II Extended Module .xm v1.04 Parser) | Created: 2026-11-22

/// FastTracker II XM 文件魔数常量
pub const XM_MAGIC: []const u8 = "Extended Module: ";
pub const XM_SUB_BYTE: u8 = 0x1A;
pub const XM_MIN_HEADER_SIZE: usize = 80;
pub const XM_MAX_CHANNELS: u16 = 32;

/// FastTracker II XM 文件头零拷贝视图
pub struct XmHeader {
    module_name: []const u8,
    tracker_name: []const u8,
    version: u16,
    header_size: u32,
    song_length: u16,
    restart_position: u16,
    num_channels: u16,
    num_patterns: u16,
    num_instruments: u16,
    flags: u16,
    default_tempo: u16,
    default_bpm: u16,
    pattern_order_table: []const u8,
    patterns_offset: usize,

    pub fn versionMajor(): u8 {
        return ((_.version >> 8) & 0xFF) as u8;
    }

    pub fn versionMinor(): u8 {
        return (_.version & 0xFF) as u8;
    }

    /// bit 0 == 1 表示 Linear 频率表，0 表示 Amiga 频率表
    pub fn isLinearFreqTable(): bool {
        return (_.flags & 1) != 0;
    }
}

/// 单个 XM Pattern 块零拷贝视图
pub struct XmPattern {
    index: u16,
    header_len: u32,
    packing_type: u8,
    num_rows: u16,
    packed_data_size: u16,
    packed_data: []const u8,
}

/// 单个 XM Instrument 块零拷贝视图
pub struct XmInstrument {
    index: u16,
    header_size: u32,
    name: []const u8,
    inst_type: u8,
    num_samples: u16,
    sample_header_size: u32,
    total_sample_bytes: usize,
}

fn readU16Le(data: []const u8, off: usize): u16 {
    return u16(data[off]) | (u16(data[off + 1]) << 8);
}

fn readU32Le(data: []const u8, off: usize): u32 {
    return u32(data[off]) |
        (u32(data[off + 1]) << 8) |
        (u32(data[off + 2]) << 16) |
        (u32(data[off + 3]) << 24);
}

fn trimXmName(raw: []const u8): []const u8 {
    let end: usize = 0;
    while (end < raw.len && raw[end] != 0) {
        end += 1;
    }
    while (end > 0 && raw[end - 1] == ' ') {
        end -= 1;
    }
    return raw[0..end];
}

/// 校验 XM 魔数 `"Extended Module: "`（0..17）、`0x1A`（偏移 37）与最小长度 `80`
pub fn isXm(data: []const u8): bool {
    if (data.len < XM_MIN_HEADER_SIZE) return false;
    let i: usize = 0;
    while (i < XM_MAGIC.len) {
        if (data[i] != XM_MAGIC[i]) return false;
        i += 1;
    }
    return data[37] == XM_SUB_BYTE;
}

/// 解析并严格校验 FastTracker II XM 文件头（校验 `header_size >= 20`、`60 + header_size <= data.len` 及 `1 <= num_channels <= 32`）
pub fn parseXmHeader(data: []const u8): XmHeader? {
    if (!isXm(data)) return null;

    const version = readU16Le(data, 58);
    const header_size = readU32Le(data, 60);
    if (header_size < 20) return null;
    if (usize(header_size) > data.len - 60) return null;

    const song_length = readU16Le(data, 64);
    const restart_position = readU16Le(data, 66);
    const num_channels = readU16Le(data, 68);
    if (num_channels == 0 || num_channels > XM_MAX_CHANNELS) return null;

    const num_patterns = readU16Le(data, 70);
    const num_instruments = readU16Le(data, 72);
    const flags = readU16Le(data, 74);
    const default_tempo = readU16Le(data, 76);
    const default_bpm = readU16Le(data, 78);

    const patterns_offset: usize = usize(header_size) + 60;
    const max_order_end: usize = 80 + 256;
    const order_end = if (patterns_offset > max_order_end) max_order_end else patterns_offset;

    return XmHeader {
        module_name: trimXmName(data[17..37]),
        tracker_name: trimXmName(data[38..58]),
        version: version,
        header_size: header_size,
        song_length: song_length,
        restart_position: restart_position,
        num_channels: num_channels,
        num_patterns: num_patterns,
        num_instruments: num_instruments,
        flags: flags,
        default_tempo: default_tempo,
        default_bpm: default_bpm,
        pattern_order_table: data[80..order_end],
        patterns_offset: patterns_offset,
    };
}

/// 零堆分配 XM Pattern 块迭代器（迭代完成后 `instruments_offset` 指向首个 Instrument 起始偏移）
pub struct XmPatternIter {
    data: []const u8,
    pos: usize,
    index: u16 = 0,
    total: u16,
    instruments_offset: usize,
    had_error: bool = false,

    pub static fn init(data: []const u8, hdr: XmHeader): XmPatternIter {
        return XmPatternIter {
            data: data,
            pos: hdr.patterns_offset,
            index: 0,
            total: hdr.num_patterns,
            instruments_offset: hdr.patterns_offset,
            had_error: false,
        };
    }

    pub fn next(*_): XmPattern? {
        if (_.had_error || _.index >= _.total) return null;
        if (9 > _.data.len - _.pos) {
            _.had_error = true;
            return null;
        }

        const header_len = readU32Le(_.data, _.pos);
        if (header_len < 9 || usize(header_len) > _.data.len - _.pos) {
            _.had_error = true;
            return null;
        }

        const packing_type = _.data[_.pos + 4];
        if (packing_type != 0) {
            _.had_error = true;
            return null;
        }

        const num_rows = readU16Le(_.data, _.pos + 5);
        const packed_size = readU16Le(_.data, _.pos + 7);

        const data_start = _.pos + usize(header_len);
        if (usize(packed_size) > _.data.len - data_start) {
            _.had_error = true;
            return null;
        }

        const packed_bytes = _.data[data_start .. data_start + usize(packed_size)];
        const cur_idx = _.index;
        _.index += 1;
        _.pos = data_start + usize(packed_size);
        _.instruments_offset = _.pos;

        return XmPattern {
            index: cur_idx,
            header_len: header_len,
            packing_type: packing_type,
            num_rows: num_rows,
            packed_data_size: packed_size,
            packed_data: packed_bytes,
        };
    }
}

/// 零堆分配 XM Instrument 块迭代器（自动跳过含 `num_samples > 0` 的全部 Sample 头与 PCM 数据区）
pub struct XmInstrumentIter {
    data: []const u8,
    pos: usize,
    index: u16 = 0,
    total: u16,
    had_error: bool = false,

    pub static fn init(data: []const u8, instruments_offset: usize, num_instruments: u16): XmInstrumentIter {
        return XmInstrumentIter {
            data: data,
            pos: instruments_offset,
            index: 0,
            total: num_instruments,
            had_error: false,
        };
    }

    pub fn next(*_): XmInstrument? {
        if (_.had_error || _.index >= _.total) return null;
        if (29 > _.data.len - _.pos) {
            _.had_error = true;
            return null;
        }

        const inst_hdr_size = readU32Le(_.data, _.pos);
        if (inst_hdr_size < 29 || usize(inst_hdr_size) > _.data.len - _.pos) {
            _.had_error = true;
            return null;
        }

        const name = trimXmName(_.data[_.pos + 4 .. _.pos + 26]);
        const inst_type = _.data[_.pos + 26];
        const num_samples = readU16Le(_.data, _.pos + 27);

        let sample_hdr_size: u32 = 0;
        let total_pcm: usize = 0;
        let scan = _.pos + usize(inst_hdr_size);

        if (num_samples > 0) {
            if (inst_hdr_size < 33) {
                _.had_error = true;
                return null;
            }
            sample_hdr_size = readU32Le(_.data, _.pos + 29);
            if (sample_hdr_size < 4) {
                _.had_error = true;
                return null;
            }

            // 依次读取 `num_samples` 个 Sample 头，累加其 4B 样本字节长度
            let si: u16 = 0;
            while (si < num_samples) {
                if (usize(sample_hdr_size) > _.data.len - scan) {
                    _.had_error = true;
                    return null;
                }
                const sample_bytes = usize(readU32Le(_.data, scan));
                if (sample_bytes > _.data.len - total_pcm) {
                    _.had_error = true;
                    return null;
                }
                total_pcm += sample_bytes;
                scan += usize(sample_hdr_size);
                si += 1;
            }

            // 跳过紧随 Sample 头之后的全部样本 PCM 数据区
            if (total_pcm > _.data.len - scan) {
                _.had_error = true;
                return null;
            }
            scan += total_pcm;
        }

        const cur_idx = _.index;
        _.index += 1;
        _.pos = scan;

        return XmInstrument {
            index: cur_idx,
            header_size: inst_hdr_size,
            name: name,
            inst_type: inst_type,
            num_samples: num_samples,
            sample_header_size: sample_hdr_size,
            total_sample_bytes: total_pcm,
        };
    }
}
