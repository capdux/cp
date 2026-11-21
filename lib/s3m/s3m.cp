// Source: Absorbed from /data/dev/bun/src/ (Zero-Alloc Scream Tracker 3 S3M Module Parser)

/// S3M 文件魔数（偏移 44..48）
pub const S3M_MAGIC: []const u8 = "SCRM";

/// S3M 文件头固定大小（96 字节）
pub const S3M_HEADER_SIZE: usize = 96;

/// S3M 偏移 28 处的 DOS EOF 标记字节
pub const S3M_SUB_BYTE: u8 = 0x1A;

/// S3M 偏移 29 处的模块类型标识（16）
pub const S3M_TYPE_MODULE: u8 = 16;

/// S3M 单个乐器/采样头大小（80 字节）
pub const S3M_INST_SIZE: usize = 80;

/// Order 表控制字节：跳过标记（254）与结束标记（255）
pub const S3M_ORDER_SKIP: u8 = 254;
pub const S3M_ORDER_END: u8 = 255;

/// S3M 96 字节文件头及三张寻址表偏移信息
pub struct S3mHeader {
    song_name: []const u8,
    order_count: u16,
    instrument_count: u16,
    pattern_count: u16,
    flags: u16,
    tracker_version: u16,
    sample_format: u16,
    global_volume: u8,
    initial_speed: u8,
    initial_tempo: u8,
    master_volume: u8,
    channel_settings: [32]u8,
    orders_offset: usize,
    instruments_pp_offset: usize,
    patterns_pp_offset: usize,

    /// 检测主音量字节 bit 7 是否启用立体声
    pub fn isStereo(): bool {
        return (_.master_volume & 0x80) != 0;
    }

    /// 返回低 7 位主音量值（0..127）
    pub fn masterVolLevel(): u8 {
        return _.master_volume & 0x7F;
    }

    /// 检测采样格式是否为有符号（1=signed, 2=unsigned）
    pub fn isSignedSamples(): bool {
        return _.sample_format == 1;
    }

    /// 统计 32 个声道设置中启用（bit 7 == 0）的声道数量
    pub fn activeChannelCount(): u8 {
        let count: u8 = 0;
        let i: usize = 0;
        while (i < 32) {
            if ((_.channel_settings[i] & 0x80) == 0) {
                count += 1;
            }
            i += 1;
        }
        return count;
    }

    /// 零拷贝返回 Order 表切片
    pub fn ordersSlice(data: []const u8): []const u8 {
        const end = _.orders_offset + usize(_.order_count);
        if (end > data.len) {
            return "";
        }
        return data[_.orders_offset..end];
    }

    /// 计算第 `idx` 个乐器头的绝对字节偏移（`parapointer * 16`）并校验 80B 边界
    pub fn instrumentOffset(data: []const u8, idx: u16): usize? {
        if (idx >= _.instrument_count) {
            return null;
        }
        const pp_off = _.instruments_pp_offset + usize(idx) * 2;
        if (pp_off + 2 > data.len) {
            return null;
        }
        const pp = readU16LE(data, pp_off);
        if (pp == 0) {
            return null;
        }
        const abs_u64 = u64(pp) * 16;
        if (abs_u64 + u64(S3M_INST_SIZE) > u64(data.len)) {
            return null;
        }
        return usize(abs_u64);
    }

    /// 计算第 `idx` 个 Pattern 的绝对字节偏移（`parapointer * 16`）并校验 2B 长度头边界
    pub fn patternOffset(data: []const u8, idx: u16): usize? {
        if (idx >= _.pattern_count) {
            return null;
        }
        const pp_off = _.patterns_pp_offset + usize(idx) * 2;
        if (pp_off + 2 > data.len) {
            return null;
        }
        const pp = readU16LE(data, pp_off);
        if (pp == 0) {
            return null;
        }
        const abs_u64 = u64(pp) * 16;
        if (abs_u64 + 2 > u64(data.len)) {
            return null;
        }
        return usize(abs_u64);
    }
}

/// S3M 80 字节乐器/采样头解析结果
pub struct S3mInstrument {
    inst_type: u8,
    dos_filename: []const u8,
    title: []const u8,
    sample_data_offset: usize,
    length: u32,
    loop_begin: u32,
    loop_end: u32,
    volume: u8,
    flags: u8,
    c2_speed: u32,
    has_scrs_sig: bool,

    pub fn hasLoop(): bool {
        return (_.flags & 0x01) != 0;
    }

    pub fn isStereoSample(): bool {
        return (_.flags & 0x02) != 0;
    }

    pub fn is16BitSample(): bool {
        return (_.flags & 0x04) != 0;
    }
}

/// S3M 压缩 Pattern 零拷贝切片视图
pub struct S3mPattern {
    offset: usize,
    packed_length: u16,
    data: []const u8,
}

// ---------------------------------------------------------------------------
// 内部小端序读取与字节辅助函数
// ---------------------------------------------------------------------------

fn readU16LE(data: []const u8, offset: usize): u16 {
    const lo = u16(data[offset]);
    const hi = u16(data[offset + 1]);
    return lo | (hi << 8);
}

fn readU32LE(data: []const u8, offset: usize): u32 {
    const b0 = u32(data[offset]);
    const b1 = u32(data[offset + 1]);
    const b2 = u32(data[offset + 2]);
    const b3 = u32(data[offset + 3]);
    return b0 | (b1 << 8) | (b2 << 16) | (b3 << 24);
}

