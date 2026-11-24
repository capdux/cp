// Source: Absorbed from /data/dev/bun/src/bun_alloc/heap_breakdown.cp | Synced: 2026-11-24T05:08:11Z
// ============================================================================
// lib/bun_alloc/heapBreakdown.cp — Zero-Alloc Named Allocation Zones & Heap
// Breakdown Profiler
// ============================================================================
// Pure cross-platform CP counterpart of Bun's named allocation zones
// (`Bun__<Label>`). Backed entirely by caller-provided `[]ZoneStats` slices
// (no hardcoded small zone limit, no OS-specific malloc_zone dependency),
// providing O(1) handle-based alloc/free/resize accounting, peak & leak
// tracking, deterministic descending sort, and zero-alloc report formatting.

pub const HEAP_ZONE_PREFIX: []const u8 = "Bun__";

pub err HeapBreakdownError {
    EmptyZoneName,
    DuplicateZone,
    ZoneTableFull,
    InvalidZoneHandle,
    UnderflowFree,
    BufferTooSmall,
}

fn strEql(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) {
        if (a[i] != b[i]) return false;
        i += 1;
    }
    return true;
}

fn appendSlice(out: []u8, pos: usize, src: []const u8): HeapBreakdownError!usize {
    if (src.len > out.len - pos) return HeapBreakdownError.BufferTooSmall;
    let i: usize = 0;
    while (i < src.len) {
        out[pos + i] = src[i];
        i += 1;
    }
    return pos + src.len;
}

fn appendByte(out: []u8, pos: usize, ch: u8): HeapBreakdownError!usize {
    if (pos >= out.len) return HeapBreakdownError.BufferTooSmall;
    out[pos] = ch;
    return pos + 1;
}

fn writeU64(out: []u8, pos: usize, val_in: u64): HeapBreakdownError!usize {
    let tmp: [24]u8 = [0; 24];
    let n: usize = 0;
    let v = val_in;
    if (v == 0) {
        tmp[0] = '0';
        n = 1;
    } else {
        while (v > 0) {
            tmp[n] = @as(u8, @intCast('0' + (v % 10)));
            v /= 10;
            n += 1;
        }
    }
    if (n > out.len - pos) return HeapBreakdownError.BufferTooSmall;
    let i: usize = 0;
    while (i < n) {
        out[pos + i] = tmp[n - 1 - i];
        i += 1;
    }
    return pos + n;
}

/// Strips module/namespace prefixes from a qualified type name (e.g. `"foo.bar.Baz"` -> `"Baz"`),
/// mirroring `bun.meta.typeBaseName(@typeName(T))` in `heap_breakdown.cp`.
pub fn heapZoneBaseName(qualified: []const u8): []const u8 {
    let last_dot: usize = qualified.len;
    let i: usize = 0;
    while (i < qualified.len) {
        if (qualified[i] == '.' || qualified[i] == ':') {
            last_dot = i;
        }
        i += 1;
    }
    if (last_dot < qualified.len) {
        return qualified[(last_dot + 1)..];
    }
    return qualified;
}

/// Formats a canonical Bun zone label (`"Bun__<name>"`) into caller buffer `out`.
pub fn formatHeapZoneLabel(name: []const u8, out: []u8): HeapBreakdownError!usize {
    if (name.len == 0) return HeapBreakdownError.EmptyZoneName;
    let pos = appendSlice(out, 0, HEAP_ZONE_PREFIX)?;
    pos = appendSlice(out, pos, name)?;
    return pos;
}

/// Lightweight O(1) index handle to a registered zone inside `HeapBreakdown`.
pub struct HeapZoneHandle {
    index: usize = 0,
}

/// Per-zone allocation metrics tracked by `HeapBreakdown`.
pub struct ZoneStats {
    name: []const u8 = "",
    live_bytes: u64 = 0,
    peak_bytes: u64 = 0,
    total_allocated_bytes: u64 = 0,
    total_freed_bytes: u64 = 0,
    alloc_count: u64 = 0,
    free_count: u64 = 0,
    resize_count: u64 = 0,
    active_allocations: u64 = 0,
    max_single_alloc: u64 = 0,

    static fn empty(): ZoneStats {
        return ZoneStats{};
    }

    fn hasLeaks(): bool {
        return _.live_bytes > 0 || _.active_allocations > 0;
    }

    fn avgAllocBytes(): u64 {
        if (_.alloc_count == 0) return 0;
        return _.total_allocated_bytes / _.alloc_count;
    }

