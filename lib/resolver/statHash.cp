// Source: Absorbed from /data/dev/bun/src/resolver/fs/stat_hash.cp | Synced: 2026-11-22T17:35:38Z
// ============================================================================
// lib/resolver/statHash.cp — Zero-Alloc Filesystem StatHash & HTTP Last-Modified
// ============================================================================
// Computes a deterministic 64-bit fingerprint from POSIX file metadata
// `(size, mode, mtime_sec, mtime_nsec, ino, path)` and caches a zero-alloc
// 29-byte RFC 9110/7231 IMF-fixdate `Last-Modified` header for regular files,
// plus strong/weak ETag formatting for static asset & bundler cache validation.

pub const STAT_HASH_SEED: u64 = 42;
pub const STAT_HTTP_DATE_LEN: usize = 29;
pub const STAT_STRONG_ETAG_LEN: usize = 18; // "0123456789abcdef"
pub const STAT_WEAK_ETAG_LEN: usize = 20;   // W/"0123456789abcdef"

pub const STAT_MODE_TYPE_MASK: u32 = 0o170000;
pub const STAT_MODE_REG: u32 = 0o100000;
pub const STAT_MODE_DIR: u32 = 0o040000;
pub const STAT_MODE_LNK: u32 = 0o120000;

pub err StatHashError {
    BufferTooSmall,
    YearOutOfRange,
}

/// Portable, zero-alloc filesystem stat metadata input (decoupled from OS syscalls).
pub struct FileStatMeta {
    size: u64 = 0,
    mode: u32 = STAT_MODE_REG | 0o644,
    mtime_sec: i64 = 0,
    mtime_nsec: i64 = 0,
    ino: u64 = 0,

    static fn regular(size: u64, mtime_sec: i64, mtime_nsec: i64, ino: u64): FileStatMeta {
        return FileStatMeta{
            size: size,
            mode: STAT_MODE_REG | 0o644,
            mtime_sec: mtime_sec,
            mtime_nsec: mtime_nsec,
            ino: ino,
        };
    }

    static fn directory(mtime_sec: i64, ino: u64): FileStatMeta {
        return FileStatMeta{
            size: 0,
            mode: STAT_MODE_DIR | 0o755,
            mtime_sec: mtime_sec,
            mtime_nsec: 0,
            ino: ino,
        };
    }

    fn isRegularFile(): bool {
        return (_.mode & STAT_MODE_TYPE_MASK) == STAT_MODE_REG;
    }

    fn isDirectory(): bool {
        return (_.mode & STAT_MODE_TYPE_MASK) == STAT_MODE_DIR;
    }

    fn isSymlink(): bool {
        return (_.mode & STAT_MODE_TYPE_MASK) == STAT_MODE_LNK;
    }

    /// Converts `(mtime_sec, mtime_nsec)` to unsigned Unix milliseconds, clamping negative values to 0.
    fn mtimeMsUnsigned(): u64 {
        if (_.mtime_sec < 0) return 0;
        const sec_u: u64 = @as(u64, @intCast(_.mtime_sec));
        const nsec_u: u64 = if (_.mtime_nsec > 0) @as(u64, @intCast(_.mtime_nsec)) else 0;
        return sec_u * 1000 + (nsec_u / 1000000);
    }
}

fn mixHashU64(h: u64, v: u64): u64 {
    let x = h ^ v;
    x = x *% 0x9E3779B97F4A7C15;
    x = (x ^ (x >> 30)) *% 0xBF58476D1CE4E5B9;
    return x ^ (x >> 27);
}

fn mixHashBytes(h_in: u64, data: []const u8): u64 {
    let h = h_in ^ @as(u64, @intCast(data.len));
    let i: usize = 0;
    while (i < data.len) {
        h = (h ^ @as(u64, data[i])) *% 0x100000001B3;
        i += 1;
    }
    return mixHashU64(h, 0x94D049BB133111EB);
}

fn write2Digits(out: []u8, off: usize, v: u64) {
    out[off] = @as(u8, @intCast('0' + ((v / 10) % 10)));
    out[off + 1] = @as(u8, @intCast('0' + (v % 10)));
}

fn write4Digits(out: []u8, off: usize, v: u64) {
    out[off] = @as(u8, @intCast('0' + ((v / 1000) % 10)));
    out[off + 1] = @as(u8, @intCast('0' + ((v / 100) % 10)));
    out[off + 2] = @as(u8, @intCast('0' + ((v / 10) % 10)));
    out[off + 3] = @as(u8, @intCast('0' + (v % 10)));
}

fn writeAscii3(out: []u8, off: usize, s: []const u8) {
    out[off] = s[0];
    out[off + 1] = s[1];
    out[off + 2] = s[2];
}

fn weekdayName(wday: u64): []const u8 {
    return switch (wday) {
        0 => "Sun",
        1 => "Mon",
        2 => "Tue",
        3 => "Wed",
        4 => "Thu",
        5 => "Fri",
        else => "Sat",
    };
}

fn monthName(month: u64): []const u8 {
    return switch (month) {
        1 => "Jan",
        2 => "Feb",
        3 => "Mar",
        4 => "Apr",
        5 => "May",
        6 => "Jun",
        7 => "Jul",
        8 => "Aug",
        9 => "Sep",
        10 => "Oct",
        11 => "Nov",
        else => "Dec",
    };
}

fn hexDigitLower(nibble: u8): u8 {
    return if (nibble < 10) ('0' + nibble) else ('a' + (nibble - 10));
}

