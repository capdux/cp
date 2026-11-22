// Source: Absorbed from /data/dev/bun/src/install/isolated_install/Store.cp | Synced: 2026-11-22T10:52:02Z
// Ported from Bun: /data/dev/bun/src/install/isolated_install/Store.cp & /data/dev/bun/docs/pm/isolated-installs.mdx
// Synced: 2026-11-22
// Pure-CP zero-allocation isolated `node_modules/.bun/` store path formatter,
// order-independent peer dependency set hasher, Global Virtual Store transitive closure
// entry hasher, symlink target planner, and deduplicated isolated store graph.

/// 隔离存储图支持的最大条目数
pub const ISOLATED_STORE_MAX_ENTRIES: usize = 64;

/// 单个条目支持的最大直接依赖边数
pub const ISOLATED_STORE_MAX_DEPS_PER_ENTRY: usize = 32;

/// 单次 Peer 集合哈希计算支持的最大 Peer 依赖数
pub const ISOLATED_STORE_MAX_PEERS: usize = 32;

/// 隔离存储路径格式化与图计算错误集
pub err IsolatedStoreError {
    EmptyPackageName,
    EmptyResolution,
    TooManyEntries,
    TooManyDeps,
    TooManyPeers,
    InvalidEntryId,
    NotEligibleForGlobalStore,
    BufferTooSmall,
}

/// 隔离存储条目类型（对齐 Bun `Store.cp` `_StorePathFormatter` & `entry_hash` 规则）
pub enum IsolatedEntryKind: u8 {
    /// 常规注册表/Tarball 包（参与 Global Virtual Store 共享）
    Registry = 0,
    /// 根工程包（不进 Global Store）
    Root = 1,
    /// `file:` 本地目录依赖（不进 Global Store）
    Folder = 2,
    /// Monorepo Workspace 包（直接软链源目录，不进 Global Store）
    Workspace = 3,
    /// 已应用 `patchedDependencies` 的补丁包（不进 Global Store）
    Patched = 4,
}

/// 单条 Peer 依赖引用（用于计算无序确定性 `peer_hash`）
pub struct IsolatedPeerRef {
    name: []const u8 = "",
    version: []const u8 = "",
}

/// 隔离存储条目（值语义，零堆分配）
pub struct IsolatedStoreEntry {
    id: u32 = 0,
    kind: IsolatedEntryKind = IsolatedEntryKind.Registry,
    name: []const u8 = "",
    resolution: []const u8 = "",
    top_level_dir_name: []const u8 = "",
    hoisted: bool = false,
    peer_hash: u64 = 0,
    entry_hash: u64 = 0,
    deps: [ISOLATED_STORE_MAX_DEPS_PER_ENTRY]u32 = [0; ISOLATED_STORE_MAX_DEPS_PER_ENTRY],
    dep_count: usize = 0,

    /// 是否允许共享 Global Virtual Store（`<cache>/links/<storepath>-<entry_hash>/`）
    fn supportsGlobalStore(): bool {
        return _.kind == IsolatedEntryKind.Registry;
    }

    fn hasPeerHash(): bool {
        return _.peer_hash != 0;
    }

    fn isScoped(): bool {
        return _.name.len > 0 && _.name[0] == '@';
    }
}

/// 零堆分配隔离存储依赖图（支持按 `name+resolution+peer_hash` 自动去重与闭包哈希计算）
pub struct IsolatedStoreGraph {
    entries: [ISOLATED_STORE_MAX_ENTRIES]IsolatedStoreEntry = [IsolatedStoreEntry{}; ISOLATED_STORE_MAX_ENTRIES],
    entry_count: usize = 0,

    static fn init(): IsolatedStoreGraph {
        return IsolatedStoreGraph{};
    }

    fn count(): usize {
        return _.entry_count;
    }

    fn entryAt(id: u32): IsolatedStoreEntry? {
        const idx = @as(usize, id);
        if (idx >= _.entry_count) {
            return null;
        }
        return _.entries[idx];
    }

    /// 注册或复用隔离存储条目（相同 `kind + name + resolution + peer_hash` 自动去重返回已有 ID）
    fn addOrFindEntry(
        *_,
        kind: IsolatedEntryKind,
        name: []const u8,
        resolution: []const u8,
        peer_hash: u64,
        hoisted: bool,
    ): u32! {
        if (kind != IsolatedEntryKind.Root && name.len == 0) {
            return IsolatedStoreError.EmptyPackageName;
        }
        if (kind != IsolatedEntryKind.Root && resolution.len == 0) {
            return IsolatedStoreError.EmptyResolution;
        }

        let i: usize = 0;
        while (i < _.entry_count) {
            const e = _.entries[i];
            if (e.kind == kind &&
                e.peer_hash == peer_hash &&
                bytesEql(e.name, name) &&
                bytesEql(e.resolution, resolution))
            {
                return e.id;
            }
            i += 1;
        }

        if (_.entry_count >= ISOLATED_STORE_MAX_ENTRIES) {
            return IsolatedStoreError.TooManyEntries;
        }

        const new_id = @as(u32, @intCast(_.entry_count));
        _.entries[_.entry_count] = IsolatedStoreEntry{
            id: new_id,
            kind: kind,
            name: name,
            resolution: resolution,
            hoisted: hoisted,
            peer_hash: peer_hash,
        };
        _.entry_count += 1;
        return new_id;
    }

