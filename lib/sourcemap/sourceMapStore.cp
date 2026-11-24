// Source: Absorbed from /data/dev/bun/src/bake/DevServer/SourceMapStore.cp | Synced: 2026-11-23T21:02:19Z
// Pure-CP zero-allocation DevServer SourceMapStore (`/_bun/client/{id}.js.map`):
// 64-bit tagged StoreSourceId (InitialResponse generation vs HmrChunk 63-bit hash),
// strong reference counting with a bounded 16-entry / 10-second weak-ref grace cache
// so browser DevTools can lazily fetch maps after HMR invalidation without OOM.

pub const SOURCEMAP_WEAK_REF_EXPIRY_SECS: u64 = 10;
pub const SOURCEMAP_WEAK_REF_MAX: usize = 16;
pub const SOURCEMAP_STORE_MAX_ENTRIES: usize = 128;
pub const SOURCEMAP_URL_PREFIX: []const u8 = "/_bun/client/";
pub const SOURCEMAP_URL_SUFFIX: []const u8 = ".js.map";

pub error SourceMapStoreError {
    CapacityExceeded,
    NotFound,
    DoubleUnref,
    BufferTooSmall,
}

pub enum SourceMapChunkKind: u8 {
    InitialResponse = 0,
    HmrChunk = 1,
}

/// 64-bit packed identifier mirroring Bun `StoreSourceId`:
/// bit 63 = `SourceMapChunkKind` (0 = InitialResponse, 1 = HmrChunk),
/// bits 0..62 = `generation_id: u32` or 63-bit `content_hash`.
pub struct SourceMapStoreId {
    raw: u64 = 0,

    static fn forInitialResponse(generation_id: u32): SourceMapStoreId {
        return SourceMapStoreId{ raw: @as(u64, generation_id) };
    }

    static fn forHmrChunk(content_hash: u64): SourceMapStoreId {
        const mask63: u64 = 0x7FFF_FFFF_FFFF_FFFF;
        const tag_bit: u64 = 0x8000_0000_0000_0000;
        return SourceMapStoreId{ raw: tag_bit | (content_hash & mask63) };
    }

    fn kind(): SourceMapChunkKind {
        return if ((_.raw & 0x8000_0000_0000_0000) != 0) .HmrChunk else .InitialResponse;
    }

    fn generationId(): u32 {
        return @as(u32, _.raw & 0xFFFF_FFFF);
    }

    fn contentHash(): u64 {
        return _.raw & 0x7FFF_FFFF_FFFF_FFFF;
    }

    /// Formats `/_bun/client/<16-lower-hex>.js.map` into caller-provided `out`.
    fn formatUrl(out: []u8): []const u8 ! SourceMapStoreError {
        const total = SOURCEMAP_URL_PREFIX.len + 16 + SOURCEMAP_URL_SUFFIX.len;
        if (total > out.len) return err.BufferTooSmall;
        let pos: usize = 0;
        let i: usize = 0;
        while (i < SOURCEMAP_URL_PREFIX.len) {
            out[pos] = SOURCEMAP_URL_PREFIX[i];
            pos += 1;
            i += 1;
        }
        const hex = "0123456789abcdef";
        let shift: u6 = 60;
        let d: usize = 0;
        while (d < 16) {
            const nib: usize = @as(usize, (_.raw >> shift) & 0xF);
            out[pos] = hex[nib];
            pos += 1;
            if (shift >= 4) shift -= 4;
            d += 1;
        }
        i = 0;
        while (i < SOURCEMAP_URL_SUFFIX.len) {
            out[pos] = SOURCEMAP_URL_SUFFIX[i];
            pos += 1;
            i += 1;
        }
        return out[0..pos];
    }
}

pub struct SourceMapStoreEntry {
    id: SourceMapStoreId = SourceMapStoreId{},
    ref_count: u32 = 0,
    file_count: u32 = 0,
    memory_bytes: usize = 0,
    weak_since_sec: u64 = 0,
    weak_seq: u64 = 0,

    fn isWeak(): bool {
        return _.ref_count == 0;
    }

    fn isExpiredAt(now_sec: u64): bool {
        if (_.ref_count > 0) return false;
        return now_sec >= _.weak_since_sec + SOURCEMAP_WEAK_REF_EXPIRY_SECS;
    }
}

/// Zero-allocation DevServer SourceMapStore with strong/weak reference lifecycle.
pub struct SourceMapStore {
    entries: [SOURCEMAP_STORE_MAX_ENTRIES]SourceMapStoreEntry = [SourceMapStoreEntry{}; SOURCEMAP_STORE_MAX_ENTRIES],
    entry_count: usize = 0,
    next_weak_seq: u64 = 1,

    static fn init(): SourceMapStore {
        return SourceMapStore{};
    }

    fn totalCount(): usize {
        return _.entry_count;
    }

    fn strongCount(): usize {
        let total: usize = 0;
        let i: usize = 0;
        while (i < _.entry_count) {
            if (_.entries[i].ref_count > 0) total += 1;
            i += 1;
        }
        return total;
    }

