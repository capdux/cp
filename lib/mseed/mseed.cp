// Source: Absorbed from /data/dev/bun/src/ (Zero-Alloc IRIS MiniSEED v2.4 Seismology Parser)
// Pure CP zero-heap-allocation FDSN SEED v2.4 MiniSEED parser:
// 48-byte fixed header, automatic Big/Little Endian detection via BTIME,
// Blockette 1000 / Blockette 1001 chain decoding, SEED sample rate formula,
// and multi-record zero-copy streaming iteration.

/// Fixed Section of Data Header size in bytes (FDSN SEED v2.4).
pub const MSEED_FIXED_HEADER_SIZE: usize = 48;

/// Minimum valid MiniSEED data record length (2^8 = 256 bytes).
pub const MSEED_MIN_RECORD_SIZE: usize = 256;

/// Blockette 1000 type code (Data Only SEED Blockette).
pub const MSEED_BLK_1000: u16 = 1000;

/// Blockette 1001 type code (Data Extension Blockette).
pub const MSEED_BLK_1001: u16 = 1001;

/// SEED v2.4 Blockette 1000 Data Encoding Format codes.
pub enum MseedEncoding: u8 {
    ascii = 0,
    int16 = 1,
    int24 = 2,
    int32 = 3,
    ieee_f32 = 4,
    ieee_f64 = 5,
    steim1 = 10,
    steim2 = 11,
    unknown = 255,
}

/// Decodes a raw Blockette 1000 encoding byte into `MseedEncoding`.
pub fn mseedEncodingFromU8(v: u8): MseedEncoding {
    switch (v) {
        0 => return MseedEncoding.ascii,
        1 => return MseedEncoding.int16,
        2 => return MseedEncoding.int24,
        3 => return MseedEncoding.int32,
        4 => return MseedEncoding.ieee_f32,
        5 => return MseedEncoding.ieee_f64,
        10 => return MseedEncoding.steim1,
        11 => return MseedEncoding.steim2,
        else => return MseedEncoding.unknown,
    }
}

/// 10-byte FDSN SEED BTIME timestamp structure.
pub struct MseedBtime {
    year: u16 = 0,
    day_of_year: u16 = 0,
    hour: u8 = 0,
    minute: u8 = 0,
    second: u8 = 0,
    tenths_ms: u16 = 0,
}

/// Zero-copy decoded MiniSEED v2.4 data record.
pub struct MseedRecord {
    seq_num: []const u8 = "",
    quality_indicator: u8 = 0,
    station: []const u8 = "",
    location: []const u8 = "",
    channel: []const u8 = "",
    network: []const u8 = "",
    start_time: MseedBtime = MseedBtime {},
    num_samples: u16 = 0,
    sample_rate_factor: i16 = 0,
    sample_rate_mult: i16 = 0,
    activity_flags: u8 = 0,
    io_clock_flags: u8 = 0,
    data_quality_flags: u8 = 0,
    num_blockettes: u8 = 0,
    time_correction: i32 = 0,
    data_offset: u16 = 0,
    first_blockette_offset: u16 = 0,
    is_big_endian: bool = true,

    // Blockette 1000 fields
    has_blk_1000: bool = false,
    encoding: MseedEncoding = MseedEncoding.unknown,
    word_order: u8 = 1,
    data_rec_len_exp: u8 = 0,
    record_bytes: usize = 0,

    // Blockette 1001 fields
    has_blk_1001: bool = false,
    timing_quality: u8 = 0,
    microsecond: i8 = 0,
    frame_count: u8 = 0,

    /// Zero-copy slice of encoded waveform payload (`offset + data_offset .. offset + record_bytes`).
    payload: []const u8 = "",

    /// Computes nominal sampling rate in Hz per FDSN SEED v2.4 formula:
    /// - `factor > 0, mult > 0`: `factor * mult`
    /// - `factor > 0, mult < 0`: `-factor / mult`
    /// - `factor < 0, mult > 0`: `-mult / factor`
    /// - `factor < 0, mult < 0`: `1.0 / (factor * mult)`
    /// - either `0`: `0.0`
    fn sampleRateHz(): f64 {
        const f = _.sample_rate_factor as f64;
        const m = _.sample_rate_mult as f64;
        if (_.sample_rate_factor > 0 && _.sample_rate_mult > 0) {
            return f * m;
        }
        if (_.sample_rate_factor > 0 && _.sample_rate_mult < 0) {
            return -f / m;
        }
        if (_.sample_rate_factor < 0 && _.sample_rate_mult > 0) {
            return -m / f;
        }
        if (_.sample_rate_factor < 0 && _.sample_rate_mult < 0) {
            return 1.0 / (f * m);
        }
        return 0.0;
    }
}

/// Aggregate statistics from a single-pass scan of a MiniSEED buffer.
pub struct MseedStats {
    record_count: u32 = 0,
    total_samples: u64 = 0,
    min_record_bytes: usize = 0,
    max_record_bytes: usize = 0,
}

