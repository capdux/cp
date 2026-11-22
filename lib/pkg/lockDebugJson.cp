// Source: Absorbed from /data/dev/bun/src/install/lockfile/lockfile_json_stringify_for_debugging.cp | Synced: 2026-11-22T14:22:00Z
// Pure-CP zero-allocation Bun Lockfile Debug JSON Serializer.
// Aligned with Bun `src/install/lockfile/lockfile_json_stringify_for_debugging.cp`
// and `docs/guides/install/git-diff-bun-lockfile.mdx`:
// - Serializes lockfile `format`, lowercase-hex `meta_hash`, `package_index` (single ID as
//   integer or multiple IDs as JSON array), `packages`, and `dependencies` into deterministic JSON.
// - Supports all Bun dependency version tags (`uninitialized`, `npm`, `dist_tag`, `tarball`,
//   `folder`, `symlink`, `workspace`, `git`, `github`, `catalog`), `is_alias`, `package_id`
//   (`null` when unresolved), and active `behavior` flag objects.
// - Performs RFC 8259 JSON string escaping without heap allocation.

/// 调试快照支持的最大包索引条目数
pub const LOCK_DEBUG_MAX_INDEX_ENTRIES: usize = 32;
/// 单个包名在 `package_index` 中支持的最大多版本 ID 数
pub const LOCK_DEBUG_MAX_IDS_PER_NAME: usize = 8;
/// 调试快照支持的最大依赖条目数
pub const LOCK_DEBUG_MAX_DEPS: usize = 64;
/// 调试快照支持的最大包记录数
pub const LOCK_DEBUG_MAX_PKGS: usize = 64;
/// 无效包 ID 哨兵值（序列化为 JSON `null`）
pub const LOCK_DEBUG_INVALID_PKG_ID: u32 = 0xFFFF_FFFF;

/// Lockfile 依赖行为标志位（对齐 Bun `Dependency.Behavior`）
pub const LOCK_BEH_NORMAL: u8 = 0x01;
pub const LOCK_BEH_OPTIONAL: u8 = 0x02;
pub const LOCK_BEH_DEV: u8 = 0x04;
pub const LOCK_BEH_PEER: u8 = 0x08;
pub const LOCK_BEH_WORKSPACE: u8 = 0x10;
pub const LOCK_BEH_BUNDLED: u8 = 0x20;

/// 调试 JSON 序列化错误集
pub err LockDebugJsonError {
    TooManyIndexEntries,
    TooManyIdsPerName,
    TooManyDependencies,
    TooManyPackages,
    EmptyPackageName,
    BufferTooSmall,
}

/// Lockfile 格式版本（对齐 Bun `Lockfile.FormatVersion`）
pub enum LockDebugFormat: u8 {
    V2 = 0,
    V3 = 1,
    TextV0 = 2,
    TextV1 = 3,

    fn asStr(): []const u8 {
        return switch (_) {
            .V2 => "v2",
            .V3 => "v3",
            .TextV0 => "text_v0",
            .TextV1 => "text_v1",
        };
    }
}

/// 依赖版本标签（对齐 Bun `Dependency.Version.Tag`）
pub enum LockDebugDepKind: u8 {
    Uninitialized = 0,
    Npm = 1,
    DistTag = 2,
    Tarball = 3,
    Folder = 4,
    Symlink = 5,
    Workspace = 6,
    Git = 7,
    Github = 8,
    Catalog = 9,

    fn asStr(): []const u8 {
        return switch (_) {
            .Uninitialized => "uninitialized",
            .Npm => "npm",
            .DistTag => "dist_tag",
            .Tarball => "tarball",
            .Folder => "folder",
            .Symlink => "symlink",
            .Workspace => "workspace",
            .Git => "git",
            .Github => "github",
            .Catalog => "catalog",
        };
    }
}

/// `package_index` 单条记录（1 个 ID 输出整数，多个 ID 输出 JSON 数组）
pub struct LockDebugIndexEntry {
    name: []const u8 = "",
    id_count: usize = 0,
    ids: [LOCK_DEBUG_MAX_IDS_PER_NAME]u32 = [0; LOCK_DEBUG_MAX_IDS_PER_NAME],
}

