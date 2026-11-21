// Source: Absorbed from /data/dev/bun/src/ (Zero-Alloc European Data Format EDF/EDF+ Biosignal Parser)

/// EDF 固定 256 字节主头长度
pub const EDF_MAIN_HEADER_SIZE: usize = 256;

/// EDF 每个信号通道的固定 256 字节通道头长度（按字段列式排布）
pub const EDF_SIGNAL_HEADER_SIZE: usize = 256;

/// EDF / EDF+ 256 字节定宽 ASCII 主头（零拷贝切片）
pub struct EdfHeader {
    version: []const u8,
    patient_id: []const u8,
    recording_id: []const u8,
    start_date: []const u8,
    start_time: []const u8,
    header_bytes: u32,
    reserved: []const u8,
    num_records: i64,
    record_duration_sec: f64,
    num_signals: u32,

    /// 判断是否为 EDF+（`reserved` 以 `"EDF+C"` 或 `"EDF+D"` 开头）
    fn isEdfPlus(): bool {
        return startsWith(_.reserved, "EDF+C") || startsWith(_.reserved, "EDF+D");
    }

    /// 计算文件总时长（秒）；当 `num_records < 0`（实时流未知记录数）时返回 `-1.0`
    fn totalDurationSec(): f64 {
        if (_.num_records < 0) {
            return -1.0;
        }
        return (_.num_records as f64) * _.record_duration_sec;
    }
}

/// EDF / EDF+ 单通道信号元数据（从列式通道头零拷贝切片解析）
pub struct EdfSignal {
    label: []const u8,
    transducer: []const u8,
    physical_dim: []const u8,
    physical_min: f64,
    physical_max: f64,
    digital_min: i32,
    digital_max: i32,
    prefiltering: []const u8,
    samples_per_record: u32,

    /// 计算该通道采样率（Hz）= `samples_per_record / record_duration_sec`
    /// 若 `record_duration_sec <= 0.0` 则安全返回 `0.0`
    fn sampleRateHz(header: EdfHeader): f64 {
        if (header.record_duration_sec <= 0.0) {
            return 0.0;
        }
        return (_.samples_per_record as f64) / header.record_duration_sec;
    }

    /// 线性物理量换算：`phys_min + (raw - dig_min) * (phys_max - phys_min) / (dig_max - dig_min)`
    /// 若 `digital_max == digital_min` 则防除零返回 `physical_min`
    fn digitalToPhysical(raw: i16): f64 {
        const dig_min_f: f64 = _.digital_min as f64;
        const dig_max_f: f64 = _.digital_max as f64;
        const dig_span: f64 = dig_max_f - dig_min_f;
        if (dig_span == 0.0) {
            return _.physical_min;
        }
        const phys_span: f64 = _.physical_max - _.physical_min;
        return _.physical_min + ((raw as f64) - dig_min_f) * phys_span / dig_span;
    }
}

// ---------------------------------------------------------------------------
// 内部零分配切片与 ASCII 数值解析辅助函数
// ---------------------------------------------------------------------------

fn isAsciiSpace(c: u8): bool {
    return c == ' ' || c == '\t';
}

fn isAsciiDigit(c: u8): bool {
    return c >= '0' && c <= '9';
}

fn trimTrailingSpaces(s: []const u8): []const u8 {
    let end: usize = s.len;
    while (end > 0 && s[end - 1] == ' ') {
        end -= 1;
    }
    return s[0..end];
}

fn trimAscii(s: []const u8): []const u8 {
    let start: usize = 0;
    while (start < s.len && isAsciiSpace(s[start])) {
        start += 1;
    }
    let end: usize = s.len;
    while (end > start && isAsciiSpace(s[end - 1])) {
        end -= 1;
    }
    return s[start..end];
}