    fn weakCount(): usize {
        let total: usize = 0;
        let i: usize = 0;
        while (i < _.entry_count) {
            if (_.entries[i].ref_count == 0) total += 1;
            i += 1;
        }
        return total;
    }

    fn totalMemoryBytes(): usize {
        let total: usize = 0;
        let i: usize = 0;
        while (i < _.entry_count) {
            total += _.entries[i].memory_bytes;
            i += 1;
        }
        return total;
    }

    /// Inserts a new SourceMap entry with `ref_count = 1`, or increments `ref_count` if already present
    /// (reviving it from the weak-ref pool if `ref_count` was 0).
    /// Returns `true` if newly inserted, `false` if an existing entry was incremented/revived.
    fn putOrRef(*_, id: SourceMapStoreId, file_count: u32, memory_bytes: usize): bool ! SourceMapStoreError {
        if (idx := _.findIndex(id.raw)) {
            _.entries[idx].ref_count += 1;
            _.entries[idx].file_count = file_count;
            _.entries[idx].memory_bytes = memory_bytes;
            return false;
        }
        if (_.entry_count >= SOURCEMAP_STORE_MAX_ENTRIES) {
            // Try evicting the oldest weak entry to make room before failing
            if (!_.evictOldestWeak()) {
                return err.CapacityExceeded;
            }
        }
        _.entries[_.entry_count] = SourceMapStoreEntry{
            id: id,
            ref_count: 1,
            file_count: file_count,
            memory_bytes: memory_bytes,
            weak_since_sec: 0,
            weak_seq: 0,
        };
        _.entry_count += 1;
        return true;
    }

    /// Looks up a SourceMap entry for DevTools serving at `now_sec`.
    /// Strong entries always resolve; weak entries resolve until `weak_since_sec + 10s`.
    fn get(id: SourceMapStoreId, now_sec: u64): SourceMapStoreEntry? {
        if (idx := _.findIndex(id.raw)) {
            const entry = _.entries[idx];
            if (entry.isExpiredAt(now_sec)) return null;
            return entry;
        }
        return null;
    }

    /// Decrements strong `ref_count`. When it drops to 0, transitions the entry into the bounded
    /// weak-ref grace pool (max 16 entries, 10s expiry) and evicts the oldest weak entry if needed.
    /// Returns `true` if the entry transitioned to weak state.
    fn unref(*_, id: SourceMapStoreId, now_sec: u64): bool ! SourceMapStoreError {
        if (idx := _.findIndex(id.raw)) {
            if (_.entries[idx].ref_count == 0) {
                return err.DoubleUnref;
            }
            _.entries[idx].ref_count -= 1;
            if (_.entries[idx].ref_count == 0) {
                _.entries[idx].weak_since_sec = now_sec;
                _.entries[idx].weak_seq = _.next_weak_seq;
                _.next_weak_seq += 1;
                // Enforce SOURCEMAP_WEAK_REF_MAX (16) cap by evicting oldest weak entry
                while (_.weakCount() > SOURCEMAP_WEAK_REF_MAX) {
                    if (!_.evictOldestWeak()) break;
                }
                return true;
            }
            return false;
        }
        return err.NotFound;
    }

    /// Physically removes all weak entries whose 10-second grace window has elapsed at `now_sec`.
    /// Returns the number of pruned entries.
    fn pruneExpired(*_, now_sec: u64): usize {
        let pruned: usize = 0;
        let i: usize = 0;
        while (i < _.entry_count) {
            if (_.entries[i].isExpiredAt(now_sec)) {
                _.swapRemoveAt(i);
                pruned += 1;
            } else {
                i += 1;
            }
        }
        return pruned;
    }

    /// Immediately removes an entry regardless of strong/weak state.
    fn removeImmediate(*_, id: SourceMapStoreId): bool {
        if (idx := _.findIndex(id.raw)) {
            _.swapRemoveAt(idx);
            return true;
        }
        return false;
    }

    // --- Internal helpers ---

    fn findIndex(raw_id: u64): usize? {
        let i: usize = 0;
        while (i < _.entry_count) {
            if (_.entries[i].id.raw == raw_id) return i;
            i += 1;
        }
        return null;
    }

    fn swapRemoveAt(*_, idx: usize) {
        const last = _.entry_count - 1;
        if (idx != last) {
            _.entries[idx] = _.entries[last];
        }
        _.entry_count -= 1;
    }

    fn evictOldestWeak(*_): bool {
        let found = false;
        let oldest_idx: usize = 0;
        let oldest_seq: u64 = 0;
        let i: usize = 0;
        while (i < _.entry_count) {
            if (_.entries[i].ref_count == 0) {
                if (!found || _.entries[i].weak_seq < oldest_seq) {
                    found = true;
                    oldest_idx = i;
                    oldest_seq = _.entries[i].weak_seq;
                }
            }
            i += 1;
        }
        if (found) {
            _.swapRemoveAt(oldest_idx);
            return true;
        }
        return false;
    }
}