/// 依赖调试视图（覆盖全部 10 种 `Dependency.Version.Tag` 字段）
pub struct LockDebugDep {
    dep_id: u32 = 0,
    name: []const u8 = "",
    literal: []const u8 = "",
    kind: LockDebugDepKind = .Npm,
    is_alias: bool = false,
    resolved_pkg_id: u32 = LOCK_DEBUG_INVALID_PKG_ID,
    behavior_mask: u8 = LOCK_BEH_NORMAL,
    /// npm/dist_tag/tarball/git/github 通用子字段
    target_name: []const u8 = "",
    version_or_tag: []const u8 = "",
    uri_or_path: []const u8 = "",
    owner: []const u8 = "",
    repo: []const u8 = "",
    committish: []const u8 = "",
    resolved_url: []const u8 = "",
}

/// 包节点调试视图
pub struct LockDebugPkg {
    pkg_id: u32 = 0,
    name: []const u8 = "",
    version: []const u8 = "",
    integrity: []const u8 = "",
}

fn copyInto(dst: []u8, offset: usize, src: []const u8): usize! {
    if (offset + src.len > dst.len) return err.BufferTooSmall;
    for (i of 0..src.len) {
        dst[offset + i] = src[i];
    }
    return offset + src.len;
}

fn copyByte(dst: []u8, offset: usize, b: u8): usize! {
    if (offset + 1 > dst.len) return err.BufferTooSmall;
    dst[offset] = b;
    return offset + 1;
}

fn copyU32(dst: []u8, offset: usize, val: u32): usize! {
    let tmp: [16]u8 = [0; 16];
    let n: usize = 0;
    let v: u32 = val;
    if (v == 0) {
        return copyByte(dst, offset, '0');
    }
    while (v > 0) {
        tmp[n] = ((v % 10) + 48) as u8;
        v = v / 10;
        n += 1;
    }
    let pos = offset;
    while (n > 0) {
        n -= 1;
        pos = copyByte(dst, pos, tmp[n])?;
    }
    return pos;
}

fn copyJsonString(dst: []u8, offset: usize, s: []const u8): usize! {
    const hex = "0123456789abcdef";
    let pos = copyByte(dst, offset, '"')?;
    for (i of 0..s.len) {
        const c = s[i];
        if (c == '"') {
            pos = copyInto(dst, pos, "\\\"")?;
        } else if (c == '\\') {
            pos = copyInto(dst, pos, "\\\\")?;
        } else if (c == '\n') {
            pos = copyInto(dst, pos, "\\n")?;
        } else if (c == '\r') {
            pos = copyInto(dst, pos, "\\r")?;
        } else if (c == '\t') {
            pos = copyInto(dst, pos, "\\t")?;
        } else if (c < 0x20) {
            pos = copyInto(dst, pos, "\\u00")?;
            pos = copyByte(dst, pos, hex[((c >> 4) & 0x0F) as usize])?;
            pos = copyByte(dst, pos, hex[(c & 0x0F) as usize])?;
        } else {
            pos = copyByte(dst, pos, c)?;
        }
    }
    return copyByte(dst, pos, '"');
}

fn copyJsonField(dst: []u8, offset: usize, first: bool, key: []const u8): usize! {
    let pos = offset;
    if (!first) {
        pos = copyByte(dst, pos, ',')?;
    }
    pos = copyJsonString(dst, pos, key)?;
    return copyByte(dst, pos, ':');
}

/// 将任意字节序列编码为小写十六进制字符串（用于 `meta_hash`）
pub fn formatLowerHexBytes(bytes: []const u8, out: []u8): usize! {
    const hex = "0123456789abcdef";
    if (bytes.len * 2 > out.len) return err.BufferTooSmall;
    let pos: usize = 0;
    for (i of 0..bytes.len) {
        const b = bytes[i];
        out[pos] = hex[((b >> 4) & 0x0F) as usize];
        out[pos + 1] = hex[(b & 0x0F) as usize];
        pos += 2;
    }
    return pos;
}

