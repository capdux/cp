// Origin: Pure-CP Stdlib (Zero-Alloc SEG-Y Rev 1/2 Seismic Reflection Parser) | Created: 2026-11-22

/// SEG-Y 固定 3200 字节文本卷头长度（EBCDIC 或 ASCII）
pub const SEGY_TEXT_HEADER_SIZE: usize = 3200;

/// SEG-Y 固定 400 字节二进制卷头长度
pub const SEGY_BINARY_HEADER_SIZE: usize = 400;

/// SEG-Y 固定 240 字节地震道头长度
pub const SEGY_TRACE_HEADER_SIZE: usize = 240;

/// SEG-Y 最小卷头长度（3200B 文本头 + 400B 二进制卷头）
pub const SEGY_MIN_HEADER_SIZE: usize = 3600;

/// SEG-Y 采样数据格式代码（Binary Header 偏移 3224..3226）
pub enum SegySampleFormat: u16 {
    unknown = 0,
    ibm_f32 = 1,
    int32 = 2,
    int16 = 3,
    ieee_f32 = 5,
    ieee_f64 = 6,
    int8 = 8,
}

/// 返回指定采样格式的单样本字节数；未知格式返回 `null`
pub fn segySampleBytes(fmt: SegySampleFormat): usize? {
    return match (fmt) {
        .ibm_f32 => 4,
        .int32 => 4,
        .int16 => 2,
        .ieee_f32 => 4,
        .ieee_f64 => 8,
        .int8 => 1,
        .unknown => null,
    };
}

/// SEG-Y 400 字节二进制卷头（偏移 3200..3600）
pub struct SegyBinaryHeader {
    job_id: i32,
    line_number: i32,
    reel_number: i32,
    traces_per_ensemble: i16,
    sample_interval_us: u16,
    samples_per_trace: u16,
    sample_format: SegySampleFormat,
    measurement_system: i16,
    format_rev: u16,
    fixed_length_flag: i16,
    num_ext_text_headers: i16,
    traces_offset: usize,
    is_little_endian: bool,
}

/// SEG-Y 240 字节地震道头关键元数据
pub struct SegyTraceHeader {
    trace_seq_line: i32,
    trace_seq_file: i32,
    field_record: i32,
    cdp: i32,
    trace_id_code: i16,
    coord_scalar: i16,
    source_x: i32,
    source_y: i32,
    group_x: i32,
    group_y: i32,
    trace_samples: u16,
    trace_interval_us: u16,
}

/// 单条地震道（道头 + 零拷贝原始采样字节切片）
pub struct SegyTrace {
    header: SegyTraceHeader,
    raw_samples: []const u8,
    is_little_endian: bool,
}

/// 零分配地震道顺序迭代器
pub struct SegyTraceIter {
    data: []const u8,
    offset: usize,
    sample_format: SegySampleFormat,
    default_samples: u16,
    fixed_length: bool,
    is_little_endian: bool,
    had_error: bool,

    /// 从卷头 `traces_offset` 处构造零分配道迭代器
    static fn init(data: []const u8, header: SegyBinaryHeader): SegyTraceIter {
        return SegyTraceIter {
            data: data,
            offset: header.traces_offset,
            sample_format: header.sample_format,
            default_samples: header.samples_per_trace,
            fixed_length: header.fixed_length_flag == 1,
            is_little_endian: header.is_little_endian,
            had_error: false,
        };
    }