fn eqlBytes(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) {
        return false;
    }
    let i: usize = 0;
    while (i < a.len) {
        if (a[i] != b[i]) {
            return false;
        }
        i += 1;
    }
    return true;
}

fn trimNul(s: []const u8): []const u8 {
    let end: usize = 0;
    while (end < s.len && s[end] != 0) {
        end += 1;
    }
    return s[0..end];
}

// ---------------------------------------------------------------------------
// 1. 格式探测与 96B 文件头解析
// ---------------------------------------------------------------------------

/// 校验数据是否为合法 S3M 模块（len >= 96、offset 28 == 0x1A、offset 29 == 16、offset 44..48 == "SCRM"）
pub fn isS3m(data: []const u8): bool {
    if (data.len < S3M_HEADER_SIZE) {
        return false;
    }
    return data[28] == S3M_SUB_BYTE &&
        data[29] == S3M_TYPE_MODULE &&
        eqlBytes(data[44..48], S3M_MAGIC);
}

/// 零分配解析 96 字节 S3M 文件头，并严格校验 Order/Instrument/Pattern 三张表不越界
pub fn parseS3mHeader(data: []const u8): S3mHeader? {
    if (!isS3m(data)) {
        return null;
    }
    const order_count = readU16LE(data, 32);
    const instrument_count = readU16LE(data, 34);
    const pattern_count = readU16LE(data, 36);
    const flags = readU16LE(data, 38);
    const tracker_version = readU16LE(data, 40);
    const sample_format = readU16LE(data, 42);

    const orders_offset: u64 = S3M_HEADER_SIZE as u64;
    const instruments_pp_offset: u64 = orders_offset + u64(order_count);
    const patterns_pp_offset: u64 = instruments_pp_offset + u64(instrument_count) * 2;
    const tables_end: u64 = patterns_pp_offset + u64(pattern_count) * 2;
    if (tables_end > u64(data.len)) {
        return null;
    }

    let ch_settings: [32]u8 = [0; 32];
    let i: usize = 0;
    while (i < 32) {
        ch_settings[i] = data[64 + i];
        i += 1;
    }

    return S3mHeader {
        song_name: trimNul(data[0..28]),
        order_count: order_count,
        instrument_count: instrument_count,
        pattern_count: pattern_count,
        flags: flags,
        tracker_version: tracker_version,
        sample_format: sample_format,
        global_volume: data[48],
        initial_speed: data[49],
        initial_tempo: data[50],
        master_volume: data[51],
        channel_settings: ch_settings,
        orders_offset: usize(orders_offset),
        instruments_pp_offset: usize(instruments_pp_offset),
        patterns_pp_offset: usize(patterns_pp_offset),
    };
}

// ---------------------------------------------------------------------------
// 2. 80 字节乐器/采样头解析
// ---------------------------------------------------------------------------

/// 零分配解析第 `idx` 个 80 字节 S3M 乐器/采样头（支持 24-bit memseg 与 `"SCRS"` 签名校验）
pub fn parseS3mInstrument(data: []const u8, header: S3mHeader, idx: u16): S3mInstrument? {
    const off = header.instrumentOffset(data, idx) ?? return null;
    const inst_type = data[off];
    const dos_filename = trimNul(data[off + 1 .. off + 13]);
    const title = trimNul(data[off + 48 .. off + 76]);
    const has_scrs = eqlBytes(data[off + 76 .. off + 80], "SCRS");

    let sample_offset: usize = 0;
    let length: u32 = 0;
    let loop_begin: u32 = 0;
    let loop_end: u32 = 0;
    let volume: u8 = 0;
    let flags: u8 = 0;
    let c2_speed: u32 = 0;

    if (inst_type == 1) {
        const memseg_hi = u32(data[off + 13]);
        const memseg_lo = u32(readU16LE(data, off + 14));
        const memseg_24 = (memseg_hi << 16) | memseg_lo;
        sample_offset = usize(u64(memseg_24) * 16);
        length = readU32LE(data, off + 16);
        loop_begin = readU32LE(data, off + 20);
        loop_end = readU32LE(data, off + 24);
        volume = data[off + 28];
        flags = data[off + 31];
        c2_speed = readU32LE(data, off + 32);
    }

    return S3mInstrument {
        inst_type: inst_type,
        dos_filename: dos_filename,
        title: title,
        sample_data_offset: sample_offset,
        length: length,
        loop_begin: loop_begin,
        loop_end: loop_end,
        volume: volume,
        flags: flags,
        c2_speed: c2_speed,
        has_scrs_sig: has_scrs,
    };
}

// ---------------------------------------------------------------------------
// 3. Pattern 打包切片提取
// ---------------------------------------------------------------------------

/// 零分配解析第 `idx` 个 Pattern，校验 2B LE 打包长度（含自身 2B）并返回压缩负载切片
pub fn parseS3mPattern(data: []const u8, header: S3mHeader, idx: u16): S3mPattern? {
    const off = header.patternOffset(data, idx) ?? return null;
    const packed_len = readU16LE(data, off);
    if (packed_len < 2) {
        return null;
    }
    const end_u64 = u64(off) + u64(packed_len);
    if (end_u64 > u64(data.len)) {
        return null;
    }
    return S3mPattern {
        offset: off,
        packed_length: packed_len,
        data: data[off + 2 .. usize(end_u64)],
    };
}