/// 序列化单条 `LockDebugDep` 为完整 JSON 对象（对齐 Bun `jsonStringifyDependency`）
pub fn stringifyLockDebugDep(dep: LockDebugDep, out: []u8): usize! {
    let pos: usize = 0;
    pos = copyByte(out, pos, '{')?;

    pos = copyJsonField(out, pos, true, "name")?;
    pos = copyJsonString(out, pos, dep.name)?;

    if (dep.kind == .Npm && dep.is_alias) {
        pos = copyJsonField(out, pos, false, "is_alias")?;
        pos = copyInto(out, pos, "true")?;
    }

    pos = copyJsonField(out, pos, false, "literal")?;
    pos = copyJsonString(out, pos, dep.literal)?;

    pos = copyJsonField(out, pos, false, dep.kind.asStr())?;
    switch (dep.kind) {
        .Uninitialized => {
            pos = copyInto(out, pos, "null")?;
        },
        .Npm => {
            pos = copyByte(out, pos, '{')?;
            pos = copyJsonField(out, pos, true, "name")?;
            pos = copyJsonString(out, pos, if (dep.target_name.len > 0) dep.target_name else dep.name)?;
            pos = copyJsonField(out, pos, false, "version")?;
            pos = copyJsonString(out, pos, dep.version_or_tag)?;
            pos = copyByte(out, pos, '}')?;
        },
        .DistTag => {
            pos = copyByte(out, pos, '{')?;
            pos = copyJsonField(out, pos, true, "name")?;
            pos = copyJsonString(out, pos, if (dep.target_name.len > 0) dep.target_name else dep.name)?;
            pos = copyJsonField(out, pos, false, "tag")?;
            pos = copyJsonString(out, pos, dep.version_or_tag)?;
            pos = copyByte(out, pos, '}')?;
        },
        .Tarball => {
            pos = copyByte(out, pos, '{')?;
            pos = copyJsonField(out, pos, true, "uri")?;
            pos = copyJsonString(out, pos, dep.uri_or_path)?;
            pos = copyJsonField(out, pos, false, "package_name")?;
            pos = copyJsonString(out, pos, if (dep.target_name.len > 0) dep.target_name else dep.name)?;
            pos = copyByte(out, pos, '}')?;
        },
        .Folder => {
            pos = copyJsonString(out, pos, dep.uri_or_path)?;
        },
        .Symlink => {
            pos = copyJsonString(out, pos, dep.uri_or_path)?;
        },
        .Workspace => {
            pos = copyJsonString(out, pos, dep.uri_or_path)?;
        },
        .Git => {
            pos = copyByte(out, pos, '{')?;
            pos = copyJsonField(out, pos, true, "owner")?;
            pos = copyJsonString(out, pos, dep.owner)?;
            pos = copyJsonField(out, pos, false, "repo")?;
            pos = copyJsonString(out, pos, dep.repo)?;
            pos = copyJsonField(out, pos, false, "committish")?;
            pos = copyJsonString(out, pos, dep.committish)?;
            pos = copyJsonField(out, pos, false, "resolved")?;
            pos = copyJsonString(out, pos, dep.resolved_url)?;
            pos = copyJsonField(out, pos, false, "package_name")?;
            pos = copyJsonString(out, pos, if (dep.target_name.len > 0) dep.target_name else dep.name)?;
            pos = copyByte(out, pos, '}')?;
        },
        .Github => {
            pos = copyByte(out, pos, '{')?;
            pos = copyJsonField(out, pos, true, "owner")?;
            pos = copyJsonString(out, pos, dep.owner)?;
            pos = copyJsonField(out, pos, false, "repo")?;
            pos = copyJsonString(out, pos, dep.repo)?;
            pos = copyJsonField(out, pos, false, "committish")?;
            pos = copyJsonString(out, pos, dep.committish)?;
            pos = copyJsonField(out, pos, false, "resolved")?;
            pos = copyJsonString(out, pos, dep.resolved_url)?;
            pos = copyJsonField(out, pos, false, "package_name")?;
            pos = copyJsonString(out, pos, if (dep.target_name.len > 0) dep.target_name else dep.name)?;
            pos = copyByte(out, pos, '}')?;
        },
        .Catalog => {
            pos = copyByte(out, pos, '{')?;
            pos = copyJsonField(out, pos, true, "name")?;
            pos = copyJsonString(out, pos, dep.name)?;
            pos = copyJsonField(out, pos, false, "version")?;
            pos = copyByte(out, pos, '"')?;
            pos = copyInto(out, pos, "catalog:")?;
            pos = copyInto(out, pos, dep.version_or_tag)?;
            pos = copyByte(out, pos, '"')?;
            pos = copyByte(out, pos, '}')?;
        },
    }

    pos = copyJsonField(out, pos, false, "package_id")?;
    if (dep.resolved_pkg_id == LOCK_DEBUG_INVALID_PKG_ID) {
        pos = copyInto(out, pos, "null")?;
    } else {
        pos = copyU32(out, pos, dep.resolved_pkg_id)?;
    }

    pos = copyJsonField(out, pos, false, "behavior")?;
    pos = copyByte(out, pos, '{')?;
    let beh_first = true;
    if ((dep.behavior_mask & LOCK_BEH_NORMAL) != 0) {
        pos = copyJsonField(out, pos, beh_first, "normal")?;
        pos = copyInto(out, pos, "true")?;
        beh_first = false;
    }
    if ((dep.behavior_mask & LOCK_BEH_OPTIONAL) != 0) {
        pos = copyJsonField(out, pos, beh_first, "optional")?;
        pos = copyInto(out, pos, "true")?;
        beh_first = false;
    }
    if ((dep.behavior_mask & LOCK_BEH_DEV) != 0) {
        pos = copyJsonField(out, pos, beh_first, "dev")?;
        pos = copyInto(out, pos, "true")?;
        beh_first = false;
    }
    if ((dep.behavior_mask & LOCK_BEH_PEER) != 0) {
        pos = copyJsonField(out, pos, beh_first, "peer")?;
        pos = copyInto(out, pos, "true")?;
        beh_first = false;
    }
    if ((dep.behavior_mask & LOCK_BEH_WORKSPACE) != 0) {
        pos = copyJsonField(out, pos, beh_first, "workspace")?;
        pos = copyInto(out, pos, "true")?;
        beh_first = false;
    }
    if ((dep.behavior_mask & LOCK_BEH_BUNDLED) != 0) {
        pos = copyJsonField(out, pos, beh_first, "bundled")?;
        pos = copyInto(out, pos, "true")?;
    }
    pos = copyByte(out, pos, '}')?;

    pos = copyJsonField(out, pos, false, "id")?;
    pos = copyU32(out, pos, dep.dep_id)?;
    pos = copyByte(out, pos, '}')?;
    return pos;
}