    /// 零分配读取下一条地震道；到达末尾或遇截断/未知格式时返回 `null`
    fn next(*_): SegyTrace? {
        if (_.offset >= _.data.len) {
            return null;
        }
        const sb_opt = segySampleBytes(_.sample_format);
        if (sb_opt == null) {
            _.had_error = true;
            _.offset = _.data.len;
            return null;
        }
        const sample_bytes = sb_opt ?? return null;

        if (_.data.len - _.offset < SEGY_TRACE_HEADER_SIZE) {
            _.had_error = true;
            _.offset = _.data.len;
            return null;
        }

        const th_slice = _.data[_.offset.._.offset + SEGY_TRACE_HEADER_SIZE];
        const le = _.is_little_endian;
        const th = SegyTraceHeader {
            trace_seq_line: readI32(th_slice, 0, le),
            trace_seq_file: readI32(th_slice, 4, le),
            field_record: readI32(th_slice, 8, le),
            cdp: readI32(th_slice, 20, le),
            trace_id_code: readI16(th_slice, 28, le),
            coord_scalar: readI16(th_slice, 70, le),
            source_x: readI32(th_slice, 72, le),
            source_y: readI32(th_slice, 76, le),
            group_x: readI32(th_slice, 80, le),
            group_y: readI32(th_slice, 84, le),
            trace_samples: readU16(th_slice, 114, le),
            trace_interval_us: readU16(th_slice, 116, le),
        };

        const nsamp_u16: u16 = if (_.fixed_length && _.default_samples > 0)
            _.default_samples
        else if (th.trace_samples > 0)
            th.trace_samples
        else
            _.default_samples;

        const payload_start = _.offset + SEGY_TRACE_HEADER_SIZE;
        const payload_bytes: usize = (nsamp_u16 as usize) * sample_bytes;
        if (_.data.len - payload_start < payload_bytes) {
            _.had_error = true;
            _.offset = _.data.len;
            return null;
        }

        const raw_samples = _.data[payload_start..payload_start + payload_bytes];
        _.offset = payload_start + payload_bytes;

        return SegyTrace {
            header: th,
            raw_samples: raw_samples,
            is_little_endian: le,
        };
    }
}

// ---------------------------------------------------------------------------
// 内部端序安全解码辅助函数
// ---------------------------------------------------------------------------

fn readU16(buf: []const u8, off: usize, le: bool): u16 {
    if (le) {
        return (buf[off] as u16) | ((buf[off + 1] as u16) << 8);
    }
    return ((buf[off] as u16) << 8) | (buf[off + 1] as u16);
}

fn readI16(buf: []const u8, off: usize, le: bool): i16 {
    const raw = readU16(buf, off, le);
    const val_i32: i32 = if (raw >= 32768) (raw as i32) - 65536 else (raw as i32);
    return val_i32 as i16;
}

fn readU32(buf: []const u8, off: usize, le: bool): u32 {
    if (le) {
        return (buf[off] as u32) |
            ((buf[off + 1] as u32) << 8) |
            ((buf[off + 2] as u32) << 16) |
            ((buf[off + 3] as u32) << 24);
    }
    return ((buf[off] as u32) << 24) |
        ((buf[off + 1] as u32) << 16) |
        ((buf[off + 2] as u32) << 8) |
        (buf[off + 3] as u32);
}

fn readI32(buf: []const u8, off: usize, le: bool): i32 {
    const raw = readU32(buf, off, le);
    const raw_i64: i64 = raw as i64;
    const val_i64: i64 = if (raw_i64 >= 2147483648) raw_i64 - 4294967296 else raw_i64;
    return val_i64 as i32;
}

fn readU64(buf: []const u8, off: usize, le: bool): u64 {
    if (le) {
        const lo: u64 = readU32(buf, off, true) as u64;
        const hi: u64 = readU32(buf, off + 4, true) as u64;
        return lo | (hi << 32);
    }
    const hi: u64 = readU32(buf, off, false) as u64;
    const lo: u64 = readU32(buf, off + 4, false) as u64;
    return (hi << 32) | lo;
}

fn decodeSampleFormat(code: u16): SegySampleFormat {
    return match (code) {
        1 => .ibm_f32,
        2 => .int32,
        3 => .int16,
        5 => .ieee_f32,
        6 => .ieee_f64,
        8 => .int8,
        else => .unknown,
    };
}

// ---------------------------------------------------------------------------
// 公开 API
// ---------------------------------------------------------------------------

/// IBM S/360 32-bit 十六进制浮点精确转 IEEE-754 `f64`
/// 格式：1-bit 符号 + 7-bit 偏置 64 十六进制指数 `16^(exp - 64)` + 24-bit 尾数 `frac / 2^24`
pub fn ibmFloatToF64(bits: u32): f64 {
    const frac: u32 = bits & 0x00FFFFFF;
    if (frac == 0) {
        return 0.0;
    }
    const sign: u32 = (bits >> 31) & 1;
    const exp: i32 = ((bits >> 24) & 0x7F) as i32;

    let val: f64 = (frac as f64) / 16777216.0;
    if (exp > 64) {
        let steps: i32 = exp - 64;
        while (steps > 0) {
            val *= 16.0;
            steps -= 1;
        }
    } else if (exp < 64) {
        let steps: i32 = 64 - exp;
        while (steps > 0) {
            val /= 16.0;
            steps -= 1;
        }
    }

    return if (sign != 0) -val else val;
}

