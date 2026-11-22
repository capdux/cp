// Source: Absorbed from /data/dev/bun/src/install_types/SemverString.cp | Synced: 2026-11-22T18:51:30Z
// ============================================================================
// lib/semver/semverString.cp — Zero-Alloc 8-Byte Inline/Pooled SemverString
// ============================================================================
// Implements Bun's 8-byte tagged small-string representation for SemVer tags,
// package names, and lockfile identifiers:
// - Bit 63 (`bytes[7] & 0x80`) == 0: inline small string (up to 8 ASCII bytes,
//   zero-terminated when < 8 bytes), enabling O(1) value comparison with zero
//   external memory dereferences.
// - Bit 63 == 1: little-endian `(off: u32, len: u31)` pointer into a shared
//   string buffer (`SemverStringPool`), with hash-based deduplication and
//   store-path escaping (`/` `\` `:` `#` -> `+`).

pub const SEMVER_STRING_MAX_INLINE_LEN: usize = 8;
pub const SEMVER_POOL_MAX_BYTES: usize = 4096;
pub const SEMVER_POOL_MAX_ENTRIES: usize = 128;

pub err SemverStringError {
    NotInlinable,
    PoolBytesFull,
    PoolEntriesFull,
    InvalidPointer,
    BufferTooSmall,
}

pub enum SemverStringTag {
    small,
    big,
}

pub struct SemverStringPointer {
    off: u32 = 0,
    len: u32 = 0,
}

/// Deterministic 64-bit FNV-1a hash for `SemverExternalString` and `SemverStringPool` deduplication.
pub fn semverStringHash(s: []const u8): u64 {
    let h: u64 = 14695981039346656037;
    let i: usize = 0;
    while (i < s.len) {
        h = h ^ @as(u64, s[i]);
        h = h *% 1099511628211;
        i += 1;
    }
    return h;
}

fn readU32Le(b: [8]u8, offset: usize): u32 {
    const b0: u32 = @as(u32, b[offset]);
    const b1: u32 = @as(u32, b[offset + 1]) << 8;
    const b2: u32 = @as(u32, b[offset + 2]) << 16;
    const b3: u32 = @as(u32, b[offset + 3]) << 24;
    return b0 | b1 | b2 | b3;
}

fn writeU32Le(out: *[8]u8, offset: usize, val: u32) {
    out.*[offset] = @as(u8, @truncate(val));
    out.*[offset + 1] = @as(u8, @truncate(val >> 8));
    out.*[offset + 2] = @as(u8, @truncate(val >> 16));
    out.*[offset + 3] = @as(u8, @truncate(val >> 24));
}

fn bytesEqual(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) {
        if (a[i] != b[i]) return false;
        i += 1;
    }
    return true;
}

/// 8-byte compact string: stores <= 8 bytes inline when bit 63 is 0, or a
/// `(off: u32, len: u31)` pointer into a shared buffer when bit 63 is 1.
pub struct SemverString {
    bytes: [8]u8 = [0; 8],

    static fn empty(): SemverString {
        return SemverString{ bytes: [0; 8] };
    }

    /// Returns true if `s` can be stored inline inside the 8-byte representation
    /// without ambiguity (no embedded NUL bytes, length <= 7 or length == 8 with ASCII last byte).
    static fn canInline(s: []const u8): bool {
        if (s.len > SEMVER_STRING_MAX_INLINE_LEN) return false;
        if (s.len == SEMVER_STRING_MAX_INLINE_LEN && (s[7] & 0x80) != 0) return false;
        let i: usize = 0;
        while (i < s.len) {
            if (s[i] == 0) return false;
            i += 1;
        }
        return true;
    }

    /// Constructs an inline `SemverString`. Fails with `NotInlinable` if `s` cannot be inlined.
    static fn initInline(s: []const u8): SemverStringError!SemverString {
        if (!SemverString.canInline(s)) return SemverStringError.NotInlinable;
        let b: [8]u8 = [0; 8];
        let i: usize = 0;
        while (i < s.len) {
            b[i] = s[i];
            i += 1;
        }
        return SemverString{ bytes: b };
    }

    /// Constructs an external pointer `SemverString` referencing `buf[off .. off + length]`.
    static fn initPointer(off: u32, length: u32): SemverStringError!SemverString {
        if ((length & 0x80000000) != 0) return SemverStringError.InvalidPointer;
        let b: [8]u8 = [0; 8];
        writeU32Le(&b, 0, off);
        writeU32Le(&b, 4, length | 0x80000000);
        return SemverString{ bytes: b };
    }