    /// 为条目 `from_id` 添加有序去重依赖边 `to_id`（对齐 Bun `OrderedArraySet` 语义）
    fn addDependency(*_, from_id: u32, to_id: u32)! {
        const fi = @as(usize, from_id);
        const ti = @as(usize, to_id);
        if (fi >= _.entry_count || ti >= _.entry_count) {
            return IsolatedStoreError.InvalidEntryId;
        }

        let entry = &_.entries[fi];
        // 有序插入并去重
        let insert_pos: usize = entry.dep_count;
        let i: usize = 0;
        while (i < entry.dep_count) {
            if (entry.deps[i] == to_id) {
                return;
            }
            if (entry.deps[i] > to_id) {
                insert_pos = i;
                break;
            }
            i += 1;
        }

        if (entry.dep_count >= ISOLATED_STORE_MAX_DEPS_PER_ENTRY) {
            return IsolatedStoreError.TooManyDeps;
        }

        let shift = entry.dep_count;
        while (shift > insert_pos) {
            entry.deps[shift] = entry.deps[shift - 1];
            shift -= 1;
        }
        entry.deps[insert_pos] = to_id;
        entry.dep_count += 1;
    }

    /// 迭代计算全图所有 `Registry` 条目的传递依赖闭包 `entry_hash`（环路安全，非 Registry 条目保持 0）
    fn computeAllEntryHashes(*_)! {
        // 先为所有 Registry 条目初始化自身基础指纹
        let base_hashes: [ISOLATED_STORE_MAX_ENTRIES]u64 = [0; ISOLATED_STORE_MAX_ENTRIES];
        let key_buf: [256]u8 = [0; 256];

        let i: usize = 0;
        while (i < _.entry_count) {
            if (_.entries[i].supportsGlobalStore()) {
                const kn = formatIsolatedStoreKey(_.entries[i], key_buf[0..])?;
                base_hashes[i] = fnv1a64(key_buf[0..kn]);
                _.entries[i].entry_hash = base_hashes[i];
            } else {
                base_hashes[i] = 0;
                _.entries[i].entry_hash = 0;
            }
            i += 1;
        }

        // 迭代不动点传播传递依赖哈希（最多 entry_count 轮，环路安全且有界）
        let round: usize = 0;
        while (round < _.entry_count) {
            let changed = false;
            let next_hashes: [ISOLATED_STORE_MAX_ENTRIES]u64 = [0; ISOLATED_STORE_MAX_ENTRIES];
            let idx: usize = 0;
            while (idx < _.entry_count) {
                const e = _.entries[idx];
                if (!e.supportsGlobalStore()) {
                    next_hashes[idx] = 0;
                    idx += 1;
                    continue;
                }
                const kn = formatIsolatedStoreKey(e, key_buf[0..])?;
                let dep_h: [ISOLATED_STORE_MAX_DEPS_PER_ENTRY]u64 = [0; ISOLATED_STORE_MAX_DEPS_PER_ENTRY];
                let d: usize = 0;
                while (d < e.dep_count) {
                    dep_h[d] = _.entries[@as(usize, e.deps[d])].entry_hash;
                    d += 1;
                }
                const computed = computeGlobalEntryClosureHash(key_buf[0..kn], dep_h[0..e.dep_count]);
                next_hashes[idx] = computed;
                if (computed != e.entry_hash) {
                    changed = true;
                }
                idx += 1;
            }

            let k: usize = 0;
            while (k < _.entry_count) {
                _.entries[k].entry_hash = next_hashes[k];
                k += 1;
            }
            if (!changed) {
                break;
            }
            round += 1;
        }
    }
}

/// 将包名或相对路径编码为 Store 路径安全段（`/` 与 `\` 替换为 `+`，如 `@scope/pkg` -> `@scope+pkg`）
pub fn formatStorePathSegment(raw: []const u8, out: []u8): usize! {
    if (raw.len > out.len) {
        return IsolatedStoreError.BufferTooSmall;
    }
    let i: usize = 0;
    while (i < raw.len) {
        const c = raw[i];
        out[i] = if (c == '/' || c == '\\') '+' else c;
        i += 1;
    }
    return raw.len;
}

