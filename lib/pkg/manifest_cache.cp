// Source: Absorbed from /data/dev/bun/src/install/PackageManifestMap.cp | Synced: 2026-11-22T10:49:46Z
// Ported from Bun: /data/dev/bun/src/install/PackageManifestMap.cp & /data/dev/bun/docs/pm/global-cache.mdx
// Synced: 2026-11-22
// Pure-CP zero-allocation npm PackageManifest memory/disk cache state machine:
// Fresh / Expired / NotFound negative-cache states, Cache-Control max-age validation,
// extended-manifest upgrade demotion, conditional ETag revalidation support,
// global cache `<name>@<version>` directory formatter (with pre-release/build suffix hashing),
// and `--backend` copy strategy parser.

/// 单实例内存清单缓存支持的最大包条目数
pub const MANIFEST_CACHE_MAX_ENTRIES: usize = 64;

/// 默认全局缓存相对子目录（对齐 `docs/pm/global-cache.mdx`）
pub const MANIFEST_DEFAULT_CACHE_SUBDIR: []const u8 = ".bun/install/cache";

/// 清单缓存与全局缓存路径错误集
pub err ManifestCacheError {
    EmptyPackageName,
    CacheFull,
    BufferTooSmall,
    UnsafeCacheName,
}

/// 缓存查找行为模式（对齐 Bun `PackageManifestMap.CacheBehavior`）
pub enum ManifestCacheBehavior: u8 {
    /// 仅查内存，不触发磁盘回退或扩展清单降级
    LoadFromMemory = 0,
    /// 先查内存，必要时降级为 Expired 或回退至磁盘缓存加载
    LoadFromMemoryFallbackToDisk = 1,
}

/// 清单缓存槽位状态（对齐 Bun `PackageManifestMap.Value`）
pub enum ManifestCacheState: u8 {
    Empty = 0,
    Fresh = 1,
    Expired = 2,
    /// 负缓存：避免对已确认不存在的磁盘缓存文件重复做文件系统探测
    NotFound = 3,
}

/// 单次缓存查找结果分类
pub enum ManifestLookupOutcome: u8 {
    HitFresh = 0,
    HitExpired = 1,
    MissNotFound = 2,
    MissNeedDiskOrNetwork = 3,
}

/// 全局缓存复制到 `node_modules` 的底层后端策略（对齐 `--backend` 文档）
pub enum InstallCopyBackend: u8 {
    Hardlink = 0,
    Clonefile = 1,
    ClonefileEachDir = 2,
    Copyfile = 3,
    Symlink = 4,
}

/// 单个包清单缓存记录
pub struct ManifestCacheRecord {
    name_hash: u64 = 0,
    package_name: []const u8 = "",
    etag: []const u8 = "",
    latest_version: []const u8 = "",
    public_max_age: i64 = 0,
    has_extended_manifest: bool = false,
    state: ManifestCacheState = ManifestCacheState.Empty,
}

/// 查找与磁盘加载返回视图
pub struct ManifestLookupResult {
    outcome: ManifestLookupOutcome = ManifestLookupOutcome.MissNeedDiskOrNetwork,
    slot_index: usize = 0,
    is_expired: bool = false,
    demoted_for_extended: bool = false,
    record: ManifestCacheRecord = ManifestCacheRecord{},
}

