// Source: Absorbed from /data/dev/bun/src/bundler/IndexStringMap.cp | Synced: 2026-11-22T12:22:00Z
// Pure-CP zero-allocation `IndexStringMap` (`SourceIndex: u32 -> []const u8`) with
// 256-slot open-addressing hash table, tombstone-safe removal, optional inline 8 KiB
// copy-arena (`putCopy`, replacing heap `dupe` for ephemeral strings), reverse lookup,
// and deterministic index-sorted export for manifest / metafile emission.

/// 开放寻址哈希表槽位数（2 的幂次）
pub const INDEX_STR_MAP_BUCKETS: usize = 256;

/// 最大活跃条目数（75% 负载因子上限）
pub const INDEX_STR_MAP_MAX_LOAD: usize = 192;

/// 内置零堆字符串拷贝池容量（字节）
pub const INDEX_STR_MAP_ARENA_CAP: usize = 8192;

/// `IndexStringMap` 操作错误集
pub err IndexStringMapError {
    MapFull,
    ArenaFull,
    BufferTooSmall,
}

fn bytesEql(a: []const u8, b: []const u8): bool {
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

/// 32 位整数雪崩哈希（SplitMix32 / Murmur3 finalizer）
fn hashU32(x: u32): u32 {
    let h = x;
    h ^= h >> 16;
    h = h *% 0x85ebca6b;
    h ^= h >> 13;
    h = h *% 0xc2b2ae35;
    h ^= h >> 16;
    return h;
}

/// 导出条目视图
pub struct IndexStringEntry {
    index: u32 = 0,
    value: []const u8 = "",
}

enum IdxBucketState: u8 {
    Empty = 0,
    Occupied = 1,
    Tombstone = 2,
}

struct IdxBucket {
    state: IdxBucketState = .Empty,
    key: u32 = 0,
    // 若 is_arena == true，值存储在 map.arena[arena_off .. arena_off + arena_len]
    is_arena: bool = false,
    arena_off: u32 = 0,
    arena_len: u32 = 0,
    // 若 is_arena == false，直接持有外部零拷贝只读切片
    ext_slice: []const u8 = "",
}

/// 零堆分配 `u32 -> string` 映射表（对齐 Bun `IndexStringMap`）
pub struct IndexStringMap {
    buckets: [INDEX_STR_MAP_BUCKETS]IdxBucket = [IdxBucket{}; INDEX_STR_MAP_BUCKETS],
    len: usize = 0,
    tombstones: usize = 0,
    arena: [INDEX_STR_MAP_ARENA_CAP]u8 = undefined,
    arena_used: usize = 0,

    static fn init(): IndexStringMap {
        return IndexStringMap{};
    }

    fn count(): usize {
        return _.len;
    }

    fn isEmpty(): bool {
        return _.len == 0;
    }

    fn arenaUsed(): usize {
        return _.arena_used;
    }

    fn arenaRemaining(): usize {
        if (_.arena_used >= INDEX_STR_MAP_ARENA_CAP) {
            return 0;
        }
        return INDEX_STR_MAP_ARENA_CAP - _.arena_used;
    }

    fn sliceForBucket(b: IdxBucket): []const u8 {
        if (b.is_arena) {
            const start: usize = @intCast(b.arena_off);
            const end: usize = start + @as(usize, @intCast(b.arena_len));
            return _.arena[start..end];
        }
        return b.ext_slice;
    }

    fn findSlot(index: u32): usize? {
        const mask: usize = INDEX_STR_MAP_BUCKETS - 1;
        let slot: usize = @intCast(hashU32(index) & @as(u32, @intCast(mask)));
        let steps: usize = 0;
        while (steps < INDEX_STR_MAP_BUCKETS) {
            const st = _.buckets[slot].state;
            if (st == .Empty) {
                return null;
            }
            if (st == .Occupied && _.buckets[slot].key == index) {
                return slot;
            }
            slot = (slot + 1) & mask;
            steps += 1;
        }
        return null;
    }

    /// O(1) 查询 `index` 对应的字符串切片
    fn get(index: u32): ([]const u8)? {
        if (slot := _.findSlot(index)) {
            return _.sliceForBucket(_.buckets[slot]);
        }
        return null;
    }

    fn contains(index: u32): bool {
        return _.findSlot(index) != null;
    }

    fn findInsertSlot(index: u32, out_existing: *bool): usize! {
        out_existing.* = false;
        const mask: usize = INDEX_STR_MAP_BUCKETS - 1;
        let slot: usize = @intCast(hashU32(index) & @as(u32, @intCast(mask)));
        let first_tombstone: usize? = null;
        let steps: usize = 0;

        while (steps < INDEX_STR_MAP_BUCKETS) {
            const st = _.buckets[slot].state;
            if (st == .Empty) {
                const target = first_tombstone ?? slot;
                if (_.len >= INDEX_STR_MAP_MAX_LOAD) {
                    return err.MapFull;
                }
                return target;
            }
            if (st == .Occupied && _.buckets[slot].key == index) {
                out_existing.* = true;
                return slot;
            }
            if (st == .Tombstone && first_tombstone == null) {
                first_tombstone = slot;
            }
            slot = (slot + 1) & mask;
            steps += 1;
        }

        if (target := first_tombstone) {
            if (_.len >= INDEX_STR_MAP_MAX_LOAD) {
                return err.MapFull;
            }
            return target;
        }
        return err.MapFull;
    }

    /// 零拷贝存入外部只读切片，返回是否覆写了已有键
    fn put(*_, index: u32, value: []const u8): bool! {
        let existing = false;
        const slot = _.findInsertSlot(index, *existing)?;
        if (!existing) {
            if (_.buckets[slot].state == .Tombstone && _.tombstones > 0) {
                _.tombstones -= 1;
            }
            _.len += 1;
        }
        _.buckets[slot] = IdxBucket{
            state: .Occupied,
            key: index,
            is_arena: false,
            arena_off: 0,
            arena_len: 0,
            ext_slice: value,
        };
        return existing;
    }

    /// 将 `value` 拷贝至内置 8 KiB 零堆字节池中并绑定到 `index`（替代堆 `dupe`，安全持有临时缓冲数据）
    fn putCopy(*_, index: u32, value: []const u8): bool! {
        if (value.len > _.arenaRemaining()) {
            return err.ArenaFull;
        }
        let existing = false;
        const slot = _.findInsertSlot(index, *existing)?;
        const off = _.arena_used;
        if (value.len > 0) {
            @memcpy(_.arena[off .. off + value.len], value);
        }
        _.arena_used += value.len;
        if (!existing) {
            if (_.buckets[slot].state == .Tombstone && _.tombstones > 0) {
                _.tombstones -= 1;
            }
            _.len += 1;
        }
        _.buckets[slot] = IdxBucket{
            state: .Occupied,
            key: index,
            is_arena: true,
            arena_off: @intCast(off),
            arena_len: @intCast(value.len),
            ext_slice: "",
        };
        return existing;
    }

    /// 若 `index` 已存在则返回现有字符串，否则存入 `default_value` 并返回
    fn getOrPut(*_, index: u32, default_value: []const u8): ([]const u8)! {
        if (existing := _.get(index)) {
            return existing;
        }
        * = _.put(index, default_value)?;
        return default_value;
    }

    /// 墓碑安全删除指定 `index`，返回是否命中
    fn remove(*_, index: u32): bool {
        if (slot := _.findSlot(index)) {
            _.buckets[slot].state = .Tombstone;
            _.buckets[slot].is_arena = false;
            _.buckets[slot].arena_off = 0;
            _.buckets[slot].arena_len = 0;
            _.buckets[slot].ext_slice = "";
            if (_.len > 0) {
                _.len -= 1;
            }
            _.tombstones += 1;
            return true;
        }
        return false;
    }

    /// 按字符串值反查对应的 `source_index`
    fn findIndexByValue(value: []const u8): u32? {
        let i: usize = 0;
        while (i < INDEX_STR_MAP_BUCKETS) {
            if (_.buckets[i].state == .Occupied) {
                if (bytesEql(_.sliceForBucket(_.buckets[i]), value)) {
                    return _.buckets[i].key;
                }
            }
            i += 1;
        }
        return null;
    }

    /// 按 `index` 升序导出全部活跃条目至 `out`，用于确定性 manifest / metafile 生成
    fn collectSortedEntries(out: []IndexStringEntry): usize! {
        if (out.len < _.len) {
            return err.BufferTooSmall;
        }
        let n: usize = 0;
        let i: usize = 0;
        while (i < INDEX_STR_MAP_BUCKETS) {
            if (_.buckets[i].state == .Occupied) {
                out[n] = IndexStringEntry{
                    index: _.buckets[i].key,
                    value: _.sliceForBucket(_.buckets[i]),
                };
                n += 1;
            }
            i += 1;
        }
        // 原地插入排序，保证按 index 严格升序
        let a: usize = 1;
        while (a < n) {
            const key = out[a];
            let b = a;
            while (b > 0 && out[b - 1].index > key.index) {
                out[b] = out[b - 1];
                b -= 1;
            }
            out[b] = key;
            a += 1;
        }
        return n;
    }

    fn clear(*_) {
        let i: usize = 0;
        while (i < INDEX_STR_MAP_BUCKETS) {
            _.buckets[i] = IdxBucket{};
            i += 1;
        }
        _.len = 0;
        _.tombstones = 0;
        _.arena_used = 0;
    }
}