/// 解析 SEG-Y 400 字节二进制卷头（校验 `data.len >= 3600` 并计算扩展文本头后的 `traces_offset`）
pub fn parseSegyHeader(data: []const u8): SegyBinaryHeader? {
    if (data.len < SEGY_MIN_HEADER_SIZE) {
        return null;
    }

    // Rev 2 端序标识位于二进制卷头偏移 96..100（文件偏移 3296..3300）：[4, 3, 2, 1] 表示小端
    const le: bool = data[3296] == 4 && data[3297] == 3 && data[3298] == 2 && data[3299] == 1;

    const job_id = readI32(data, 3200, le);
    const line_number = readI32(data, 3204, le);
    const reel_number = readI32(data, 3208, le);
    const traces_per_ensemble = readI16(data, 3212, le);
    const sample_interval_us = readU16(data, 3216, le);
    const samples_per_trace = readU16(data, 3220, le);
    const fmt_code = readU16(data, 3224, le);
    const measurement_system = readI16(data, 3254, le);
    const format_rev = readU16(data, 3500, le);
    const fixed_length_flag = readI16(data, 3502, le);
    const num_ext_text_headers = readI16(data, 3504, le);

    const ext_count: u64 = if (num_ext_text_headers > 0) (num_ext_text_headers as u64) else 0;
    const traces_off_u64: u64 = (SEGY_MIN_HEADER_SIZE as u64) + ext_count * (SEGY_TEXT_HEADER_SIZE as u64);
    if (traces_off_u64 > (data.len as u64)) {
        return null;
    }

    return SegyBinaryHeader {
        job_id: job_id,
        line_number: line_number,
        reel_number: reel_number,
        traces_per_ensemble: traces_per_ensemble,
        sample_interval_us: sample_interval_us,
        samples_per_trace: samples_per_trace,
        sample_format: decodeSampleFormat(fmt_code),
        measurement_system: measurement_system,
        format_rev: format_rev,
        fixed_length_flag: fixed_length_flag,
        num_ext_text_headers: num_ext_text_headers,
        traces_offset: traces_off_u64 as usize,
        is_little_endian: le,
    };
}

/// 从地震道原始采样字节中按 `sample_format` 统一解码第 `idx` 个样本为 `f64`，严格防越界
pub fn readSampleF64(trace: SegyTrace, sample_format: SegySampleFormat, idx: usize): f64? {
    const elem_size = segySampleBytes(sample_format) ?? return null;
    if (idx >= trace.raw_samples.len / elem_size) {
        return null;
    }
    const off = idx * elem_size;
    const le = trace.is_little_endian;

    match (sample_format) {
        .ibm_f32 => {
            return ibmFloatToF64(readU32(trace.raw_samples, off, le));
        },
        .ieee_f32 => {
            const bits: u32 = readU32(trace.raw_samples, off, le);
            const val_f32: f32 = @bitCast(bits);
            return val_f32 as f64;
        },
        .ieee_f64 => {
            const bits: u64 = readU64(trace.raw_samples, off, le);
            const val_f64: f64 = @bitCast(bits);
            return val_f64;
        },
        .int32 => {
            return readI32(trace.raw_samples, off, le) as f64;
        },
        .int16 => {
            return readI16(trace.raw_samples, off, le) as f64;
        },
        .int8 => {
            const b: u8 = trace.raw_samples[off];
            const s: i32 = if (b >= 128) (b as i32) - 256 else (b as i32);
            return s as f64;
        },
        .unknown => {
            return null;
        },
    }
}

/// 应用 SEG-Y 坐标/高程缩放标量：
/// - `scalar > 0`：`raw * scalar`
/// - `scalar < 0`：`raw / (-scalar)`
/// - `scalar == 0`：保持原值 `raw`
pub fn applyCoordScalar(raw: i32, scalar: i16): f64 {
    if (scalar > 0) {
        return (raw as f64) * (scalar as f64);
    }
    if (scalar < 0) {
        const denom: i32 = -(scalar as i32);
        return (raw as f64) / (denom as f64);
    }
    return raw as f64;
}