// ---------------------------------------------------------------------------
// Internal Endianness & Validation Helpers
// ---------------------------------------------------------------------------

fn readU16Le(data: []const u8, off: usize): u16 {
    return (data[off] as u16) | ((data[off + 1] as u16) << 8);
}

fn readU16Be(data: []const u8, off: usize): u16 {
    return ((data[off] as u16) << 8) | (data[off + 1] as u16);
}

fn readU16(data: []const u8, off: usize, is_be: bool): u16 {
    if (is_be) {
        return readU16Be(data, off);
    }
    return readU16Le(data, off);
}

fn readI16(data: []const u8, off: usize, is_be: bool): i16 {
    const bits = readU16(data, off, is_be);
    return @bitCast(bits);
}

fn readU32Le(data: []const u8, off: usize): u32 {
    return (data[off] as u32) |
        ((data[off + 1] as u32) << 8) |
        ((data[off + 2] as u32) << 16) |
        ((data[off + 3] as u32) << 24);
}

fn readU32Be(data: []const u8, off: usize): u32 {
    return ((data[off] as u32) << 24) |
        ((data[off + 1] as u32) << 16) |
        ((data[off + 2] as u32) << 8) |
        (data[off + 3] as u32);
}

fn readI32(data: []const u8, off: usize, is_be: bool): i32 {
    const bits = if (is_be) readU32Be(data, off) else readU32Le(data, off);
    return @bitCast(bits);
}

fn isAsciiDigit(ch: u8): bool {
    return ch >= '0' && ch <= '9';
}

fn isValidQuality(ch: u8): bool {
    return ch == 'D' || ch == 'R' || ch == 'Q' || ch == 'M';
}

fn trimRightSpaces(s: []const u8): []const u8 {
    let end = s.len;
    while (end > 0) {
        const ch = s[end - 1];
        if (ch == ' ' || ch == 0) {
            end -= 1;
        } else {
            break;
        }
    }
    return s[0..end];
}

fn isValidBtimeYearDay(year: u16, doy: u16): bool {
    return year >= 1900 && year <= 2100 && doy >= 1 && doy <= 366;
}

// ---------------------------------------------------------------------------
// Public API
// ---------------------------------------------------------------------------

/// Validates the 8-byte MiniSEED v2.4 fixed header prefix:
/// `data.len >= 48`, bytes `0..6` are ASCII digits, byte `6` is `'D'`/`'R'`/`'Q'`/`'M'`,
/// and byte `7` is ASCII space `' '`.
pub fn isMseedRecord(data: []const u8): bool {
    if (data.len < MSEED_FIXED_HEADER_SIZE) {
        return false;
    }
    let i: usize = 0;
    while (i < 6) {
        if (!isAsciiDigit(data[i])) {
            return false;
        }
        i += 1;
    }
    if (!isValidQuality(data[6])) {
        return false;
    }
    return data[7] == ' ';
}