/// 格式化隔离存储唯一键（对齐 Bun `_StorePathFormatter`）：
/// - `Root`: `<name>@root`（无名时回退 `top_level_dir_name`）
/// - `Folder`: `<name>@file+<path>`
/// - 其他: `<name>@<resolution>`
/// - 若 `peer_hash != 0`，追加 `+<16位小写hex>`
pub fn formatIsolatedStoreKey(entry: IsolatedStoreEntry, out: []u8): usize! {
    let pos: usize = 0;
    switch (entry.kind) {
        .Root => {
            if (entry.name.len == 0) {
                if (entry.top_level_dir_name.len == 0) {
                    return IsolatedStoreError.EmptyPackageName;
                }
                pos = appendStoreSeg(out, pos, entry.top_level_dir_name)?;
            } else {
                pos = appendStoreSeg(out, pos, entry.name)?;
                pos = appendBytes(out, pos, "@root")?;
            }
        },
        .Folder => {
            if (entry.name.len == 0) {
                return IsolatedStoreError.EmptyPackageName;
            }
            if (entry.resolution.len == 0) {
                return IsolatedStoreError.EmptyResolution;
            }
            pos = appendStoreSeg(out, pos, entry.name)?;
            pos = appendBytes(out, pos, "@file+")?;
            pos = appendStoreSeg(out, pos, entry.resolution)?;
        },
        else => {
            if (entry.name.len == 0) {
                return IsolatedStoreError.EmptyPackageName;
            }
            if (entry.resolution.len == 0) {
                return IsolatedStoreError.EmptyResolution;
            }
            pos = appendStoreSeg(out, pos, entry.name)?;
            pos = appendByte(out, pos, '@')?;
            pos = appendStoreSeg(out, pos, entry.resolution)?;
        },
    }

    if (entry.peer_hash != 0) {
        pos = appendByte(out, pos, '+')?;
        pos = appendHex64Lower(out, pos, entry.peer_hash)?;
    }

    return pos;
}

/// 格式化完整的隔离存储包落盘目录：`node_modules/.bun/<store_key>/node_modules/<pkg_name>`
pub fn formatIsolatedStoreDir(entry: IsolatedStoreEntry, out: []u8): usize! {
    if (entry.name.len == 0) {
        return IsolatedStoreError.EmptyPackageName;
    }
    let pos: usize = 0;
    pos = appendBytes(out, pos, "node_modules/.bun/")?;
    const kn = formatIsolatedStoreKey(entry, out[pos..])?;
    pos += kn;
    pos = appendBytes(out, pos, "/node_modules/")?;
    pos = appendBytes(out, pos, entry.name)?;
    return pos;
}

/// 格式化顶层 `node_modules` 指向隔离存储的相对软链接目标：
/// - 非作用域包：`node_modules/<pkg>` -> `.bun/<store_key>/node_modules/<pkg>`
/// - 作用域包：`node_modules/@scope/<pkg>` -> `../.bun/<store_key>/node_modules/@scope/<pkg>`
pub fn formatIsolatedSymlinkTarget(entry: IsolatedStoreEntry, out: []u8): usize! {
    if (entry.name.len == 0) {
        return IsolatedStoreError.EmptyPackageName;
    }
    let pos: usize = 0;
    if (entry.isScoped()) {
        pos = appendBytes(out, pos, "../.bun/")?;
    } else {
        pos = appendBytes(out, pos, ".bun/")?;
    }
    const kn = formatIsolatedStoreKey(entry, out[pos..])?;
    pos += kn;
    pos = appendBytes(out, pos, "/node_modules/")?;
    pos = appendBytes(out, pos, entry.name)?;
    return pos;
}

/// 格式化 Global Virtual Store 目录名：`<store_key>-<16位小写hex>`（对齐 `_GlobalStorePathFormatter`）
pub fn formatGlobalStorePath(entry: IsolatedStoreEntry, out: []u8): usize! {
    if (!entry.supportsGlobalStore() || entry.entry_hash == 0) {
        return IsolatedStoreError.NotEligibleForGlobalStore;
    }
    let pos = formatIsolatedStoreKey(entry, out)?;
    pos = appendByte(out, pos, '-')?;
    pos = appendHex64Lower(out, pos, entry.entry_hash)?;
    return pos;
}

