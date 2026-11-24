// Source: Absorbed from /data/dev/bun/src/install/pnpm.cp | Synced: 2026-11-23T20:24:05Z
// Pure-CP zero-allocation pnpm-lock.yaml (v7/v8/v9) migration parser & index:
// balanced-parenthesis dependency path suffix parser (patch_hash + nested peers),
// package key & importer version normalizer (scoped, npm: alias, link:, file:, workspace:, git, tarball),
// and fixed-capacity snapshot/package/patch/importer integrity verifier.

/// 最低受支持的 pnpm-lock.yaml 主版本号（对齐 Bun `lockfile_version_num < 7 => PnpmLockfileTooOld`）
pub const PNPM_LOCK_MIN_SUPPORTED_MAJOR: u32 = 7;

/// 单个迁移索引支持的最大 `patchedDependencies` 条目数
pub const PNPM_LOCK_MAX_PATCHES: usize = 64;

/// 单个迁移索引支持的最大 `snapshots` 条目数
pub const PNPM_LOCK_MAX_SNAPSHOTS: usize = 128;

/// 单个迁移索引支持的最大 `importers` 工作区条目数
pub const PNPM_LOCK_MAX_IMPORTERS: usize = 64;

/// pnpm-lock.yaml 解析与迁移索引错误集
pub err PnpmLockError {
    InvalidLockfileVersion,
    LockfileTooOld,
    InvalidDepPath,
    InvalidPatchHash,
    InvalidPackageKey,
    DuplicatePatchHash,
    DuplicateImporterPath,
    MissingRootImporter,
    MissingSnapshot,
    TableFull,
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

fn startsWith(s: []const u8, prefix: []const u8): bool {
    if (prefix.len > s.len) {
        return false;
    }
    let i: usize = 0;
    while (i < prefix.len) {
        if (s[i] != prefix[i]) {
            return false;
        }
        i += 1;
    }
    return true;
}

fn indexOfByte(s: []const u8, target: u8): usize? {
    let i: usize = 0;
    while (i < s.len) {
        if (s[i] == target) {
            return i;
        }
        i += 1;
    }
    return null;
}

fn lastIndexOfByte(s: []const u8, target: u8): usize? {
    let i = s.len;
    while (i > 0) {
        i -= 1;
        if (s[i] == target) {
            return i;
        }
    }
    return null;
}

fn hasControlChar(s: []const u8): bool {
    let i: usize = 0;
    while (i < s.len) {
        const c = s[i];
        if (c < 0x20 || c == 0x7f) {
            return true;
        }
        i += 1;
    }
    return false;
}

fn trimSpaces(s: []const u8): []const u8 {
    let start: usize = 0;
    while (start < s.len && (s[start] == ' ' || s[start] == '\t' || s[start] == '\r' || s[start] == '\n')) {
        start += 1;
    }
    let end = s.len;
    while (end > start && (s[end - 1] == ' ' || s[end - 1] == '\t' || s[end - 1] == '\r' || s[end - 1] == '\n')) {
        end -= 1;
    }
    return s[start..end];
}

fn stripMatchingQuotes(s: []const u8): []const u8 {
    if (s.len >= 2) {
        const first = s[0];
        const last = s[s.len - 1];
        if ((first == '\'' && last == '\'') || (first == '"' && last == '"')) {
            return s[1 .. s.len - 1];
        }
    }
    return s;
}

fn parseU32Decimal(s: []const u8): u32! {
    if (s.len == 0) {
        return err.InvalidLockfileVersion;
    }
    let acc: u64 = 0;
    let i: usize = 0;
    while (i < s.len) {
        const c = s[i];
        if (c < '0' || c > '9') {
            return err.InvalidLockfileVersion;
        }
        acc = acc * 10 + @as(u64, c - '0');
        if (acc > 0xFFFF_FFFF) {
            return err.InvalidLockfileVersion;
        }
        i += 1;
    }
    return @intCast(acc);
}

/// pnpm-lock.yaml 的 `lockfileVersion` 结构化版本（如 `9.0` -> `{ major: 9, minor: 0 }`）
pub struct PnpmLockVersion {
    major: u32 = 9,
    minor: u32 = 0,

    /// 是否采用 pnpm v9+ 格式（去除 package key 前导 `/` 且拆分 `packages` 与 `snapshots`）
    fn isV9OrNewer(): bool {
        return _.major >= 9;
    }

    fn eql(other: PnpmLockVersion): bool {
        return _.major == other.major && _.minor == other.minor;
    }
}

/// 解析并校验 pnpm `lockfileVersion`（支持 `'9.0'`、`"9.0"`、`9.0`、`7.0`、`8.0`、`9` 等）
/// 低于 v7 返回 `err.LockfileTooOld`（对齐 Bun `migratePnpmLockfile`）。
pub fn parsePnpmLockfileVersion(raw: []const u8): PnpmLockVersion! {
    const trimmed = stripMatchingQuotes(trimSpaces(raw));
    if (trimmed.len == 0 || trimmed[0] == '-') {
        return err.InvalidLockfileVersion;
    }

    let major: u32 = 0;
    let minor: u32 = 0;
    if (dot_idx := indexOfByte(trimmed, '.')) {
        major = parseU32Decimal(trimmed[0..dot_idx])?;
        const rest = trimmed[dot_idx + 1 ..];
        if (rest.len == 0 || indexOfByte(rest, '.') != null) {
            return err.InvalidLockfileVersion;
        }
        minor = parseU32Decimal(rest)?;
    } else {
        major = parseU32Decimal(trimmed)?;
    }

    if (major < PNPM_LOCK_MIN_SUPPORTED_MAJOR) {
        return err.LockfileTooOld;
    }
    return PnpmLockVersion{ major: major, minor: minor };
}

/// pnpm 依赖路径尾部后缀定位索引（对齐 Bun `DepPathSuffix`）
pub struct PnpmDepPathSuffix {
    peers_idx: usize? = null,
    patch_hash_idx: usize? = null,

    fn hasSuffix(): bool {
        return _.patch_hash_idx != null || _.peers_idx != null;
    }

    fn suffixStart(): usize? {
        return _.patch_hash_idx ?? _.peers_idx;
    }
}

/// 逆向平衡括号扫描，定位 pnpm 依赖路径的 `(patch_hash=...)` 与 `(peer@ver)...` 起始索引。
/// 完整兼容多层嵌套 peer 依赖，如 `react-dom@18.2.0(patch_hash=abc)(react@18.2.0(scheduler@0.23.0))`。
pub fn indexOfPnpmDepPathSuffix(path: []const u8): PnpmDepPathSuffix {
    if (path.len < 2) {
        return PnpmDepPathSuffix{};
    }
    if (path[path.len - 1] != ')') {
        return PnpmDepPathSuffix{};
    }

    let open: i64 = 1;
    let i = path.len - 1;
    while (i > 0) {
        i -= 1;

        if (path[i] == '(') {
            open -= 1;
        } else if (path[i] == ')') {
            open += 1;
        } else if (open == 0) {
            if (startsWith(path[i + 1 ..], "(patch_hash=")) {
                let peers_idx: usize? = null;
                if (rel := indexOfByte(path[i + 2 ..], '(')) {
                    peers_idx = rel + i + 2;
                }
                return PnpmDepPathSuffix{
                    peers_idx: peers_idx,
                    patch_hash_idx: i + 1,
                };
            }
            return PnpmDepPathSuffix{
                peers_idx: i + 1,
                patch_hash_idx: null,
            };
        }
    }
    return PnpmDepPathSuffix{};
}

/// 剥离 pnpm 依赖路径尾部的 `(patch_hash=...)` 与 `(peers...)` 后缀：
/// - `name@version(peer@1.0.0)` -> `name@version`
/// - `1.2.3(patch_hash=abc)(react@18.2.0)` -> `1.2.3`
pub fn removePnpmDepPathSuffix(path: []const u8): []const u8 {
    const s = indexOfPnpmDepPathSuffix(path);
    if (idx := s.suffixStart()) {
        return path[0..idx];
    }
    return path;
}

/// 从 pnpm 依赖路径中提取 `(patch_hash=<hash>)` 里的 `<hash>` 值；
/// 无 patch_hash 后缀时返回 `null`，格式残缺或空 hash 返回 `err.InvalidPatchHash`。
pub fn extractPnpmPatchHash(path: []const u8): ([]const u8)?! {
    const s = indexOfPnpmDepPathSuffix(path);
    const idx = s.patch_hash_idx ?? return null;
    const prefix = "(patch_hash=";
    if (idx + prefix.len > path.len) {
        return err.InvalidPatchHash;
    }
    const rest = path[idx + prefix.len ..];
    const close_idx = indexOfByte(rest, ')') ?? return err.InvalidPatchHash;
    if (close_idx == 0) {
        return err.InvalidPatchHash;
    }
    const hash = rest[0..close_idx];
    if (hasControlChar(hash)) {
        return err.InvalidPatchHash;
    }
    return hash;
}

/// 从 pnpm 依赖路径中提取 peer 依赖后缀切片（如 `(react@18.2.0)`）
pub fn extractPnpmPeersSuffix(path: []const u8): ([]const u8)? {
    const s = indexOfPnpmDepPathSuffix(path);
    const idx = s.peers_idx ?? return null;
    if (idx >= path.len) {
        return null;
    }
    return path[idx..];
}

/// 解析后的 pnpm `packages` / `snapshots` 键元数据
pub struct PnpmPackageKey {
    name: []const u8 = "",
    version: []const u8 = "",
    has_leading_slash: bool = false,
    patch_hash: ([]const u8)? = null,
    peers: ([]const u8)? = null,
}

/// 解析 pnpm package/snapshot 键（兼容 v9 `pkg@ver`、`@scope/pkg@ver(peer@1.0.0)` 与 v7/v8 `/pkg@ver`）
pub fn parsePnpmPackageKey(raw_key: []const u8): PnpmPackageKey! {
    if (raw_key.len == 0 || hasControlChar(raw_key)) {
        return err.InvalidPackageKey;
    }

    let has_leading_slash = false;
    let body = raw_key;
    if (body[0] == '/') {
        has_leading_slash = true;
        body = body[1..];
    }

    const patch_hash = extractPnpmPatchHash(body)?;
    const peers = extractPnpmPeersSuffix(body);
    const base = removePnpmDepPathSuffix(body);

    if (base.len < 3) {
        return err.InvalidPackageKey;
    }

    let search_from: usize = 0;
    if (base[0] == '@') {
        const slash_idx = indexOfByte(base, '/') ?? return err.InvalidPackageKey;
        if (slash_idx <= 1 || slash_idx + 1 >= base.len) {
            return err.InvalidPackageKey;
        }
        search_from = slash_idx + 1;
    }

    const rel_at = indexOfByte(base[search_from..], '@') ?? return err.InvalidPackageKey;
    const at_idx = search_from + rel_at;
    if (at_idx == 0 || at_idx + 1 >= base.len) {
        return err.InvalidPackageKey;
    }

    const name = base[0..at_idx];
    const version = base[at_idx + 1 ..];
    if (name.len == 0 || version.len == 0) {
        return err.InvalidPackageKey;
    }

    return PnpmPackageKey{
        name: name,
        version: version,
        has_leading_slash: has_leading_slash,
        patch_hash: patch_hash,
        peers: peers,
    };
}

/// pnpm importer 中单条依赖版本字符串的解析协议类型
pub enum PnpmDepResolutionKind {
    Registry,
    Workspace,
    Link,
    File,
    Tarball,
    Git,
    Alias,
}

/// 解析后的 pnpm importer 依赖解析目标（对齐 Bun `migratePnpmLockfile` 根/工作区依赖解析）
pub struct PnpmImporterDepSpec {
    kind: PnpmDepResolutionKind = PnpmDepResolutionKind.Registry,
    package_name: []const u8 = "",
    resolved_version: []const u8 = "",
    target_path: []const u8 = "",
    patch_hash: ([]const u8)? = null,
    peers: ([]const u8)? = null,
}

/// 解析 pnpm `importers.<path>.dependencies.<dep_name>.version` 字段。
/// 支持 `npm:alias@1.2.3(peer@1.0.0)`、`link:../pkg`、`file:../a.tgz`、`workspace:*`、
/// `https://...`、`git+...` 以及标准 registry 版本 `1.2.3(patch_hash=abc)(peer@1.0.0)`。
pub fn parsePnpmImporterVersion(dep_name: []const u8, raw_version: []const u8): PnpmImporterDepSpec! {
    if (dep_name.len == 0 || raw_version.len == 0 || hasControlChar(dep_name) || hasControlChar(raw_version)) {
        return err.InvalidDepPath;
    }

    const patch_hash = extractPnpmPatchHash(raw_version)?;
    const peers = extractPnpmPeersSuffix(raw_version);
    const clean = removePnpmDepPathSuffix(raw_version);
    if (clean.len == 0) {
        return err.InvalidDepPath;
    }

    if (startsWith(clean, "link:")) {
        const target = clean[5..];
        if (target.len == 0) {
            return err.InvalidDepPath;
        }
        return PnpmImporterDepSpec{
            kind: PnpmDepResolutionKind.Link,
            package_name: dep_name,
            resolved_version: clean,
            target_path: target,
            patch_hash: patch_hash,
            peers: peers,
        };
    }

    if (startsWith(clean, "file:")) {
        const target = clean[5..];
        if (target.len == 0) {
            return err.InvalidDepPath;
        }
        return PnpmImporterDepSpec{
            kind: PnpmDepResolutionKind.File,
            package_name: dep_name,
            resolved_version: clean,
            target_path: target,
            patch_hash: patch_hash,
            peers: peers,
        };
    }

    if (startsWith(clean, "workspace:")) {
        const target = clean[10..];
        if (target.len == 0) {
            return err.InvalidDepPath;
        }
        return PnpmImporterDepSpec{
            kind: PnpmDepResolutionKind.Workspace,
            package_name: dep_name,
            resolved_version: target,
            target_path: target,
            patch_hash: patch_hash,
            peers: peers,
        };
    }

    if (startsWith(clean, "https://") || startsWith(clean, "http://")) {
        return PnpmImporterDepSpec{
            kind: PnpmDepResolutionKind.Tarball,
            package_name: dep_name,
            resolved_version: clean,
            target_path: clean,
            patch_hash: patch_hash,
            peers: peers,
        };
    }

    if (startsWith(clean, "git+") || startsWith(clean, "github:")) {
        return PnpmImporterDepSpec{
            kind: PnpmDepResolutionKind.Git,
            package_name: dep_name,
            resolved_version: clean,
            target_path: clean,
            patch_hash: patch_hash,
            peers: peers,
        };
    }

    // 处理 npm alias：`npm:real-pkg@1.2.3` 或 `@scope/real-pkg@1.2.3`
    let maybe_alias = clean;
    let explicit_npm = false;
    if (startsWith(maybe_alias, "npm:")) {
        explicit_npm = true;
        maybe_alias = maybe_alias[4..];
        if (maybe_alias.len == 0) {
            return err.InvalidDepPath;
        }
    }

    if (explicit_npm || maybe_alias[0] == '@' || indexOfByte(maybe_alias, '@') != null) {
        let search_from: usize = 0;
        if (maybe_alias[0] == '@') {
            const slash = indexOfByte(maybe_alias, '/') ?? return err.InvalidDepPath;
            if (slash <= 1 || slash + 1 >= maybe_alias.len) {
                return err.InvalidDepPath;
            }
            search_from = slash + 1;
        }
        if (rel_at := indexOfByte(maybe_alias[search_from..], '@')) {
            const at_idx = search_from + rel_at;
            if (at_idx == 0 || at_idx + 1 >= maybe_alias.len) {
                return err.InvalidDepPath;
            }
            return PnpmImporterDepSpec{
                kind: PnpmDepResolutionKind.Alias,
                package_name: maybe_alias[0..at_idx],
                resolved_version: maybe_alias[at_idx + 1 ..],
                target_path: "",
                patch_hash: patch_hash,
                peers: peers,
            };
        } else if (explicit_npm) {
            return err.InvalidDepPath;
        }
    }

    return PnpmImporterDepSpec{
        kind: PnpmDepResolutionKind.Registry,
        package_name: dep_name,
        resolved_version: clean,
        target_path: "",
        patch_hash: patch_hash,
        peers: peers,
    };
}

/// 将 `name` 与 `version` 格式化为规范化锁文件键 `name@version` 写入调用方提供的固定缓冲区
pub fn formatPnpmCanonicalKey(name: []const u8, version: []const u8, out: []u8): usize! {
    if (name.len == 0 || version.len == 0) {
        return err.InvalidPackageKey;
    }
    const total = name.len + 1 + version.len;
    if (total > out.len) {
        return err.BufferTooSmall;
    }
    let pos: usize = 0;
    let i: usize = 0;
    while (i < name.len) {
        out[pos] = name[i];
        pos += 1;
        i += 1;
    }
    out[pos] = '@';
    pos += 1;
    i = 0;
    while (i < version.len) {
        out[pos] = version[i];
        pos += 1;
        i += 1;
    }
    return pos;
}

/// 已注册的 `patchedDependencies` 条目
pub struct PnpmPatchEntry {
    dep_name: []const u8 = "",
    patch_path: []const u8 = "",
    hash: []const u8 = "",
    matched_version: ([]const u8)? = null,
    matched: bool = false,
}

/// 已注册的 `snapshots` 条目（按去后缀 `canonical_key` 幂等去重）
pub struct PnpmSnapshotRecord {
    canonical_key: []const u8 = "",
    raw_key: []const u8 = "",
    patch_hash: ([]const u8)? = null,
    has_peers: bool = false,
    variant_count: u32 = 0,
    matched_package: bool = false,
}

/// 已注册的 `importers` 条目
pub struct PnpmImporterRecord {
    path: []const u8 = "",
    is_root: bool = false,
    dep_count: usize = 0,
}

/// 迁移索引完整性汇总统计
pub struct PnpmMigrationStats {
    lockfile_version: PnpmLockVersion = PnpmLockVersion{},
    importers_count: usize = 0,
    snapshots_count: usize = 0,
    packages_matched: usize = 0,
    patches_registered: usize = 0,
    patches_matched: usize = 0,
    has_root_importer: bool = false,
}

/// 零堆分配 pnpm-lock.yaml 迁移完整性索引器（对齐 Bun `migratePnpmLockfile` 的
/// `patches` / `importers` / `snapshots` / `packages` 交叉校验状态机）
pub struct PnpmMigrationIndex {
    version: PnpmLockVersion = PnpmLockVersion{},
    patches: [PNPM_LOCK_MAX_PATCHES]PnpmPatchEntry = [PnpmPatchEntry{}; PNPM_LOCK_MAX_PATCHES],
    patch_len: usize = 0,
    snapshots: [PNPM_LOCK_MAX_SNAPSHOTS]PnpmSnapshotRecord = [PnpmSnapshotRecord{}; PNPM_LOCK_MAX_SNAPSHOTS],
    snapshot_len: usize = 0,
    importers: [PNPM_LOCK_MAX_IMPORTERS]PnpmImporterRecord = [PnpmImporterRecord{}; PNPM_LOCK_MAX_IMPORTERS],
    importer_len: usize = 0,
    has_root_importer: bool = false,

    static fn init(ver: PnpmLockVersion): PnpmMigrationIndex {
        return PnpmMigrationIndex{ version: ver };
    }

    /// 注册 `patchedDependencies` 条目（按 `hash` 唯一去重，重复返回 `err.DuplicatePatchHash`）
    fn registerPatch(*_, dep_name: []const u8, patch_path: []const u8, hash: []const u8): usize! {
        if (dep_name.len == 0 || patch_path.len == 0 || hash.len == 0) {
            return err.InvalidPatchHash;
        }
        let i: usize = 0;
        while (i < _.patch_len) {
            if (bytesEql(_.patches[i].hash, hash)) {
                return err.DuplicatePatchHash;
            }
            i += 1;
        }
        if (_.patch_len >= PNPM_LOCK_MAX_PATCHES) {
            return err.TableFull;
        }
        const idx = _.patch_len;
        _.patches[idx] = PnpmPatchEntry{
            dep_name: dep_name,
            patch_path: patch_path,
            hash: hash,
            matched_version: null,
            matched: false,
        };
        _.patch_len += 1;
        return idx;
    }

    /// 注册 `importers` 条目（`"."` 标记为根包，重复路径返回 `err.DuplicateImporterPath`）
    fn registerImporter(*_, importer_path: []const u8, dep_count: usize): usize! {
        if (importer_path.len == 0 || hasControlChar(importer_path)) {
            return err.InvalidDepPath;
        }
        let i: usize = 0;
        while (i < _.importer_len) {
            if (bytesEql(_.importers[i].path, importer_path)) {
                return err.DuplicateImporterPath;
            }
            i += 1;
        }
        if (_.importer_len >= PNPM_LOCK_MAX_IMPORTERS) {
            return err.TableFull;
        }
        const is_root = bytesEql(importer_path, ".");
        const idx = _.importer_len;
        _.importers[idx] = PnpmImporterRecord{
            path: importer_path,
            is_root: is_root,
            dep_count: dep_count,
        };
        _.importer_len += 1;
        if (is_root) {
            _.has_root_importer = true;
        }
        return idx;
    }

    /// 注册 `snapshots` 条目：
    /// 1. 剥离后缀得到 `canonical_key`（如 `react-dom@18.2.0`）；
    /// 2. 若带 `(patch_hash=...)`，自动匹配已注册的 `patches` 并记录其实际解析版本；
    /// 3. 同一 `canonical_key` 的多 peer 变体幂等合并并递增 `variant_count`。
    fn registerSnapshot(*_, raw_key: []const u8): usize! {
        const parsed = parsePnpmPackageKey(raw_key)?;
        const base_without_slash = if (parsed.has_leading_slash) raw_key[1..] else raw_key;
        const canonical = removePnpmDepPathSuffix(base_without_slash);

        if (ph := parsed.patch_hash) {
            let pi: usize = 0;
            while (pi < _.patch_len) {
                if (bytesEql(_.patches[pi].hash, ph) && !_.patches[pi].matched) {
                    _.patches[pi].matched = true;
                    _.patches[pi].matched_version = parsed.version;
                    break;
                }
                pi += 1;
            }
        }

        let si: usize = 0;
        while (si < _.snapshot_len) {
            if (bytesEql(_.snapshots[si].canonical_key, canonical)) {
                _.snapshots[si].variant_count += 1;
                return si;
            }
            si += 1;
        }

        if (_.snapshot_len >= PNPM_LOCK_MAX_SNAPSHOTS) {
            return err.TableFull;
        }
        const idx = _.snapshot_len;
        _.snapshots[idx] = PnpmSnapshotRecord{
            canonical_key: canonical,
            raw_key: raw_key,
            patch_hash: parsed.patch_hash,
            has_peers: parsed.peers != null,
            variant_count: 1,
            matched_package: false,
        };
        _.snapshot_len += 1;
        return idx;
    }

    /// 将 `packages` 节的键与已注册的 `snapshots` 交叉匹配；缺失对应 snapshot 返回 `err.MissingSnapshot`
    fn matchPackage(*_, raw_package_key: []const u8): usize! {
        const parsed = parsePnpmPackageKey(raw_package_key)?;
        const base_without_slash = if (parsed.has_leading_slash) raw_package_key[1..] else raw_package_key;
        const canonical = removePnpmDepPathSuffix(base_without_slash);

        let si: usize = 0;
        while (si < _.snapshot_len) {
            if (bytesEql(_.snapshots[si].canonical_key, canonical)) {
                _.snapshots[si].matched_package = true;
                return si;
            }
            si += 1;
        }
        return err.MissingSnapshot;
    }

    /// 校验根 importer 存在性，返回当前迁移索引统计摘要
    fn validateAndStats(): PnpmMigrationStats! {
        if (!_.has_root_importer) {
            return err.MissingRootImporter;
        }
        return _.stats();
    }

    fn stats(): PnpmMigrationStats {
        let pkg_matched: usize = 0;
        let si: usize = 0;
        while (si < _.snapshot_len) {
            if (_.snapshots[si].matched_package) {
                pkg_matched += 1;
            }
            si += 1;
        }

        let patch_matched: usize = 0;
        let pi: usize = 0;
        while (pi < _.patch_len) {
            if (_.patches[pi].matched) {
                patch_matched += 1;
            }
            pi += 1;
        }

        return PnpmMigrationStats{
            lockfile_version: _.version,
            importers_count: _.importer_len,
            snapshots_count: _.snapshot_len,
            packages_matched: pkg_matched,
            patches_registered: _.patch_len,
            patches_matched: patch_matched,
            has_root_importer: _.has_root_importer,
        };
    }

    fn patchAt(idx: usize): PnpmPatchEntry? {
        if (idx >= _.patch_len) {
            return null;
        }
        return _.patches[idx];
    }

    fn snapshotAt(idx: usize): PnpmSnapshotRecord? {
        if (idx >= _.snapshot_len) {
            return null;
        }
        return _.snapshots[idx];
    }
}