/// Parses a single MiniSEED v2.4 record starting at `offset` in `data`.
/// Automatically detects header byte order (Big Endian vs Little Endian) via BTIME.
pub fn parseMseedRecord(data: []const u8, offset: usize): MseedRecord? {
    if (offset >= data.len || data.len - offset < MSEED_FIXED_HEADER_SIZE) {
        return null;
    }
    const rec = data[offset..];
    if (!isMseedRecord(rec)) {
        return null;
    }

    // Auto-detect endianness using BTIME year (1900..2100) and day_of_year (1..366)
    let is_be = true;
    const year_be = readU16Be(rec, 20);
    const doy_be = readU16Be(rec, 22);
    if (isValidBtimeYearDay(year_be, doy_be)) {
        is_be = true;
    } else {
        const year_le = readU16Le(rec, 20);
        const doy_le = readU16Le(rec, 22);
        if (isValidBtimeYearDay(year_le, doy_le)) {
            is_be = false;
        } else {
            return null;
        }
    }

    const year = readU16(rec, 20, is_be);
    const doy = readU16(rec, 22, is_be);
    const hour = rec[24];
    const minute = rec[25];
    const second = rec[26];
    const tenths_ms = readU16(rec, 28, is_be);

    if (hour > 23 || minute > 59 || second > 60 || tenths_ms > 9999) {
        return null;
    }

    const num_samples = readU16(rec, 30, is_be);
    const sr_factor = readI16(rec, 32, is_be);
    const sr_mult = readI16(rec, 34, is_be);
    const activity_flags = rec[36];
    const io_clock_flags = rec[37];
    const data_quality_flags = rec[38];
    const num_blockettes = rec[39];
    const time_correction = readI32(rec, 40, is_be);
    const data_offset = readU16(rec, 44, is_be);
    const first_blk_offset = readU16(rec, 46, is_be);

    let result = MseedRecord {
        seq_num: rec[0..6],
        quality_indicator: rec[6],
        station: trimRightSpaces(rec[8..13]),
        location: trimRightSpaces(rec[13..15]),
        channel: trimRightSpaces(rec[15..18]),
        network: trimRightSpaces(rec[18..20]),
        start_time: MseedBtime {
            year: year,
            day_of_year: doy,
            hour: hour,
            minute: minute,
            second: second,
            tenths_ms: tenths_ms,
        },
        num_samples: num_samples,
        sample_rate_factor: sr_factor,
        sample_rate_mult: sr_mult,
        activity_flags: activity_flags,
        io_clock_flags: io_clock_flags,
        data_quality_flags: data_quality_flags,
        num_blockettes: num_blockettes,
        time_correction: time_correction,
        data_offset: data_offset,
        first_blockette_offset: first_blk_offset,
        is_big_endian: is_be,
    };

    // Traverse Blockette chain
    let blk_off = first_blk_offset as usize;
    let blk_idx: u8 = 0;
    while (blk_idx < num_blockettes && blk_off != 0) {
        if (blk_off < MSEED_FIXED_HEADER_SIZE || blk_off + 4 > rec.len) {
            return null;
        }
        const blk_type = readU16(rec, blk_off, is_be);
        const next_off = readU16(rec, blk_off + 2, is_be) as usize;

        if (blk_type == MSEED_BLK_1000) {
            if (blk_off + 8 > rec.len) {
                return null;
            }
            const exp = rec[blk_off + 6];
            if (exp < 8 || exp > 20) {
                return null;
            }
            result.has_blk_1000 = true;
            result.encoding = mseedEncodingFromU8(rec[blk_off + 4]);
            result.word_order = rec[blk_off + 5];
            result.data_rec_len_exp = exp;
            result.record_bytes = (1 as usize) << (exp as u6);
        } else if (blk_type == MSEED_BLK_1001) {
            if (blk_off + 8 > rec.len) {
                return null;
            }
            result.has_blk_1001 = true;
            result.timing_quality = rec[blk_off + 4];
            result.microsecond = @bitCast(rec[blk_off + 5]);
            result.frame_count = rec[blk_off + 7];
        }

        // Prevent cyclic or backward blockette pointers
        if (next_off != 0 && next_off <= blk_off) {
            return null;
        }
        blk_off = next_off;
        blk_idx += 1;
    }

    if (!result.has_blk_1000) {
        if (rec.len < MSEED_MIN_RECORD_SIZE) {
            return null;
        }
        result.record_bytes = if (rec.len >= 4096) 4096 else rec.len;
    }

    if (result.record_bytes > rec.len) {
        return null;
    }

    const doff = data_offset as usize;
    if (doff == 0 && num_samples == 0) {
        result.payload = "";
    } else {
        if (doff < MSEED_FIXED_HEADER_SIZE || doff > result.record_bytes) {
            return null;
        }
        result.payload = rec[doff..result.record_bytes];
    }

    return result;
}

/// Zero-allocation streaming iterator over concatenated MiniSEED records.
pub struct MseedIter {
    data: []const u8 = "",
    pos: usize = 0,
    failed: bool = false,

    pub fn init(data: []const u8): MseedIter {
        return MseedIter {
            data: data,
            pos: 0,
            failed: false,
        };
    }

    /// Returns `true` if iteration halted due to a malformed or truncated record.
    fn hasError(): bool {
        return _.failed;
    }

    /// Parses the next MiniSEED record and advances by its `record_bytes`.
    fn next(*_): MseedRecord? {
        if (_.failed || _.pos >= _.data.len) {
            return null;
        }
        if (rec := parseMseedRecord(_.data, _.pos)) {
            if (rec.record_bytes == 0) {
                _.failed = true;
                return null;
            }
            _.pos += rec.record_bytes;
            return rec;
        }
        _.failed = true;
        return null;
    }
}

/// Scans a MiniSEED buffer in a single zero-allocation pass and aggregates record/sample statistics.
/// Returns `null` if the buffer is empty or contains any malformed/truncated record.
pub fn summarizeMseed(data: []const u8): MseedStats? {
    if (data.len == 0) {
        return null;
    }
    let it = MseedIter.init(data);
    let stats = MseedStats {};

    while (rec := it.next()) {
        if (stats.record_count == 0) {
            stats.min_record_bytes = rec.record_bytes;
            stats.max_record_bytes = rec.record_bytes;
        } else {
            if (rec.record_bytes < stats.min_record_bytes) {
                stats.min_record_bytes = rec.record_bytes;
            }
            if (rec.record_bytes > stats.max_record_bytes) {
                stats.max_record_bytes = rec.record_bytes;
            }
        }
        stats.record_count += 1;
        stats.total_samples += rec.num_samples as u64;
    }

    if (it.hasError() || stats.record_count == 0) {
        return null;
    }
    return stats;
}