/// 零堆分配 Bun Lockfile 调试 JSON 快照构建与序列化器
pub struct LockDebugSnapshot {
    format: LockDebugFormat = .TextV1,
    meta_hash: [32]u8 = [0; 32],
    meta_hash_len: usize = 0,

    index_entries: [LOCK_DEBUG_MAX_INDEX_ENTRIES]LockDebugIndexEntry = [LockDebugIndexEntry{}; LOCK_DEBUG_MAX_INDEX_ENTRIES],
    index_count: usize = 0,

    pkgs: [LOCK_DEBUG_MAX_PKGS]LockDebugPkg = [LockDebugPkg{}; LOCK_DEBUG_MAX_PKGS],
    pkg_count: usize = 0,

    deps: [LOCK_DEBUG_MAX_DEPS]LockDebugDep = [LockDebugDep{}; LOCK_DEBUG_MAX_DEPS],
    dep_count: usize = 0,

    static fn init(format: LockDebugFormat): LockDebugSnapshot {
        return LockDebugSnapshot{ format: format };
    }

    /// 设置原始 `meta_hash` 字节（最多 32 字节，序列化时自动转为小写十六进制）
    fn setMetaHash(*_, raw: []const u8) {
        const n = if (raw.len <= 32) raw.len else 32;
        for (i of 0..n) {
            _.meta_hash[i] = raw[i];
        }
        _.meta_hash_len = n;
    }