/// 清单缓存命中与降级统计
pub struct ManifestCacheStats {
    entry_count: usize = 0,
    fresh_hits: usize = 0,
    expired_hits: usize = 0,
    not_found_hits: usize = 0,
    extended_demotions: usize = 0,
    disk_ingests: usize = 0,
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

fn hexDigitLower(nibble: u8): u8 {
    if (nibble < 10) {
        return ('0' as u8) + nibble;
    }
    return ('a' as u8) + (nibble - 10);
}

fn appendBytes(out: []u8, pos: *usize, src: []const u8): void! {
    if (pos.* + src.len > out.len) {
        return ManifestCacheError.BufferTooSmall;
    }
    let i: usize = 0;
    while (i < src.len) {
        out[pos.* + i] = src[i];
        i += 1;
    }
    pos.* += src.len;
}

fn appendByte(out: []u8, pos: *usize, b: u8): void! {
    if (pos.* + 1 > out.len) {
        return ManifestCacheError.BufferTooSmall;
    }
    out[pos.*] = b;
    pos.* += 1;
}

/// 计算确定性 64 位 FNV-1a 包名哈希
pub fn hashPackageManifestName(name: []const u8): u64 {
    let h: u64 = 0xcbf29ce484222325;
    let i: usize = 0;
    while (i < name.len) {
        h ^= name[i] as u64;
        h *%= 0x100000001b3;
        i += 1;
    }
    return h;
}

/// 解析 `--backend` 安装复制策略字符串
pub fn parseInstallCopyBackend(raw: []const u8): InstallCopyBackend? {
    if (bytesEql(raw, "hardlink")) {
        return InstallCopyBackend.Hardlink;
    }
    if (bytesEql(raw, "clonefile")) {
        return InstallCopyBackend.Clonefile;
    }
    if (bytesEql(raw, "clonefile_each_dir")) {
        return InstallCopyBackend.ClonefileEachDir;
    }
    if (bytesEql(raw, "copyfile")) {
        return InstallCopyBackend.Copyfile;
    }
    if (bytesEql(raw, "symlink")) {
        return InstallCopyBackend.Symlink;
    }
    return null;
}

fn containsUnsafeSeq(s: []const u8): bool {
    let i: usize = 0;
    while (i < s.len) {
        const c = s[i];
        if (c == 0 || c == '\n' || c == '\r' || c == '\\') {
            return true;
        }
        if (c == '.' && i + 1 < s.len && s[i + 1] == '.') {
            return true;
        }
        i += 1;
    }
    return false;
}

/// 格式化全局缓存子目录名 `<name>@<version>`（对齐 `docs/pm/global-cache.mdx`）：
/// - 若 `version` 包含 pre-release（`-`）或 build（`+`）后缀，则将后缀替换为确定性 8 位十六进制哈希以防路径过长；
/// - 自动拦截 `..` 路径穿越与非法控制字符。
pub fn formatGlobalCacheEntryDir(package_name: []const u8, version: []const u8, out: []u8): usize! {
    if (package_name.len == 0 || version.len == 0) {
        return ManifestCacheError.EmptyPackageName;
    }
    if (containsUnsafeSeq(package_name) || containsUnsafeSeq(version)) {
        return ManifestCacheError.UnsafeCacheName;
    }

    let pos: usize = 0;
    appendBytes(out, &pos, package_name)?;
    appendByte(out, &pos, '@')?;

    // 查找首个 '-' 或 '+'
    let split_idx: usize? = null;
    let i: usize = 0;
    while (i < version.len) {
        if (version[i] == '-' || version[i] == '+') {
            split_idx = i;
            break;
        }
        i += 1;
    }

    if (idx := split_idx) {
        appendBytes(out, &pos, version[0..idx])?;
        appendByte(out, &pos, version[idx])?;
        const suffix_hash = hashPackageManifestName(version[idx + 1 ..]);
        // 输出高 32 位的 8 位小写十六进制哈希
        let shift: u6 = 60;
        let d: usize = 0;
        while (d < 8) {
            const nib = ((suffix_hash >> shift) & 0xF) as u8;
            appendByte(out, &pos, hexDigitLower(nib))?;
            shift -= 4;
            d += 1;
        }
    } else {
        appendBytes(out, &pos, version)?;
    }

    return pos;
}

/// 零堆分配 npm PackageManifest 内存/磁盘缓存状态机（对齐 Bun `PackageManifestMap`）
pub struct PackageManifestCache {
    entries: [MANIFEST_CACHE_MAX_ENTRIES]ManifestCacheRecord = [ManifestCacheRecord{}; MANIFEST_CACHE_MAX_ENTRIES],
    count: usize = 0,
    enable_manifest_cache: bool = true,
    enable_cache_control: bool = true,
    stats: ManifestCacheStats = ManifestCacheStats{},