/// 计算无序确定性 Peer 依赖集合 64 位哈希（空集合返回 `0`，不同输入顺序产生相同哈希）
pub fn computePeerSetHash(peers: []const IsolatedPeerRef): u64! {
    if (peers.len == 0) {
        return 0;
    }
    if (peers.len > ISOLATED_STORE_MAX_PEERS) {
        return IsolatedStoreError.TooManyPeers;
    }

    // 为每条 peer 计算独立哈希，排序后折叠合并（保证顺序无关且零堆分配）
    let item_hashes: [ISOLATED_STORE_MAX_PEERS]u64 = [0; ISOLATED_STORE_MAX_PEERS];
    let i: usize = 0;
    while (i < peers.len) {
        let h: u64 = 0xcbf29ce484222325;
        h = fnv1a64Update(h, peers[i].name);
        h = fnv1a64UpdateByte(h, '@');
        h = fnv1a64Update(h, peers[i].version);
        item_hashes[i] = h;
        i += 1;
    }

    sortU64Slice(item_hashes[0..peers.len]);

    let final_h: u64 = 0xcbf29ce484222325;
    let k: usize = 0;
    while (k < peers.len) {
        if (k > 0 && item_hashes[k] == item_hashes[k - 1]) {
            // 重复 peer 去重
            k += 1;
            continue;
        }
        final_h = fnv1a64UpdateU64(final_h, item_hashes[k]);
        k += 1;
    }
    return if (final_h == 0) 1 else final_h;
}

/// 计算单条目的 Global Virtual Store 依赖闭包哈希（`store_key` + 排序后的依赖哈希列表，结果永不为 0）
pub fn computeGlobalEntryClosureHash(store_key: []const u8, dep_hashes: []const u64): u64 {
    let sorted: [ISOLATED_STORE_MAX_DEPS_PER_ENTRY]u64 = [0; ISOLATED_STORE_MAX_DEPS_PER_ENTRY];
    const n = if (dep_hashes.len < ISOLATED_STORE_MAX_DEPS_PER_ENTRY) dep_hashes.len else ISOLATED_STORE_MAX_DEPS_PER_ENTRY;
    let i: usize = 0;
    while (i < n) {
        sorted[i] = dep_hashes[i];
        i += 1;
    }
    sortU64Slice(sorted[0..n]);

    let h = fnv1a64(store_key);
    let k: usize = 0;
    while (k < n) {
        h = fnv1a64UpdateByte(h, '|');
        h = fnv1a64UpdateU64(h, sorted[k]);
        k += 1;
    }
    return if (h == 0) 1 else h;
}

// ============================================================================
// 内部零分配辅助函数
// ============================================================================

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

fn appendByte(out: []u8, pos: usize, b: u8): usize! {
    if (pos >= out.len) {
        return IsolatedStoreError.BufferTooSmall;
    }
    out[pos] = b;
    return pos + 1;
}

fn appendBytes(out: []u8, pos: usize, src: []const u8): usize! {
    if (src.len > out.len - pos) {
        return IsolatedStoreError.BufferTooSmall;
    }
    let i: usize = 0;
    while (i < src.len) {
        out[pos + i] = src[i];
        i += 1;
    }
    return pos + src.len;
}

fn appendStoreSeg(out: []u8, pos: usize, raw: []const u8): usize! {
    const n = formatStorePathSegment(raw, out[pos..])?;
    return pos + n;
}

fn appendHex64Lower(out: []u8, pos: usize, val: u64): usize! {
    if (16 > out.len - pos) {
        return IsolatedStoreError.BufferTooSmall;
    }
    const hex = "0123456789abcdef";
    let shift: u6 = 60;
    let i: usize = 0;
    while (i < 16) {
        const nibble = @as(usize, @intCast((val >> shift) & 0x0f));
        out[pos + i] = hex[nibble];
        if (shift >= 4) {
            shift -= 4;
        }
        i += 1;
    }
    return pos + 16;
}

fn fnv1a64UpdateByte(h: u64, b: u8): u64 {
    return (h ^ @as(u64, b)) *% 0x100000001b3;
}

fn fnv1a64Update(h_: u64, data: []const u8): u64 {
    let h = h_;
    let i: usize = 0;
    while (i < data.len) {
        h = fnv1a64UpdateByte(h, data[i]);
        i += 1;
    }
    return h;
}

fn fnv1a64UpdateU64(h_: u64, val: u64): u64 {
    let h = h_;
    let i: usize = 0;
    let v = val;
    while (i < 8) {
        h = fnv1a64UpdateByte(h, @as(u8, @truncate(v & 0xff)));
        v >>= 8;
        i += 1;
    }
    return h;
}

fn fnv1a64(data: []const u8): u64 {
    const h = fnv1a64Update(0xcbf29ce484222325, data);
    return if (h == 0) 1 else h;
}

fn sortU64Slice(slice: []u64) {
    let i: usize = 1;
    while (i < slice.len) {
        const key = slice[i];
        let j = i;
        while (j > 0 && slice[j - 1] > key) {
            slice[j] = slice[j - 1];
            j -= 1;
        }
        slice[j] = key;
        i += 1;
    }
}