    fn resetCounters(*_) {
        _.live_bytes = 0;
        _.peak_bytes = 0;
        _.total_allocated_bytes = 0;
        _.total_freed_bytes = 0;
        _.alloc_count = 0;
        _.free_count = 0;
        _.resize_count = 0;
        _.active_allocations = 0;
        _.max_single_alloc = 0;
    }
}

/// External-slice-driven named allocation zone registry & heap breakdown profiler.
pub struct HeapBreakdown {
    zones: []ZoneStats,
    len: usize = 0,

    /// Creates a `HeapBreakdown` backed by caller-owned `storage` (any capacity).
    static fn init(storage: []ZoneStats): HeapBreakdown {
        return HeapBreakdown{
            zones: storage,
            len: 0,
        };
    }

    fn capacity(): usize {
        return _.zones.len;
    }

    fn zoneCount(): usize {
        return _.len;
    }

    /// Finds an existing zone by exact name, returning its handle or `null`.
    fn findZone(name: []const u8): HeapZoneHandle? {
        let i: usize = 0;
        while (i < _.len) {
            if (strEql(_.zones[i].name, name)) {
                return HeapZoneHandle{ index: i };
            }
            i += 1;
        }
        return null;
    }

    /// Registers a new named zone; rejects empty names, duplicates, and full storage.
    fn registerZone(*_, name: []const u8): HeapBreakdownError!HeapZoneHandle {
        if (name.len == 0) return HeapBreakdownError.EmptyZoneName;
        if (_.findZone(name) != null) return HeapBreakdownError.DuplicateZone;
        if (_.len >= _.zones.len) return HeapBreakdownError.ZoneTableFull;
        const idx = _.len;
        _.zones[idx] = ZoneStats{ name: name };
        _.len += 1;
        return HeapZoneHandle{ index: idx };
    }

    /// Idempotent lookup-or-register (mirrors Bun `getZone(name)`).
    fn getOrRegisterZone(*_, name: []const u8): HeapBreakdownError!HeapZoneHandle {
        if (name.len == 0) return HeapBreakdownError.EmptyZoneName;
        if (existing := _.findZone(name)) {
            return existing;
        }
        return _.registerZone(name);
    }

    /// Returns a snapshot copy of the metrics for `handle`.
    fn stats(handle: HeapZoneHandle): HeapBreakdownError!ZoneStats {
        if (handle.index >= _.len) return HeapBreakdownError.InvalidZoneHandle;
        return _.zones[handle.index];
    }

    /// Records an allocation of `bytes` in `handle` (O(1) hot path).
    fn recordAlloc(*_, handle: HeapZoneHandle, bytes: usize): HeapBreakdownError!void {
        if (handle.index >= _.len) return HeapBreakdownError.InvalidZoneHandle;
        const b: u64 = @as(u64, bytes);
        _.zones[handle.index].live_bytes += b;
        if (_.zones[handle.index].live_bytes > _.zones[handle.index].peak_bytes) {
            _.zones[handle.index].peak_bytes = _.zones[handle.index].live_bytes;
        }
        _.zones[handle.index].total_allocated_bytes += b;
        _.zones[handle.index].alloc_count += 1;
        _.zones[handle.index].active_allocations += 1;
        if (b > _.zones[handle.index].max_single_alloc) {
            _.zones[handle.index].max_single_alloc = b;
        }
    }

    /// Records a deallocation of `bytes` in `handle`; guards against live_bytes or active underflow.
    fn recordFree(*_, handle: HeapZoneHandle, bytes: usize): HeapBreakdownError!void {
        if (handle.index >= _.len) return HeapBreakdownError.InvalidZoneHandle;
        const b: u64 = @as(u64, bytes);
        if (b > _.zones[handle.index].live_bytes || _.zones[handle.index].active_allocations == 0) {
            return HeapBreakdownError.UnderflowFree;
        }
        _.zones[handle.index].live_bytes -= b;
        _.zones[handle.index].total_freed_bytes += b;
        _.zones[handle.index].free_count += 1;
        _.zones[handle.index].active_allocations -= 1;
    }