    static fn init(enable_manifest_cache: bool, enable_cache_control: bool): PackageManifestCache {
        return PackageManifestCache{
            enable_manifest_cache: enable_manifest_cache,
            enable_cache_control: enable_cache_control,
        };
    }

    fn findSlot(name_hash: u64, package_name: []const u8): usize? {
        let i: usize = 0;
        while (i < _.count) {
            if (_.entries[i].name_hash == name_hash && bytesEql(_.entries[i].package_name, package_name)) {
                return i;
            }
            i += 1;
        }
        return null;
    }

    fn getOrAllocateSlot(*_, name_hash: u64, package_name: []const u8): usize! {
        if (existing := _.findSlot(name_hash, package_name)) {
            return existing;
        }
        if (_.count >= MANIFEST_CACHE_MAX_ENTRIES) {
            return ManifestCacheError.CacheFull;
        }
        const idx = _.count;
        _.count += 1;
        _.stats.entry_count = _.count;
        return idx;
    }

    /// 写入一条新鲜（Fresh）清单缓存记录（若同名包已存在则原地更新）
    fn insertFresh(
        *_,
        package_name: []const u8,
        etag: []const u8,
        latest_version: []const u8,
        public_max_age: i64,
        has_extended_manifest: bool,
    ): usize! {
        if (package_name.len == 0) {
            return ManifestCacheError.EmptyPackageName;
        }
        const h = hashPackageManifestName(package_name);
        const idx = _.getOrAllocateSlot(h, package_name)?;
        _.entries[idx] = ManifestCacheRecord{
            name_hash: h,
            package_name: package_name,
            etag: etag,
            latest_version: latest_version,
            public_max_age: public_max_age,
            has_extended_manifest: has_extended_manifest,
            state: ManifestCacheState.Fresh,
        };
        return idx;
    }

    /// 写入负缓存（NotFound），避免重复探测磁盘缓存文件
    fn markNotFound(*_, package_name: []const u8): usize! {
        if (package_name.len == 0) {
            return ManifestCacheError.EmptyPackageName;
        }
        const h = hashPackageManifestName(package_name);
        const idx = _.getOrAllocateSlot(h, package_name)?;
        _.entries[idx] = ManifestCacheRecord{
            name_hash: h,
            package_name: package_name,
            state: ManifestCacheState.NotFound,
        };
        return idx;
    }

    /// 按 Bun `byNameHashAllowExpired` 语义执行内存查找与扩展清单降级判定
    fn lookup(
        *_,
        package_name: []const u8,
        behavior: ManifestCacheBehavior,
        allow_expired: bool,
        needs_extended_manifest: bool,
    ): ManifestLookupResult {
        if (package_name.len == 0) {
            return ManifestLookupResult{ outcome: ManifestLookupOutcome.MissNotFound };
        }
        const h = hashPackageManifestName(package_name);
        const slot_opt = _.findSlot(h, package_name);
        if (slot_opt == null) {
            return ManifestLookupResult{ outcome: ManifestLookupOutcome.MissNeedDiskOrNetwork };
        }
        const idx = slot_opt.?;

        if (behavior == ManifestCacheBehavior.LoadFromMemory) {
            switch (_.entries[idx].state) {
                ManifestCacheState.Fresh => {
                    _.stats.fresh_hits += 1;
                    return ManifestLookupResult{
                        outcome: ManifestLookupOutcome.HitFresh,
                        slot_index: idx,
                        record: _.entries[idx],
                    };
                },
                ManifestCacheState.Expired => {
                    if (allow_expired) {
                        _.stats.expired_hits += 1;
                        return ManifestLookupResult{
                            outcome: ManifestLookupOutcome.HitExpired,
                            slot_index: idx,
                            is_expired: true,
                            record: _.entries[idx],
                        };
                    }
                    return ManifestLookupResult{ outcome: ManifestLookupOutcome.MissNeedDiskOrNetwork, slot_index: idx };
                },
                ManifestCacheState.NotFound => {
                    _.stats.not_found_hits += 1;
                    return ManifestLookupResult{ outcome: ManifestLookupOutcome.MissNotFound, slot_index: idx };
                },
                ManifestCacheState.Empty => {
                    return ManifestLookupResult{ outcome: ManifestLookupOutcome.MissNeedDiskOrNetwork, slot_index: idx };
                },
            }
        }

        // LoadFromMemoryFallbackToDisk
        let demoted = false;
        if (_.entries[idx].state == ManifestCacheState.Fresh) {
            if (needs_extended_manifest && !_.entries[idx].has_extended_manifest) {
                _.entries[idx].state = ManifestCacheState.Expired;
                _.stats.extended_demotions += 1;
                demoted = true;
            } else {
                _.stats.fresh_hits += 1;
                return ManifestLookupResult{
                    outcome: ManifestLookupOutcome.HitFresh,
                    slot_index: idx,
                    record: _.entries[idx],
                };
            }
        }

        if (_.entries[idx].state == ManifestCacheState.Expired) {
            if (allow_expired) {
                _.stats.expired_hits += 1;
                return ManifestLookupResult{
                    outcome: ManifestLookupOutcome.HitExpired,
                    slot_index: idx,
                    is_expired: true,
                    demoted_for_extended: demoted,
                    record: _.entries[idx],
                };
            }
            return ManifestLookupResult{
                outcome: ManifestLookupOutcome.MissNeedDiskOrNetwork,
                slot_index: idx,
                is_expired: true,
                demoted_for_extended: demoted,
            };
        }

        if (_.entries[idx].state == ManifestCacheState.NotFound) {
            _.stats.not_found_hits += 1;
            return ManifestLookupResult{ outcome: ManifestLookupOutcome.MissNotFound, slot_index: idx };
        }
        return ManifestLookupResult{ outcome: ManifestLookupOutcome.MissNeedDiskOrNetwork, slot_index: idx };
    }