fn bytesEq(a: []const u8, b: []const u8): bool {
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

fn startsWith(s: []const u8, prefix: []const u8): bool {
    if (s.len < prefix.len) {
        return false;
    }
    let i: usize = 0;
    while (i < prefix.len) {
        if (s[i] != prefix[i]) {
            return false;
        }
        i += 1;
    }
    return true;
}

fn parseAsciiU32(raw: []const u8): u32? {
    const s = trimAscii(raw);
    if (s.len == 0) {
        return null;
    }
    let i: usize = 0;
    if (s[0] == '+') {
        i = 1;
    }
    if (i >= s.len) {
        return null;
    }
    let acc: u64 = 0;
    while (i < s.len) {
        const c = s[i];
        if (!isAsciiDigit(c)) {
            return null;
        }
        acc = acc * 10 + ((c - '0') as u64);
        if (acc > 4294967295) {
            return null;
        }
        i += 1;
    }
    return acc as u32;
}

fn parseAsciiI32(raw: []const u8): i32? {
    const s = trimAscii(raw);
    if (s.len == 0) {
        return null;
    }
    let i: usize = 0;
    let neg: bool = false;
    if (s[0] == '-') {
        neg = true;
        i = 1;
    } else if (s[0] == '+') {
        i = 1;
    }
    if (i >= s.len) {
        return null;
    }
    let acc: i64 = 0;
    while (i < s.len) {
        const c = s[i];
        if (!isAsciiDigit(c)) {
            return null;
        }
        acc = acc * 10 + ((c - '0') as i64);
        if (neg) {
            if (acc > 2147483648) {
                return null;
            }
        } else {
            if (acc > 2147483647) {
                return null;
            }
        }
        i += 1;
    }
    const signed_val: i64 = if (neg) -acc else acc;
    return signed_val as i32;
}

fn parseAsciiI64(raw: []const u8): i64? {
    const s = trimAscii(raw);
    if (s.len == 0) {
        return null;
    }
    let i: usize = 0;
    let neg: bool = false;
    if (s[0] == '-') {
        neg = true;
        i = 1;
    } else if (s[0] == '+') {
        i = 1;
    }
    if (i >= s.len) {
        return null;
    }
    let acc: u64 = 0;
    const limit: u64 = if (neg) 9223372036854775808 else 9223372036854775807;
    while (i < s.len) {
        const c = s[i];
        if (!isAsciiDigit(c)) {
            return null;
        }
        const d: u64 = (c - '0') as u64;
        if (acc > (limit - d) / 10) {
            return null;
        }
        acc = acc * 10 + d;
        i += 1;
    }
    if (neg) {
        if (acc == 9223372036854775808) {
            return -9223372036854775807 - 1;
        }
        return -(acc as i64);
    }
    return acc as i64;
}

fn parseAsciiF64(raw: []const u8): f64? {
    const s = trimAscii(raw);
    if (s.len == 0) {
        return null;
    }
    let i: usize = 0;
    let neg: bool = false;
    if (s[0] == '-') {
        neg = true;
        i = 1;
    } else if (s[0] == '+') {
        i = 1;
    }
    if (i >= s.len) {
        return null;
    }

    let val: f64 = 0.0;
    let has_digits: bool = false;
    while (i < s.len && isAsciiDigit(s[i])) {
        val = val * 10.0 + ((s[i] - '0') as f64);
        has_digits = true;
        i += 1;
    }

    if (i < s.len && s[i] == '.') {
        i += 1;
        let frac: f64 = 0.1;
        while (i < s.len && isAsciiDigit(s[i])) {
            val += ((s[i] - '0') as f64) * frac;
            frac *= 0.1;
            has_digits = true;
            i += 1;
        }
    }

    if (!has_digits) {
        return null;
    }

    if (i < s.len && (s[i] == 'e' || s[i] == 'E')) {
        i += 1;
        let exp_neg: bool = false;
        if (i < s.len && s[i] == '-') {
            exp_neg = true;
            i += 1;
        } else if (i < s.len && s[i] == '+') {
            i += 1;
        }
        if (i >= s.len || !isAsciiDigit(s[i])) {
            return null;
        }
        let exp_val: i32 = 0;
        while (i < s.len && isAsciiDigit(s[i])) {
            if (exp_val < 1000) {
                exp_val = exp_val * 10 + ((s[i] - '0') as i32);
            }
            i += 1;
        }
        let step: i32 = 0;
        while (step < exp_val) {
            if (exp_neg) {
                val *= 0.1;
            } else {
                val *= 10.0;
            }
            step += 1;
        }
    }

    if (i != s.len) {
        return null;
    }
    return if (neg) -val else val;
}

fn validateHeaderSize(header_bytes: u32, num_signals: u32): bool {
    const total_blocks: u64 = (num_signals as u64) + 1;
    const expected: u64 = total_blocks * 256;
    return (header_bytes as u64) == expected;
}

// ---------------------------------------------------------------------------
// 公开 API
// ---------------------------------------------------------------------------

/// 快速嗅探字节流是否为合法 EDF / EDF+ 头：
/// 1. 长度 >= 256
/// 2. 前 8 字节为 `"0       "`（版本 0 + 7 个空格）
/// 3. `header_bytes == 256 * (1 + num_signals)`
pub fn isEdf(data: []const u8): bool {
    if (data.len < EDF_MAIN_HEADER_SIZE) {
        return false;
    }
    if (!bytesEq(data[0..8], "0       ")) {
        return false;
    }
    const header_bytes = parseAsciiU32(data[184..192]) ?? return false;
    const num_signals = parseAsciiU32(data[252..256]) ?? return false;
    return validateHeaderSize(header_bytes, num_signals);
}

/// 零分配解析 256 字节定宽 ASCII 主头
pub fn parseEdfHeader(data: []const u8): EdfHeader? {
    if (data.len < EDF_MAIN_HEADER_SIZE) {
        return null;
    }
    const version = data[0..8];
    if (!bytesEq(version, "0       ")) {
        return null;
    }

    const header_bytes = parseAsciiU32(data[184..192]) ?? return null;
    const num_records = parseAsciiI64(data[236..244]) ?? return null;
    const record_duration_sec = parseAsciiF64(data[244..252]) ?? return null;
    const num_signals = parseAsciiU32(data[252..256]) ?? return null;

    if (!validateHeaderSize(header_bytes, num_signals)) {
        return null;
    }
    if (num_records < -1) {
        return null;
    }
    if (record_duration_sec < 0.0) {
        return null;
    }

    return EdfHeader {
        version: version,
        patient_id: trimTrailingSpaces(data[8..88]),
        recording_id: trimTrailingSpaces(data[88..168]),
        start_date: trimTrailingSpaces(data[168..176]),
        start_time: trimTrailingSpaces(data[176..184]),
        header_bytes: header_bytes,
        reserved: trimTrailingSpaces(data[192..236]),
        num_records: num_records,
        record_duration_sec: record_duration_sec,
        num_signals: num_signals,
    };
}

/// 零分配解析指定通道索引 `sig_idx` 的列式信号头元数据
pub fn parseEdfSignal(data: []const u8, header: EdfHeader, sig_idx: u32): EdfSignal? {
    if (sig_idx >= header.num_signals) {
        return null;
    }
    if (!validateHeaderSize(header.header_bytes, header.num_signals)) {
        return null;
    }
    const hdr_len: usize = header.header_bytes as usize;
    if (data.len < hdr_len) {
        return null;
    }

    const ns: usize = header.num_signals as usize;
    const idx: usize = sig_idx as usize;

    const label_off: usize = EDF_MAIN_HEADER_SIZE + idx * 16;
    const trans_off: usize = EDF_MAIN_HEADER_SIZE + 16 * ns + idx * 80;
    const pdim_off: usize = EDF_MAIN_HEADER_SIZE + 96 * ns + idx * 8;
    const pmin_off: usize = EDF_MAIN_HEADER_SIZE + 104 * ns + idx * 8;
    const pmax_off: usize = EDF_MAIN_HEADER_SIZE + 112 * ns + idx * 8;
    const dmin_off: usize = EDF_MAIN_HEADER_SIZE + 120 * ns + idx * 8;
    const dmax_off: usize = EDF_MAIN_HEADER_SIZE + 128 * ns + idx * 8;
    const pref_off: usize = EDF_MAIN_HEADER_SIZE + 136 * ns + idx * 80;
    const spr_off: usize = EDF_MAIN_HEADER_SIZE + 216 * ns + idx * 8;

    const physical_min = parseAsciiF64(data[pmin_off..pmin_off + 8]) ?? return null;
    const physical_max = parseAsciiF64(data[pmax_off..pmax_off + 8]) ?? return null;
    const digital_min = parseAsciiI32(data[dmin_off..dmin_off + 8]) ?? return null;
    const digital_max = parseAsciiI32(data[dmax_off..dmax_off + 8]) ?? return null;
    const samples_per_record = parseAsciiU32(data[spr_off..spr_off + 8]) ?? return null;

    return EdfSignal {
        label: trimTrailingSpaces(data[label_off..label_off + 16]),
        transducer: trimTrailingSpaces(data[trans_off..trans_off + 80]),
        physical_dim: trimTrailingSpaces(data[pdim_off..pdim_off + 8]),
        physical_min: physical_min,
        physical_max: physical_max,
        digital_min: digital_min,
        digital_max: digital_max,
        prefiltering: trimTrailingSpaces(data[pref_off..pref_off + 80]),
        samples_per_record: samples_per_record,
    };
}

/// 计算单条数据记录的字节长度（所有通道 `samples_per_record` 之和 × 2 字节）
pub fn edfRecordByteSize(data: []const u8, header: EdfHeader): usize? {
    if (!validateHeaderSize(header.header_bytes, header.num_signals)) {
        return null;
    }
    const hdr_len: usize = header.header_bytes as usize;
    if (data.len < hdr_len) {
        return null;
    }

    const ns: usize = header.num_signals as usize;
    const spr_base: usize = EDF_MAIN_HEADER_SIZE + 216 * ns;
    let total_samples: u64 = 0;
    let s: usize = 0;
    while (s < ns) {
        const off = spr_base + s * 8;
        const spr = parseAsciiU32(data[off..off + 8]) ?? return null;
        total_samples += spr as u64;
        s += 1;
    }

    const total_bytes: u64 = total_samples * 2;
    return total_bytes as usize;
}

/// 从 16-bit signed little-endian 交错数据记录中精确读取指定 `(record_idx, sig_idx, sample_idx)` 数字样本，严格防越界
pub fn readEdfDigitalSample(
    data: []const u8,
    header: EdfHeader,
    record_idx: usize,
    sig_idx: u32,
    sample_idx: usize,
): i16? {
    if (sig_idx >= header.num_signals) {
        return null;
    }
    if (!validateHeaderSize(header.header_bytes, header.num_signals)) {
        return null;
    }
    if (header.num_records >= 0 && (record_idx as i64) >= header.num_records) {
        return null;
    }
    const hdr_len: usize = header.header_bytes as usize;
    if (data.len < hdr_len) {
        return null;
    }

    const ns: usize = header.num_signals as usize;
    const target_sig: usize = sig_idx as usize;
    const spr_base: usize = EDF_MAIN_HEADER_SIZE + 216 * ns;

    let samples_before: u64 = 0;
    let target_samples: u64 = 0;
    let total_samples: u64 = 0;

    let s: usize = 0;
    while (s < ns) {
        const off = spr_base + s * 8;
        const spr = parseAsciiU32(data[off..off + 8]) ?? return null;
        const spr_u64: u64 = spr as u64;
        if (s < target_sig) {
            samples_before += spr_u64;
        } else if (s == target_sig) {
            target_samples = spr_u64;
        }
        total_samples += spr_u64;
        s += 1;
    }

    if ((sample_idx as u64) >= target_samples) {
        return null;
    }

    const record_bytes: u64 = total_samples * 2;
    const data_len_u64: u64 = data.len as u64;
    const hdr_u64: u64 = header.header_bytes as u64;

    if (record_bytes > 0 && (record_idx as u64) > (data_len_u64 - hdr_u64) / record_bytes) {
        return null;
    }

    const byte_off_u64: u64 =
        hdr_u64 + (record_idx as u64) * record_bytes + (samples_before + (sample_idx as u64)) * 2;
    if (byte_off_u64 + 2 > data_len_u64) {
        return null;
    }

    const off: usize = byte_off_u64 as usize;
    const lo: u16 = data[off] as u16;
    const hi: u16 = data[off + 1] as u16;
    const raw_u16: u16 = lo | (hi << 8);
    const val_i32: i32 = if (raw_u16 >= 32768) (raw_u16 as i32) - 65536 else (raw_u16 as i32);
    return val_i32 as i16;
}