fn writeHexU64(out: []u8, off: usize, v: u64) {
    let i: usize = 0;
    while (i < 16) {
        const shift: u6 = @as(u6, @intCast((15 - i) * 4));
        const nib: u8 = @as(u8, @intCast((v >> shift) & 0xF));
        out[off + i] = hexDigitLower(nib);
        i += 1;
    }
}

/// Formats a Unix millisecond timestamp as a 29-byte RFC 9110/7231 IMF-fixdate
/// string (e.g. `"Sun, 06 Nov 1994 08:49:37 GMT"`), zero heap allocation.
pub fn formatHttpDate(epoch_ms: u64, out: []u8): StatHashError!usize {
    if (out.len < STAT_HTTP_DATE_LEN) return StatHashError.BufferTooSmall;

    const total_secs = epoch_ms / 1000;
    const days = total_secs / 86400;
    const rem_secs = total_secs % 86400;
    const hour = rem_secs / 3600;
    const minute = (rem_secs % 3600) / 60;
    const second = rem_secs % 60;

    // Howard Hinnant civil-from-days algorithm (UTC, Gregorian proleptic)
    const z = days + 719468;
    const era = z / 146097;
    const doe = z - era * 146097;
    const yoe = (doe - doe / 1460 + doe / 36524 - doe / 146096) / 365;
    let year = yoe + era * 400;
    const doy = doe - (365 * yoe + yoe / 4 - yoe / 100);
    const mp = (5 * doy + 2) / 153;
    const day = doy - (153 * mp + 2) / 5 + 1;
    const month = if (mp < 10) (mp + 3) else (mp - 9);
    if (month <= 2) year += 1;

    if (year > 9999) return StatHashError.YearOutOfRange;

    // 1970-01-01 was Thursday (4)
    const wday = (days + 4) % 7;

    writeAscii3(out, 0, weekdayName(wday));
    out[3] = ',';
    out[4] = ' ';
    write2Digits(out, 5, day);
    out[7] = ' ';
    writeAscii3(out, 8, monthName(month));
    out[11] = ' ';
    write4Digits(out, 12, year);
    out[16] = ' ';
    write2Digits(out, 17, hour);
    out[19] = ':';
    write2Digits(out, 20, minute);
    out[22] = ':';
    write2Digits(out, 23, second);
    out[25] = ' ';
    out[26] = 'G';
    out[27] = 'M';
    out[28] = 'T';

    return STAT_HTTP_DATE_LEN;
}

/// Zero-alloc filesystem metadata fingerprint with cached HTTP `Last-Modified` header
/// (aligned with Bun `src/resolver/fs/stat_hash.cp`).
pub struct FileStatHash {
    value: u64 = 0,
    last_modified_u64: u64 = 0,
    last_modified_buffer: [32]u8 = [0; 32],
    last_modified_buffer_len: u8 = 0,

    static fn init(): FileStatHash {
        return FileStatHash{};
    }

    /// Recomputes the 64-bit stat fingerprint from `(meta, path)` and updates
    /// the cached `Last-Modified` HTTP date when the hash changes for a regular file.
    /// Returns `true` if the fingerprint changed relative to its previous value.
    fn update(*_, meta: FileStatMeta, path: []const u8): bool {
        let h = mixHashU64(STAT_HASH_SEED, meta.size);
        h = mixHashU64(h, @as(u64, meta.mode));
        h = mixHashU64(h, @as(u64, @bitCast(meta.mtime_sec)));
        h = mixHashU64(h, @as(u64, @bitCast(meta.mtime_nsec)));
        h = mixHashU64(h, meta.ino);
        h = mixHashBytes(h, path);

        const prev = _.value;
        _.value = h;
        const changed = prev != h;

        if (changed && meta.isRegularFile()) {
            const ms = meta.mtimeMsUnsigned();
            if (ms > 0) {
                const n = formatHttpDate(ms, _.last_modified_buffer[0..]) catch 0;
                _.last_modified_buffer_len = @as(u8, @intCast(n));
                _.last_modified_u64 = if (n > 0) ms else 0;
            } else {
                _.last_modified_buffer_len = 0;
                _.last_modified_u64 = 0;
            }
        } else if (!meta.isRegularFile()) {
            _.last_modified_buffer_len = 0;
            _.last_modified_u64 = 0;
        }

        return changed;
    }

    /// Returns the cached RFC 9110 `Last-Modified` slice if present, or `null`.
    fn lastModified(): ([]const u8)? {
        if (_.last_modified_buffer_len == 0) return null;
        return _.last_modified_buffer[0..@as(usize, _.last_modified_buffer_len)];
    }

    fn hasLastModified(): bool {
        return _.last_modified_buffer_len > 0;
    }

    /// Formats a strong HTTP ETag (`"<16-hex>"`, 18 bytes) into `out`.
    fn formatStrongEtag(out: []u8): StatHashError!usize {
        if (out.len < STAT_STRONG_ETAG_LEN) return StatHashError.BufferTooSmall;
        out[0] = '"';
        writeHexU64(out, 1, _.value);
        out[17] = '"';
        return STAT_STRONG_ETAG_LEN;
    }

    /// Formats a weak HTTP ETag (`W/"<16-hex>"`, 20 bytes) into `out`.
    fn formatWeakEtag(out: []u8): StatHashError!usize {
        if (out.len < STAT_WEAK_ETAG_LEN) return StatHashError.BufferTooSmall;
        out[0] = 'W';
        out[1] = '/';
        out[2] = '"';
        writeHexU64(out, 3, _.value);
        out[19] = '"';
        return STAT_WEAK_ETAG_LEN;
    }

    fn eql(other: FileStatHash): bool {
        return _.value == other.value && _.last_modified_u64 == other.last_modified_u64;
    }
}
