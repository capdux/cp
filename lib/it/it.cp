// Source: Absorbed from /data/dev/bun/src/ (Zero-Alloc Impulse Tracker 2.14 .it Binary Module Parser)

/// Impulse Tracker 魔数与结构常量
pub const IT_MAGIC: []const u8 = "IMPM";
pub const IT_INST_MAGIC: []const u8 = "IMPI";
pub const IT_SAMPLE_MAGIC: []const u8 = "IMPS";
pub const IT_HEADER_SIZE: usize = 192;
pub const IT_SAMPLE_HEADER_SIZE: usize = 80;

/// IT 头部 flags / special 标志位常量
pub const IT_FLAG_STEREO: u16 = 0x01;
pub const IT_FLAG_USE_INSTRUMENTS: u16 = 0x04;
pub const IT_FLAG_LINEAR_SLIDES: u16 = 0x08;
pub const IT_SPECIAL_HAS_MESSAGE: u16 = 0x01;

/// Impulse Tracker 192B 文件头零拷贝视图
pub struct ItHeader {
    song_name: []const u8,
    order_count: u16,
    instrument_count: u16,
    sample_count: u16,
    pattern_count: u16,
    created_with_version: u16,
    compatible_version: u16,
    flags: u16,
    special: u16,
    global_volume: u8,
    mix_volume: u8,
    initial_speed: u8,
    initial_tempo: u8,
    pan_separation: u8,
    message_length: u16,
    message_offset: u32,
    orders_offset: usize,
    instruments_table_offset: usize,
    samples_table_offset: usize,
    patterns_table_offset: usize,

    pub fn isStereo(): bool {
        return (_.flags & IT_FLAG_STEREO) != 0;
    }

    pub fn usesInstruments(): bool {
        return (_.flags & IT_FLAG_USE_INSTRUMENTS) != 0;
    }

    pub fn usesLinearSlides(): bool {
        return (_.flags & IT_FLAG_LINEAR_SLIDES) != 0;
    }

    pub fn hasMessage(): bool {
        return (_.special & IT_SPECIAL_HAS_MESSAGE) != 0;
    }

    /// 零拷贝提取可选曲目备注文本（无备注或越界返回 `null`）
    pub fn songMessage(data: []const u8): ([]const u8)? {
        if (!_.hasMessage() || _.message_length == 0) return null;
        const off = usize(_.message_offset);
        const mlen = usize(_.message_length);
        if (off > data.len || mlen > data.len - off) return null;
        return trimZstr(data[off .. off + mlen]);
    }

    /// 零拷贝返回 Order 表字节切片
    pub fn ordersSlice(data: []const u8): []const u8 {
        const count = usize(_.order_count);
        if (_.orders_offset > data.len || count > data.len - _.orders_offset) {
            return "";
        }
        return data[_.orders_offset .. _.orders_offset + count];
    }

    /// 查询第 `idx` 个 Instrument 的 32-bit 文件偏移
    pub fn instrumentOffset(data: []const u8, idx: usize): usize? {
        if (idx >= usize(_.instrument_count)) return null;
        const off = _.instruments_table_offset + idx * 4;
        if (4 > data.len - off) return null;
        return usize(readU32Le(data, off));
    }

    /// 查询第 `idx` 个 Sample 的 32-bit 文件偏移
    pub fn sampleOffset(data: []const u8, idx: usize): usize? {
        if (idx >= usize(_.sample_count)) return null;
        const off = _.samples_table_offset + idx * 4;
        if (4 > data.len - off) return null;
        return usize(readU32Le(data, off));
    }

    /// 查询第 `idx` 个 Pattern 的 32-bit 文件偏移（0 表示标准空 64 行 Pattern）
    pub fn patternOffset(data: []const u8, idx: usize): usize? {
        if (idx >= usize(_.pattern_count)) return null;
        const off = _.patterns_table_offset + idx * 4;
        if (4 > data.len - off) return null;
        return usize(readU32Le(data, off));
    }
}

/// 80B `"IMPS"` 采样头零拷贝视图
pub struct ItSample {
    dos_filename: []const u8,
    global_volume: u8,
    flags: u8,
    default_volume: u8,
    sample_name: []const u8,
    length: u32,
    loop_begin: u32,
    loop_end: u32,
    c5_speed: u32,
    sample_pointer: u32,

    pub fn hasData(): bool {
        return (_.flags & 0x01) != 0;
    }

    pub fn is16Bit(): bool {
        return (_.flags & 0x02) != 0;
    }

    pub fn isStereo(): bool {
        return (_.flags & 0x04) != 0;
    }

    pub fn isCompressed(): bool {
        return (_.flags & 0x08) != 0;
    }

    pub fn hasLoop(): bool {
        return (_.flags & 0x10) != 0;
    }
}

/// `"IMPI"` 乐器头零拷贝视图
pub struct ItInstrument {
    dos_filename: []const u8,
    global_volume: u8,
    default_pan: u8,
    name: []const u8,
}

/// Impulse Tracker Pattern 块零拷贝视图
pub struct ItPattern {
    is_empty: bool,
    rows: u16,
    packed_len: u16,
    packed_data: []const u8,
}

