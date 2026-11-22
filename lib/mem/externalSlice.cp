// Source: Absorbed from /data/dev/bun/src/install/ExternalSlice.cp | Synced: 2026-11-22T15:47:00Z
// ============================================================================
// lib/mem/externalSlice.cp — Compact 8-Byte Offset/Length External Slice Views
// ============================================================================
// Absorbed from Bun's lockfile & arena ExternalSlice<T> descriptor.
// Represents a slice as a relocatable `(off: u32, len: u32)` pair (8 bytes vs
// 16-byte pointer+length fat slice), surviving backing buffer reallocation and
// safe for binary serialization / mmap / zero-heap fixed pools.

pub const EXTERNAL_SLICE_INVALID_SENTINEL: u32 = 0xFFFF_FFFF;

/// Compact 8-byte `(off, len)` index handle into an external backing slice `[]const T`.
pub struct ExternalSlice<T> {
    off: u32 = 0,
    len: u32 = 0,

    static fn empty(): ExternalSlice<T> {
        return ExternalSlice<T>{ off: 0, len: 0 };
    }

    static fn invalid(): ExternalSlice<T> {
        return ExternalSlice<T>{
            off: EXTERNAL_SLICE_INVALID_SENTINEL,
            len: EXTERNAL_SLICE_INVALID_SENTINEL,
        };
    }

    static fn init(off: u32, len: u32): ExternalSlice<T> {
        return ExternalSlice<T>{ off: off, len: len };
    }

    static fn fromRange(begin_idx: u32, end_idx: u32): ExternalSlice<T>? {
        if (end_idx < begin_idx) return null;
        return ExternalSlice<T>{
            off: begin_idx,
            len: end_idx - begin_idx,
        };
    }

    fn isInvalid(): bool {
        return _.off == EXTERNAL_SLICE_INVALID_SENTINEL && _.len == EXTERNAL_SLICE_INVALID_SENTINEL;
    }

    fn isEmpty(): bool {
        return _.len == 0 && !_.isInvalid();
    }

    fn begin(): u32 {
        return _.off;
    }

    fn end(): u64 {
        return (@as(u64, _.off)) + (@as(u64, _.len));
    }

    fn contains(id: u32): bool {
        if (_.isInvalid()) return false;
        const id64: u64 = @as(u64, id);
        const start64: u64 = @as(u64, _.off);
        const end64: u64 = start64 + (@as(u64, _.len));
        return id64 >= start64 && id64 < end64;
    }

    fn isWithin(buf_len: usize): bool {
        if (_.isInvalid()) return false;
        const end64: u64 = _.end();
        return end64 <= (@as(u64, buf_len));
    }

    /// Clamping zero-copy read view (mirrors Bun's `ExternalSlice.get`).
    /// Returns an empty slice if invalid or if `off >= buf.len`.
    fn get(buf: []const T): []const T {
        if (_.isInvalid()) return buf[0..0];
        const start: usize = @as(usize, _.off);
        if (start >= buf.len) return buf[0..0];
        const avail: usize = buf.len - start;
        const want: usize = @as(usize, _.len);
        const take: usize = if (want < avail) want else avail;
        return buf[start .. start + take];
    }

    /// Strict bounds-checked read view. Returns `null` if invalid or out of bounds.
    fn getChecked(buf: []const T): ([]const T)? {
        if (!_.isWithin(buf.len)) return null;
        const start: usize = @as(usize, _.off);
        const end_idx: usize = start + (@as(usize, _.len));
        return buf[start..end_idx];
    }

    /// Clamping zero-copy mutable view (mirrors Bun's `ExternalSlice.mut`).
    fn mut(buf: []T): []T {
        if (_.isInvalid()) return buf[0..0];
        const start: usize = @as(usize, _.off);
        if (start >= buf.len) return buf[0..0];
        const avail: usize = buf.len - start;
        const want: usize = @as(usize, _.len);
        const take: usize = if (want < avail) want else avail;
        return buf[start .. start + take];
    }