    /// Records an in-place resize from `old_bytes` to `new_bytes` in `handle`.
    fn recordResize(*_, handle: HeapZoneHandle, old_bytes: usize, new_bytes: usize): HeapBreakdownError!void {
        if (handle.index >= _.len) return HeapBreakdownError.InvalidZoneHandle;
        const ob: u64 = @as(u64, old_bytes);
        const nb: u64 = @as(u64, new_bytes);
        if (ob > _.zones[handle.index].live_bytes) {
            return HeapBreakdownError.UnderflowFree;
        }
        if (nb >= ob) {
            const delta = nb - ob;
            _.zones[handle.index].live_bytes += delta;
            _.zones[handle.index].total_allocated_bytes += delta;
            if (_.zones[handle.index].live_bytes > _.zones[handle.index].peak_bytes) {
                _.zones[handle.index].peak_bytes = _.zones[handle.index].live_bytes;
            }
            if (nb > _.zones[handle.index].max_single_alloc) {
                _.zones[handle.index].max_single_alloc = nb;
            }
        } else {
            const delta = ob - nb;
            _.zones[handle.index].live_bytes -= delta;
            _.zones[handle.index].total_freed_bytes += delta;
        }
        _.zones[handle.index].resize_count += 1;
    }

    fn resetZone(*_, handle: HeapZoneHandle): HeapBreakdownError!void {
        if (handle.index >= _.len) return HeapBreakdownError.InvalidZoneHandle;
        _.zones[handle.index].resetCounters();
    }

    fn resetAll(*_) {
        let i: usize = 0;
        while (i < _.len) {
            _.zones[i].resetCounters();
            i += 1;
        }
    }

    fn totalLiveBytes(): u64 {
        let sum: u64 = 0;
        let i: usize = 0;
        while (i < _.len) {
            sum += _.zones[i].live_bytes;
            i += 1;
        }
        return sum;
    }

    fn totalPeakBytes(): u64 {
        let sum: u64 = 0;
        let i: usize = 0;
        while (i < _.len) {
            sum += _.zones[i].peak_bytes;
            i += 1;
        }
        return sum;
    }

    fn totalAllocatedBytes(): u64 {
        let sum: u64 = 0;
        let i: usize = 0;
        while (i < _.len) {
            sum += _.zones[i].total_allocated_bytes;
            i += 1;
        }
        return sum;
    }

    fn totalActiveAllocations(): u64 {
        let sum: u64 = 0;
        let i: usize = 0;
        while (i < _.len) {
            sum += _.zones[i].active_allocations;
            i += 1;
        }
        return sum;
    }

    fn hasLeaks(): bool {
        let i: usize = 0;
        while (i < _.len) {
            if (_.zones[i].hasLeaks()) return true;
            i += 1;
        }
        return false;
    }

    /// Returns the zone with the highest `live_bytes` (ties broken by `peak_bytes`), or `null` if empty.
    fn heaviestLiveZone(): HeapZoneHandle? {
        if (_.len == 0) return null;
        let best: usize = 0;
        let i: usize = 1;
        while (i < _.len) {
            const cur = _.zones[i];
            const top = _.zones[best];
            if (cur.live_bytes > top.live_bytes || (cur.live_bytes == top.live_bytes && cur.peak_bytes > top.peak_bytes)) {
                best = i;
            }
            i += 1;
        }
        return HeapZoneHandle{ index: best };
    }

    /// In-place deterministic insertion sort of registered zones by `live_bytes` descending
    /// (ties broken by `peak_bytes` descending). Note: invalidates previously cached handles.
    fn sortByLiveBytesDesc(*_) {
        let i: usize = 1;
        while (i < _.len) {
            const key = _.zones[i];
            let j = i;
            while (j > 0) {
                const prev = _.zones[j - 1];
                const should_move = key.live_bytes > prev.live_bytes
                    || (key.live_bytes == prev.live_bytes && key.peak_bytes > prev.peak_bytes);
                if (!should_move) break;
                _.zones[j] = prev;
                j -= 1;
            }
            _.zones[j] = key;
            i += 1;
        }
    }

    /// Formats a zero-alloc multi-line heap breakdown report into `out`.
    /// Each line has the format:
    /// `Bun__<name>: live=<bytes> peak=<bytes> allocs=<n> active=<n>\n`
    fn formatReport(out: []u8): HeapBreakdownError!usize {
        let pos: usize = 0;
        let i: usize = 0;
        while (i < _.len) {
            const z = _.zones[i];
            pos = appendSlice(out, pos, HEAP_ZONE_PREFIX)?;
            pos = appendSlice(out, pos, z.name)?;
            pos = appendSlice(out, pos, ": live=")?;
            pos = writeU64(out, pos, z.live_bytes)?;
            pos = appendSlice(out, pos, " peak=")?;
            pos = writeU64(out, pos, z.peak_bytes)?;
            pos = appendSlice(out, pos, " allocs=")?;
            pos = writeU64(out, pos, z.alloc_count)?;
            pos = appendSlice(out, pos, " active=")?;
            pos = writeU64(out, pos, z.active_allocations)?;
            pos = appendByte(out, pos, '\n')?;
            i += 1;
        }
        return pos;
    }
}