    /// 向 `package_index` 注册包名与对应的包 ID 列表
    fn addPackageIndex(*_, name: []const u8, ids: []const u32): usize! {
        if (name.len == 0) return err.EmptyPackageName;
        if (ids.len > LOCK_DEBUG_MAX_IDS_PER_NAME) return err.TooManyIdsPerName;
        if (_.index_count >= LOCK_DEBUG_MAX_INDEX_ENTRIES) return err.TooManyIndexEntries;

        const idx = _.index_count;
        _.index_entries[idx] = LockDebugIndexEntry{
            name: name,
            id_count: ids.len,
        };
        for (k of 0..ids.len) {
            _.index_entries[idx].ids[k] = ids[k];
        }
        _.index_count += 1;
        return idx;
    }

    /// 追加包节点记录
    fn addPackage(*_, pkg: LockDebugPkg): usize! {
        if (_.pkg_count >= LOCK_DEBUG_MAX_PKGS) return err.TooManyPackages;
        const idx = _.pkg_count;
        _.pkgs[idx] = pkg;
        _.pkg_count += 1;
        return idx;
    }

    /// 追加依赖记录
    fn addDependency(*_, dep: LockDebugDep): usize! {
        if (_.dep_count >= LOCK_DEBUG_MAX_DEPS) return err.TooManyDependencies;
        const idx = _.dep_count;
        _.deps[idx] = dep;
        _.dep_count += 1;
        return idx;
    }

    /// 将完整 Lockfile 调试快照序列化为紧凑合法 JSON（对齐 Bun `jsonStringify`）
    fn stringify(out: []u8): usize! {
        let pos: usize = 0;
        pos = copyByte(out, pos, '{')?;

        pos = copyJsonField(out, pos, true, "format")?;
        pos = copyJsonString(out, pos, _.format.asStr())?;

        pos = copyJsonField(out, pos, false, "meta_hash")?;
        let hex_buf: [64]u8 = [0; 64];
        const hex_len = formatLowerHexBytes(_.meta_hash[0.._.meta_hash_len], hex_buf[0..])?;
        pos = copyJsonString(out, pos, hex_buf[0..hex_len])?;

        // package_index
        pos = copyJsonField(out, pos, false, "package_index")?;
        pos = copyByte(out, pos, '{')?;
        for (i of 0.._.index_count) {
            const ent = _.index_entries[i];
            pos = copyJsonField(out, pos, i == 0, ent.name)?;
            if (ent.id_count == 1) {
                pos = copyU32(out, pos, ent.ids[0])?;
            } else {
                pos = copyByte(out, pos, '[')?;
                for (k of 0..ent.id_count) {
                    if (k > 0) pos = copyByte(out, pos, ',')?;
                    pos = copyU32(out, pos, ent.ids[k])?;
                }
                pos = copyByte(out, pos, ']')?;
            }
        }
        pos = copyByte(out, pos, '}')?;

        // packages
        pos = copyJsonField(out, pos, false, "packages")?;
        pos = copyByte(out, pos, '[')?;
        for (i of 0.._.pkg_count) {
            if (i > 0) pos = copyByte(out, pos, ',')?;
            const p = _.pkgs[i];
            pos = copyByte(out, pos, '{')?;
            pos = copyJsonField(out, pos, true, "id")?;
            pos = copyU32(out, pos, p.pkg_id)?;
            pos = copyJsonField(out, pos, false, "name")?;
            pos = copyJsonString(out, pos, p.name)?;
            pos = copyJsonField(out, pos, false, "version")?;
            pos = copyJsonString(out, pos, p.version)?;
            if (p.integrity.len > 0) {
                pos = copyJsonField(out, pos, false, "integrity")?;
                pos = copyJsonString(out, pos, p.integrity)?;
            }
            pos = copyByte(out, pos, '}')?;
        }
        pos = copyByte(out, pos, ']')?;

        // dependencies
        pos = copyJsonField(out, pos, false, "dependencies")?;
        pos = copyByte(out, pos, '[')?;
        for (i of 0.._.dep_count) {
            if (i > 0) pos = copyByte(out, pos, ',')?;
            const dep_len = stringifyLockDebugDep(_.deps[i], out[pos..])?;
            pos += dep_len;
        }
        pos = copyByte(out, pos, ']')?;

        pos = copyByte(out, pos, '}')?;
        return pos;
    }
}