    /// Strict bounds-checked mutable view. Returns `null` if invalid or out of bounds.
    fn mutChecked(buf: []T): ([]T)? {
        if (!_.isWithin(buf.len)) return null;
        const start: usize = @as(usize, _.off);
        const end_idx: usize = start + (@as(usize, _.len));
        return buf[start..end_idx];
    }

    /// Derives a sub-range within this ExternalSlice with overflow & bounds protection.
    fn subslice(rel_off: u32, rel_len: u32): ExternalSlice<T>? {
        if (_.isInvalid()) return null;
        const rel_end: u64 = (@as(u64, rel_off)) + (@as(u64, rel_len));
        if (rel_end > (@as(u64, _.len))) return null;
        const abs_off64: u64 = (@as(u64, _.off)) + (@as(u64, rel_off));
        if (abs_off64 >= (@as(u64, EXTERNAL_SLICE_INVALID_SENTINEL))) return null;
        return ExternalSlice<T>{
            off: @as(u32, @intCast(abs_off64)),
            len: rel_len,
        };
    }

    fn eql(other: ExternalSlice<T>): bool {
        return _.off == other.off && _.len == other.len;
    }
}

pub type ExternalByteSlice = ExternalSlice<u8>;

/// Paired external slice descriptor (mirrors Bun's `ExternalStringMap`),
/// mapping parallel `keys` and `values` ranges inside an external backing array.
pub struct ExternalSliceMap<T> {
    keys: ExternalSlice<T> = ExternalSlice<T>{ off: 0, len: 0 },
    values: ExternalSlice<T> = ExternalSlice<T>{ off: 0, len: 0 },

    static fn empty(): ExternalSliceMap<T> {
        return ExternalSliceMap<T>{
            keys: ExternalSlice<T>.empty(),
            values: ExternalSlice<T>.empty(),
        };
    }

    static fn init(keys: ExternalSlice<T>, values: ExternalSlice<T>): ExternalSliceMap<T> {
        return ExternalSliceMap<T>{ keys: keys, values: values };
    }

    fn isBalanced(): bool {
        return !_.keys.isInvalid() && !_.values.isInvalid() && _.keys.len == _.values.len;
    }

    fn count(): u32 {
        if (!_.isBalanced()) return 0;
        return _.keys.len;
    }

    fn getKeys(buf: []const T): []const T {
        return _.keys.get(buf);
    }

    fn getValues(buf: []const T): []const T {
        return _.values.get(buf);
    }
}

/// Zero-heap caller-backed byte pool producing compact `ExternalSlice<u8>` handles.
pub struct ExternalBytePool {
    buf: []u8,
    pos: usize = 0,

    static fn init(backing: []u8): ExternalBytePool {
        return ExternalBytePool{ buf: backing, pos: 0 };
    }

    fn used(): usize {
        return _.pos;
    }

    fn remaining(): usize {
        return _.buf.len - _.pos;
    }

    fn reset(*_) {
        _.pos = 0;
    }

    fn append(*_, data: []const u8): ExternalSlice<u8>? {
        if (data.len > _.buf.len - _.pos) return null;
        if (_.pos >= (@as(usize, EXTERNAL_SLICE_INVALID_SENTINEL))) return null;
        if (data.len >= (@as(usize, EXTERNAL_SLICE_INVALID_SENTINEL))) return null;
        const start = _.pos;
        let i: usize = 0;
        while (i < data.len) {
            _.buf[start + i] = data[i];
            i += 1;
        }
        _.pos = start + data.len;
        return ExternalSlice<u8>{
            off: @as(u32, @intCast(start)),
            len: @as(u32, @intCast(data.len)),
        };
    }

    fn get(handle: ExternalSlice<u8>): []const u8 {
        return handle.get(_.buf[0.._.pos]);
    }

    fn getChecked(handle: ExternalSlice<u8>): ([]const u8)? {
        return handle.getChecked(_.buf[0.._.pos]);
    }
}