    fn isInline(): bool {
        return (_.bytes[7] & 0x80) == 0;
    }

    fn tag(): SemverStringTag {
        if (_.isInline()) return SemverStringTag.small;
        return SemverStringTag.big;
    }

    fn isEmpty(): bool {
        if (!_.isInline()) {
            return _.ptr().len == 0;
        }
        return _.bytes[0] == 0;
    }

    fn ptr(): SemverStringPointer {
        const off = readU32Le(_.bytes, 0);
        const len_tagged = readU32Le(_.bytes, 4);
        return SemverStringPointer{
            off: off,
            len: len_tagged & 0x7FFFFFFF,
        };
    }

    fn len(): usize {
        if (_.isInline()) {
            let i: usize = 0;
            while (i < SEMVER_STRING_MAX_INLINE_LEN) {
                if (_.bytes[i] == 0) return i;
                i += 1;
            }
            return SEMVER_STRING_MAX_INLINE_LEN;
        }
        return @as(usize, _.ptr().len);
    }

    /// Returns the slice from `buf` for an external (non-inline) `SemverString`.
    fn sliceExternal(buf: []const u8): SemverStringError![]const u8 {
        if (_.isInline()) return SemverStringError.InvalidPointer;
        const p = _.ptr();
        const start = @as(usize, p.off);
        const length = @as(usize, p.len);
        if (start > buf.len || length > buf.len - start) {
            return SemverStringError.InvalidPointer;
        }
        return buf[start..(start + length)];
    }

    /// Returns the byte at `idx` (works uniformly for both inline and external strings).
    fn byteAt(buf: []const u8, idx: usize): u8? {
        if (_.isInline()) {
            const n = _.len();
            if (idx >= n) return null;
            return _.bytes[idx];
        }
        const p = _.ptr();
        const start = @as(usize, p.off);
        const length = @as(usize, p.len);
        if (idx >= length || start > buf.len || length > buf.len - start) {
            return null;
        }
        return buf[start + idx];
    }

    /// Copies the logical string bytes into `out` and returns the number of bytes written.
    fn copyTo(buf: []const u8, out: []u8): SemverStringError!usize {
        if (_.isInline()) {
            const n = _.len();
            if (n > out.len) return SemverStringError.BufferTooSmall;
            let i: usize = 0;
            while (i < n) {
                out[i] = _.bytes[i];
                i += 1;
            }
            return n;
        }
        const ext = _.sliceExternal(buf)?;
        if (ext.len > out.len) return SemverStringError.BufferTooSmall;
        let i: usize = 0;
        while (i < ext.len) {
            out[i] = ext[i];
            i += 1;
        }
        return ext.len;
    }

    /// Compares two `SemverString` values for byte equality across their respective buffers.
    fn eql(other: SemverString, this_buf: []const u8, other_buf: []const u8): bool {
        if (_.isInline() && other.isInline()) {
            let i: usize = 0;
            while (i < SEMVER_STRING_MAX_INLINE_LEN) {
                if (_.bytes[i] != other.bytes[i]) return false;
                i += 1;
            }
            return true;
        }
        const n1 = _.len();
        const n2 = other.len();
        if (n1 != n2) return false;
        let i: usize = 0;
        while (i < n1) {
            const c1 = _.byteAt(this_buf, i) ?? return false;
            const c2 = other.byteAt(other_buf, i) ?? return false;
            if (c1 != c2) return false;
            i += 1;
        }
        return true;
    }

    /// Lexicographical byte ordering: returns `-1` (lt), `0` (eq), or `1` (gt).
    fn order(other: SemverString, this_buf: []const u8, other_buf: []const u8): i32 {
        const n1 = _.len();
        const n2 = other.len();
        const min_n = if (n1 < n2) n1 else n2;
        let i: usize = 0;
        while (i < min_n) {
            const c1 = _.byteAt(this_buf, i) ?? 0;
            const c2 = other.byteAt(other_buf, i) ?? 0;
            if (c1 < c2) return -1;
            if (c1 > c2) return 1;
            i += 1;
        }
        if (n1 < n2) return -1;
        if (n1 > n2) return 1;
        return 0;
    }