    /// 吸收从磁盘缓存反序列化的包清单：按 `needs_extended_manifest` 与 `Cache-Control` 决定归类为 `Fresh` 或 `Expired`
    fn ingestDiskManifest(
        *_,
        package_name: []const u8,
        etag: []const u8,
        latest_version: []const u8,
        public_max_age: i64,
        has_extended_manifest: bool,
        now_sec: i64,
        allow_expired: bool,
        needs_extended_manifest: bool,
    ): ManifestLookupResult! {
        if (package_name.len == 0) {
            return ManifestCacheError.EmptyPackageName;
        }
        if (!_.enable_manifest_cache) {
            return ManifestLookupResult{ outcome: ManifestLookupOutcome.MissNeedDiskOrNetwork };
        }
        const h = hashPackageManifestName(package_name);
        const idx = _.getOrAllocateSlot(h, package_name)?;
        _.stats.disk_ingests += 1;

        let is_fresh = false;
        let demoted = false;
        if (needs_extended_manifest && !has_extended_manifest) {
            _.stats.extended_demotions += 1;
            demoted = true;
            is_fresh = false;
        } else if (_.enable_cache_control && public_max_age > now_sec) {
            is_fresh = true;
        }

        _.entries[idx] = ManifestCacheRecord{
            name_hash: h,
            package_name: package_name,
            etag: etag,
            latest_version: latest_version,
            public_max_age: public_max_age,
            has_extended_manifest: has_extended_manifest,
            state: if (is_fresh) ManifestCacheState.Fresh else ManifestCacheState.Expired,
        };

        if (is_fresh) {
            _.stats.fresh_hits += 1;
            return ManifestLookupResult{
                outcome: ManifestLookupOutcome.HitFresh,
                slot_index: idx,
                record: _.entries[idx],
            };
        }

        if (allow_expired) {
            _.stats.expired_hits += 1;
            return ManifestLookupResult{
                outcome: ManifestLookupOutcome.HitExpired,
                slot_index: idx,
                is_expired: true,
                demoted_for_extended: demoted,
                record: _.entries[idx],
            };
        }

        return ManifestLookupResult{
            outcome: ManifestLookupOutcome.MissNeedDiskOrNetwork,
            slot_index: idx,
            is_expired: true,
            demoted_for_extended: demoted,
        };
    }
}
