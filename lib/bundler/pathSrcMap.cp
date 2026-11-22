// Source: Absorbed from /data/dev/bun/src/bundler/PathToSourceIndexMap.cp | Synced: 2026-11-22T12:16:00Z
// Pure-CP zero-allocation open-addressing `PathToSourceIndexMap` with plugin namespace
// isolation (`file` vs virtual namespaces like `virtual:` / `env:`), FNV-1a 64-bit
// hashing, tombstone-safe removal, atomic `getOrPut` / `getOrPutPath`, and reverse
// `source_index -> BundlerModulePath` lookup.

/// 开放寻址哈希表固定槽位数（2 的幂次）
pub const PATH_SRC_MAP_BUCKETS: usize = 256;

/// 最大活跃条目数（75% 负载因子上限，保证 O(1) 平均探测长度）
pub const PATH_SRC_MAP_MAX_LOAD: usize = 192;

/// 无效 SourceIndex 哨兵值
pub const PATH_SRC_INVALID_INDEX: u32 = 0xFFFF_FFFF;

/// `PathToSourceIndexMap` 操作错误集
pub err PathSourceMapError {
    EmptyPath,
    MapFull,
    InvalidNamespace,
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

fn fnv1aUpdate(h: u64, data: []const u8): u64 {
    let acc = h;
    let i: usize = 0;
    while (i < data.len) {
        acc ^= @as(u64, data[i]);
        acc = acc *% 1099511628211;
        i += 1;
    }
    return acc;
}

fn writeByte(out: []u8, pos: *usize, b: u8): usize! {
    if (pos.* >= out.len) {
        return err.BufferTooSmall;
    }
    out[pos.*] = b;
    pos.* += 1;
    return 1;
}

fn writeSlice(out: []u8, pos: *usize, s: []const u8): usize! {
    if (s.len > out.len - pos.*) {
        return err.BufferTooSmall;
    }
    let i: usize = 0;
    while (i < s.len) {
        out[pos.* + i] = s[i];
        i += 1;
    }
    pos.* += s.len;
    return s.len;
}

/// Bundler 模块路径键（对齐 Bun `Fs.Path` 与插件 `namespace` 隔离规范）
pub struct BundlerModulePath {
    namespace: []const u8 = "file",
    text: []const u8 = "",

    /// 构造标准文件系统路径（`namespace = "file"`）
    static fn file(text: []const u8): BundlerModulePath {
        return BundlerModulePath{
            namespace: "file",
            text: text,
        };
    }

    /// 构造带自定义插件命名空间的模块路径（如 `namespace = "virtual"`, `text = "config.json"`）
    static fn withNamespace(ns: []const u8, text: []const u8): BundlerModulePath {
        return BundlerModulePath{
            namespace: if (ns.len == 0) "file" else ns,
            text: text,
        };
    }

    fn isFileNamespace(): bool {
        return _.namespace.len == 0 || bytesEql(_.namespace, "file");
    }

    fn isVirtual(): bool {
        return !_.isFileNamespace();
    }

    fn normalizedNamespace(): []const u8 {
        if (_.namespace.len == 0) {
            return "file";
        }
        return _.namespace;
    }

    fn eql(other: BundlerModulePath): bool {
        return bytesEql(_.normalizedNamespace(), other.normalizedNamespace()) &&
            bytesEql(_.text, other.text);
    }

    /// 计算 `namespace + '\0' + text` 的 64 位 FNV-1a 哈希，隔离同名不同命名空间模块
    fn hash64(): u64 {
        let h: u64 = 14695981039346656037;
        h = fnv1aUpdate(h, _.normalizedNamespace());
        h ^= 0xff;
        h = h *% 1099511628211;
        h = fnv1aUpdate(h, _.text);
        return h;
    }

    /// 格式化展示路径（`"file"` 输出原路径，虚拟命名空间输出 `"<ns>:<text>"`）
    fn formatInto(out: []u8): usize! {
        let pos: usize = 0;
        if (_.isVirtual()) {
            * = writeSlice(out, *pos, _.namespace)?;
            * = writeByte(out, *pos, ':')?;
        }
        * = writeSlice(out, *pos, _.text)?;
        return pos;
    }
}

/// `getOrPut` / `getOrPutPath` 返回结果
pub struct GetOrPutSourceResult {
    source_index: u32 = PATH_SRC_INVALID_INDEX,
    found_existing: bool = false,
}

enum BucketState: u8 {
    Empty = 0,
    Occupied = 1,
    Tombstone = 2,
}

struct PathBucket {
    state: BucketState = .Empty,
    hash: u64 = 0,
    key: BundlerModulePath = BundlerModulePath{},
    value: u32 = PATH_SRC_INVALID_INDEX,
}

/// 零堆分配 Bundler 路径 → `SourceIndex` 开放寻址映射表（对齐 Bun `PathToSourceIndexMap`）
pub struct PathToSourceIndexMap {
    buckets: [PATH_SRC_MAP_BUCKETS]PathBucket = [PathBucket{}; PATH_SRC_MAP_BUCKETS],
    len: usize = 0,
    tombstones: usize = 0,

    static fn init(): PathToSourceIndexMap {
        return PathToSourceIndexMap{};
    }

    fn count(): usize {
        return _.len;
    }

    fn isEmpty(): bool {
        return _.len == 0;
    }

    fn findSlot(path: BundlerModulePath, hash: u64): usize? {
        const mask: usize = PATH_SRC_MAP_BUCKETS - 1;
        let idx: usize = @intCast(hash & @as(u64, mask));
        let steps: usize = 0;
        while (steps < PATH_SRC_MAP_BUCKETS) {
            const b = _.buckets[idx];
            if (b.state == .Empty) {
                return null;
            }
            if (b.state == .Occupied && b.hash == hash && b.key.eql(path)) {
                return idx;
            }
            idx = (idx + 1) & mask;
            steps += 1;
        }
        return null;
    }

    fn get(text: []const u8): u32? {
        return _.getPath(BundlerModulePath.file(text));
    }

    fn getPath(path: BundlerModulePath): u32? {
        if (path.text.len == 0) {
            return null;
        }
        const h = path.hash64();
        if (slot := _.findSlot(path, h)) {
            return _.buckets[slot].value;
        }
        return null;
    }

    fn contains(text: []const u8): bool {
        return _.get(text) != null;
    }

    fn containsPath(path: BundlerModulePath): bool {
        return _.getPath(path) != null;
    }

    /// 插入或覆写默认 `"file"` 命名空间路径映射，返回是否覆写了已有键
    fn put(*_, text: []const u8, value: u32): bool! {
        return _.putPath(BundlerModulePath.file(text), value);
    }

    /// 插入或覆写带命名空间的模块路径映射，返回是否覆写了已有键
    fn putPath(*_, path: BundlerModulePath, value: u32): bool! {
        if (path.text.len == 0) {
            return err.EmptyPath;
        }
        const h = path.hash64();
        const mask: usize = PATH_SRC_MAP_BUCKETS - 1;
        let idx: usize = @intCast(h & @as(u64, mask));
        let first_tombstone: usize? = null;
        let steps: usize = 0;

        while (steps < PATH_SRC_MAP_BUCKETS) {
            const st = _.buckets[idx].state;
            if (st == .Empty) {
                const target = first_tombstone ?? idx;
                if (_.len >= PATH_SRC_MAP_MAX_LOAD) {
                    return err.MapFull;
                }
                if (_.buckets[target].state == .Tombstone && _.tombstones > 0) {
                    _.tombstones -= 1;
                }
                _.buckets[target] = PathBucket{
                    state: .Occupied,
                    hash: h,
                    key: BundlerModulePath{
                        namespace: path.normalizedNamespace(),
                        text: path.text,
                    },
                    value: value,
                };
                _.len += 1;
                return false;
            }
            if (st == .Occupied && _.buckets[idx].hash == h && _.buckets[idx].key.eql(path)) {
                _.buckets[idx].value = value;
                return true;
            }
            if (st == .Tombstone && first_tombstone == null) {
                first_tombstone = idx;
            }
            idx = (idx + 1) & mask;
            steps += 1;
        }

        if (target := first_tombstone) {
            if (_.len >= PATH_SRC_MAP_MAX_LOAD) {
                return err.MapFull;
            }
            if (_.tombstones > 0) {
                _.tombstones -= 1;
            }
            _.buckets[target] = PathBucket{
                state: .Occupied,
                hash: h,
                key: BundlerModulePath{
                    namespace: path.normalizedNamespace(),
                    text: path.text,
                },
                value: value,
            };
            _.len += 1;
            return false;
        }
        return err.MapFull;
    }

    fn getOrPut(*_, text: []const u8, next_index: u32): GetOrPutSourceResult! {
        return _.getOrPutPath(BundlerModulePath.file(text), next_index);
    }

    /// 原子语义：若 `path` 已存在返回既有 `source_index` (`found_existing: true`)，否则写入 `next_index`
    fn getOrPutPath(*_, path: BundlerModulePath, next_index: u32): GetOrPutSourceResult! {
        if (path.text.len == 0) {
            return err.EmptyPath;
        }
        if (existing := _.getPath(path)) {
            return GetOrPutSourceResult{
                source_index: existing,
                found_existing: true,
            };
        }
        * = _.putPath(path, next_index)?;
        return GetOrPutSourceResult{
            source_index: next_index,
            found_existing: false,
        };
    }

    fn remove(*_, text: []const u8): bool {
        return _.removePath(BundlerModulePath.file(text));
    }

    /// 墓碑安全删除：删除后不断裂开放寻址探测链，返回是否命中并删除
    fn removePath(*_, path: BundlerModulePath): bool {
        if (path.text.len == 0) {
            return false;
        }
        const h = path.hash64();
        if (slot := _.findSlot(path, h)) {
            _.buckets[slot].state = .Tombstone;
            _.buckets[slot].value = PATH_SRC_INVALID_INDEX;
            if (_.len > 0) {
                _.len -= 1;
            }
            _.tombstones += 1;
            return true;
        }
        return false;
    }

    /// 按 `source_index` 反查对应模块路径（用于诊断日志与 sourcemap 定位）
    fn reverseLookup(source_index: u32): BundlerModulePath? {
        let i: usize = 0;
        while (i < PATH_SRC_MAP_BUCKETS) {
            if (_.buckets[i].state == .Occupied && _.buckets[i].value == source_index) {
                return _.buckets[i].key;
            }
            i += 1;
        }
        return null;
    }

    fn clear(*_) {
        let i: usize = 0;
        while (i < PATH_SRC_MAP_BUCKETS) {
            _.buckets[i] = PathBucket{};
            i += 1;
        }
        _.len = 0;
        _.tombstones = 0;
    }
}