    /// Formats the string as a safe package store path segment (replacing `/`, `\`, `:`, `#`
    /// with `+`, matching Bun's `SemverStringStorePathFormatter`).
    fn formatStorePath(buf: []const u8, out: []u8): SemverStringError!usize {
        const n = _.len();
        if (n > out.len) return SemverStringError.BufferTooSmall;
        let i: usize = 0;
        while (i < n) {
            const c = _.byteAt(buf, i) ?? return SemverStringError.InvalidPointer;
            if (c == '/' || c == '\\' || c == ':' || c == '#') {
                out[i] = '+';
            } else {
                out[i] = c;
            }
            i += 1;
        }
        return n;
    }
}

/// Pair of compact `SemverString` + precomputed 64-bit hash for O(1) rejection
/// (aligned with Bun's `Semver.ExternalString`).
pub struct SemverExternalString {
    value: SemverString = SemverString.empty(),
    hash: u64 = 0,

    static fn initInline(s: []const u8): SemverStringError!SemverExternalString {
        const v = SemverString.initInline(s)?;
        return SemverExternalString{
            value: v,
            hash: semverStringHash(s),
        };
    }

    fn eqlFast(other: SemverExternalString, this_buf: []const u8, other_buf: []const u8): bool {
        if (_.hash != other.hash) return false;
        return _.value.eql(other.value, this_buf, other_buf);
    }
}

/// Fixed-capacity zero-allocation string interning pool (aligned with Bun's
/// `SemverStringBuf` / `SemverStringBuilder`). Short strings (<= 8 ASCII bytes)
/// are inlined without consuming pool space; longer strings are deduplicated by
/// 64-bit hash + exact byte match.
pub struct SemverStringPool {
    data: [SEMVER_POOL_MAX_BYTES]u8 = [0; SEMVER_POOL_MAX_BYTES],
    data_len: usize = 0,
    hashes: [SEMVER_POOL_MAX_ENTRIES]u64 = [0; SEMVER_POOL_MAX_ENTRIES],
    entries: [SEMVER_POOL_MAX_ENTRIES]SemverString = [SemverString.empty(); SEMVER_POOL_MAX_ENTRIES],
    entry_count: usize = 0,

    static fn init(): SemverStringPool {
        return SemverStringPool{};
    }

    fn usedBytes(): usize {
        return _.data_len;
    }

    fn dedupCount(): usize {
        return _.entry_count;
    }

    fn bytes(): []const u8 {
        return _.data[0.._.data_len];
    }

    /// Interns `s`: returns an inline `SemverString` when possible without touching the pool;
    /// otherwise deduplicates and appends to the fixed byte arena.
    fn intern(*_, s: []const u8): SemverStringError!SemverString {
        if (SemverString.canInline(s)) {
            return SemverString.initInline(s);
        }
        const h = semverStringHash(s);
        let idx: usize = 0;
        while (idx < _.entry_count) {
            if (_.hashes[idx] == h) {
                const existing = _.entries[idx];
                const ext = existing.sliceExternal(_.data[0.._.data_len])?;
                if (bytesEqual(ext, s)) {
                    return existing;
                }
            }
            idx += 1;
        }

        if (_.entry_count >= SEMVER_POOL_MAX_ENTRIES) {
            return SemverStringError.PoolEntriesFull;
        }
        if (s.len > SEMVER_POOL_MAX_BYTES - _.data_len) {
            return SemverStringError.PoolBytesFull;
        }

        const start_off = _.data_len;
        let i: usize = 0;
        while (i < s.len) {
            _.data[start_off + i] = s[i];
            i += 1;
        }
        _.data_len = start_off + s.len;

        const created = SemverString.initPointer(@as(u32, @intCast(start_off)), @as(u32, @intCast(s.len)))?;
        _.hashes[_.entry_count] = h;
        _.entries[_.entry_count] = created;
        _.entry_count += 1;
        return created;
    }

    /// Interns `s` and returns a `SemverExternalString` carrying its 64-bit hash.
    fn internExternal(*_, s: []const u8): SemverStringError!SemverExternalString {
        const h = semverStringHash(s);
        const v = _.intern(s)?;
        return SemverExternalString{ value: v, hash: h };
    }

    /// Resolves any `SemverString` (inline or pooled) into caller-provided `out`.
    fn resolve(s: SemverString, out: []u8): SemverStringError!usize {
        return s.copyTo(_.data[0.._.data_len], out);
    }
}