fn fourccEq(data: []const u8, off: usize, a: u8, b: u8, c: u8, d: u8): bool {
    return data[off] == a && data[off + 1] == b && data[off + 2] == c && data[off + 3] == d;
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

fn trimZstr(raw: []const u8): []const u8 {
    let end: usize = 0;
    while (end < raw.len && raw[end] != 0) {
        end += 1;
    }
    return raw[0..end];
}

/// 校验 Impulse Tracker `"IMPM"` 魔数与 192B 最小长度
pub fn isIt(data: []const u8): bool {
    if (data.len < IT_HEADER_SIZE) return false;
    return fourccEq(data, 0, 'I', 'M', 'P', 'M');
}

/// 解析 192B Impulse Tracker 文件头并严格校验 4 张偏移表不越界
pub fn parseItHeader(data: []const u8): ItHeader? {
    if (!isIt(data)) return null;

    const order_count = readU16Le(data, 32);
    const instrument_count = readU16Le(data, 34);
    const sample_count = readU16Le(data, 36);
    const pattern_count = readU16Le(data, 38);

    const orders_offset: usize = IT_HEADER_SIZE;
    const order_bytes = usize(order_count);
    if (order_bytes > data.len - orders_offset) return null;
    const inst_tbl_off = orders_offset + order_bytes;

    const inst_tbl_bytes = usize(instrument_count) * 4;
    if (inst_tbl_bytes > data.len - inst_tbl_off) return null;
    const smp_tbl_off = inst_tbl_off + inst_tbl_bytes;

    const smp_tbl_bytes = usize(sample_count) * 4;
    if (smp_tbl_bytes > data.len - smp_tbl_off) return null;
    const pat_tbl_off = smp_tbl_off + smp_tbl_bytes;

    const pat_tbl_bytes = usize(pattern_count) * 4;
    if (pat_tbl_bytes > data.len - pat_tbl_off) return null;

    return ItHeader {
        song_name: trimZstr(data[4..30]),
        order_count: order_count,
        instrument_count: instrument_count,
        sample_count: sample_count,
        pattern_count: pattern_count,
        created_with_version: readU16Le(data, 40),
        compatible_version: readU16Le(data, 42),
        flags: readU16Le(data, 44),
        special: readU16Le(data, 46),
        global_volume: data[48],
        mix_volume: data[49],
        initial_speed: data[50],
        initial_tempo: data[51],
        pan_separation: data[52],
        message_length: readU16Le(data, 54),
        message_offset: readU32Le(data, 56),
        orders_offset: orders_offset,
        instruments_table_offset: inst_tbl_off,
        samples_table_offset: smp_tbl_off,
        patterns_table_offset: pat_tbl_off,
    };
}

/// 解析第 `idx` 个 80B `"IMPS"` 采样头
pub fn parseItSample(data: []const u8, header: ItHeader, idx: usize): ItSample? {
    const off = header.sampleOffset(data, idx) ?? return null;
    if (off > data.len || IT_SAMPLE_HEADER_SIZE > data.len - off) return null;
    if (!fourccEq(data, off, 'I', 'M', 'P', 'S')) return null;

    return ItSample {
        dos_filename: trimZstr(data[off + 4 .. off + 16]),
        global_volume: data[off + 17],
        flags: data[off + 18],
        default_volume: data[off + 19],
        sample_name: trimZstr(data[off + 20 .. off + 46]),
        length: readU32Le(data, off + 48),
        loop_begin: readU32Le(data, off + 52),
        loop_end: readU32Le(data, off + 56),
        c5_speed: readU32Le(data, off + 60),
        sample_pointer: readU32Le(data, off + 72),
    };
}

/// 解析第 `idx` 个 `"IMPI"` 乐器头
pub fn parseItInstrument(data: []const u8, header: ItHeader, idx: usize): ItInstrument? {
    const off = header.instrumentOffset(data, idx) ?? return null;
    if (off > data.len || 58 > data.len - off) return null;
    if (!fourccEq(data, off, 'I', 'M', 'P', 'I')) return null;

    return ItInstrument {
        dos_filename: trimZstr(data[off + 4 .. off + 16]),
        global_volume: data[off + 24],
        default_pan: data[off + 25],
        name: trimZstr(data[off + 32 .. off + 58]),
    };
}

/// 解析第 `idx` 个 Pattern（偏移为 0 返回标准空 64 行 Pattern；否则校验 8B 头、`1 <= rows <= 200` 与数据边界）
pub fn parseItPattern(data: []const u8, header: ItHeader, idx: usize): ItPattern? {
    const off = header.patternOffset(data, idx) ?? return null;
    if (off == 0) {
        return ItPattern {
            is_empty: true,
            rows: 64,
            packed_len: 0,
            packed_data: "",
        };
    }
    if (off > data.len || 8 > data.len - off) return null;

    const plen = readU16Le(data, off);
    const rows = readU16Le(data, off + 2);
    if (rows == 0 || rows > 200) return null;

    const dstart = off + 8;
    if (usize(plen) > data.len - dstart) return null;

    return ItPattern {
        is_empty: false,
        rows: rows,
        packed_len: plen,
        packed_data: data[dstart .. dstart + usize(plen)],
    };
}
