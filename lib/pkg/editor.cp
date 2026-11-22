// Source: Absorbed from /data/dev/bun/src/install/PackageManager/PackageJSONEditor.cp | Synced: 2026-11-22T12:36:10Z
// Pure-CP zero-allocation package.json dependency, trustedDependencies & patchedDependencies editor:
// Bun dependency group precedence (optional > dev > prod > peer), version pin preservation (~, ^, exact, npm: alias),
// cross-group deduplication/migration, alphabetized trustedDependencies, and deterministic JSON formatting.

/// 单个依赖分组支持的最大依赖条目数
pub const PKG_JSON_MAX_DEPS_PER_GROUP: usize = 64;

/// `trustedDependencies` 支持的最大受信任包数量
pub const PKG_JSON_MAX_TRUSTED_DEPS: usize = 64;

/// `patchedDependencies` 支持的最大补丁映射数量
pub const PKG_JSON_MAX_PATCHED_DEPS: usize = 64;

/// `package.json` 编辑与序列化错误集
pub err PkgJsonEditError {
    InvalidPackageName,
    InvalidVersion,
    InvalidPatchKey,
    TableFull,
    BufferTooSmall,
    NotFound,
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

fn bytesLessThan(a: []const u8, b: []const u8): bool {
    const min_len = if (a.len < b.len) a.len else b.len;
    let i: usize = 0;
    while (i < min_len) {
        if (a[i] != b[i]) {
            return a[i] < b[i];
        }
        i += 1;
    }
    return a.len < b.len;
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
        if (s[i] < 0x20 || s[i] == 0x7f) {
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

fn writeByte(out: []u8, pos: *usize, b: u8): bool! {
    if (pos >= out.len) {
        return err.BufferTooSmall;
    }
    out[pos] = b;
    pos += 1;
    return true;
}

fn writeSlice(out: []u8, pos: *usize, s: []const u8): bool! {
    if (pos + s.len > out.len) {
        return err.BufferTooSmall;
    }
    let i: usize = 0;
    while (i < s.len) {
        out[pos] = s[i];
        pos += 1;
        i += 1;
    }
    return true;
}

fn writeJsonString(out: []u8, pos: *usize, s: []const u8): bool! {
    * = writeByte(out, *pos, '"')?;
    let i: usize = 0;
    while (i < s.len) {
        const c = s[i];
        if (c == '"' || c == '\\') {
            * = writeByte(out, *pos, '\\')?;
            * = writeByte(out, *pos, c)?;
        } else {
            * = writeByte(out, *pos, c)?;
        }
        i += 1;
    }
    * = writeByte(out, *pos, '"')?;
    return true;
}

/// `package.json` 依赖分组（顺序严格对齐 Bun `dependency_groups` 解析优先级：
/// `optionalDependencies` > `devDependencies` > `dependencies` > `peerDependencies`）
pub enum PkgJsonDepGroup {
    Optional,
    Dev,
    Prod,
    Peer,
}

/// 返回依赖分组在 `package.json` 中对应的字段名
pub fn pkgJsonDepGroupFieldName(group: PkgJsonDepGroup): []const u8 {
    switch (group) {
        PkgJsonDepGroup.Optional => return "optionalDependencies",
        PkgJsonDepGroup.Dev => return "devDependencies",
        PkgJsonDepGroup.Prod => return "dependencies",
        PkgJsonDepGroup.Peer => return "peerDependencies",
    }
}

/// 将 `package.json` 字段名解析为 `PkgJsonDepGroup`
pub fn parsePkgJsonDepGroup(field: []const u8): PkgJsonDepGroup? {
    if (bytesEql(field, "optionalDependencies")) return PkgJsonDepGroup.Optional;
    if (bytesEql(field, "devDependencies")) return PkgJsonDepGroup.Dev;
    if (bytesEql(field, "dependencies")) return PkgJsonDepGroup.Prod;
    if (bytesEql(field, "peerDependencies")) return PkgJsonDepGroup.Peer;
    return null;
}

/// 依赖版本固定粒度（对齐 Bun `Semver.Version.whichVersionIsPinned`）
pub enum PkgJsonVersionPin {
    /// 精确版本（如 `1.2.3`，无 `^`/`~`）
    Patch,
    /// 次版本范围（如 `~1.2.3`）
    Minor,
    /// 主版本范围（如 `^1.2.3` 或默认新装）
    Major,
}

/// 检测原版本字面量的固定策略；自动兼容 `npm:@scope/pkg@~1.2.3` 别名格式
pub fn detectPkgJsonVersionPin(spec: []const u8): PkgJsonVersionPin {
    const trimmed = trimSpaces(spec);
    if (trimmed.len == 0) {
        return PkgJsonVersionPin.Major;
    }

    let ver = trimmed;
    if (startsWith(ver, "npm:")) {
        if (at_idx := lastIndexOfByte(ver, '@')) {
            if (at_idx + 1 < ver.len) {
                ver = ver[at_idx + 1 ..];
            }
        }
    }

    if (ver.len == 0) {
        return PkgJsonVersionPin.Major;
    }
    if (ver[0] == '~') {
        return PkgJsonVersionPin.Minor;
    }
    if (ver[0] == '^') {
        return PkgJsonVersionPin.Major;
    }
    if (ver[0] >= '0' && ver[0] <= '9') {
        return PkgJsonVersionPin.Patch;
    }
    return PkgJsonVersionPin.Major;
}

/// 按 Bun `PackageJSONEditor` 规则格式化更新后的版本字面量：
/// - `exact_versions = true`（`--exact` / `-E`）：始终输出精确版本号 `1.2.3`（若原为 `npm:alias@...` 则保留别名前缀）；
/// - `exact_versions = false`：沿用原 `original_spec` 的固定策略（`Patch` -> `1.2.3`、`Minor` -> `~1.2.3`、`Major` -> `^1.2.3`）。
pub fn formatEditedVersionSpec(
    original_spec: []const u8,
    resolved_version: []const u8,
    exact_versions: bool,
    out: []u8,
): usize! {
    const clean_ver = trimSpaces(resolved_version);
    if (clean_ver.len == 0 || hasControlChar(clean_ver)) {
        return err.InvalidVersion;
    }

    let pos: usize = 0;
    const trimmed_orig = trimSpaces(original_spec);

    // 若原字面量为 `npm:<pkg>@<ver>`，保留 `npm:<pkg>@` 前缀
    if (startsWith(trimmed_orig, "npm:")) {
        if (at_idx := lastIndexOfByte(trimmed_orig, '@')) {
            if (at_idx > 4) {
                * = writeSlice(out, *pos, trimmed_orig[0 .. at_idx + 1])?;
            }
        }
    }

    if (!exact_versions) {
        const pin = detectPkgJsonVersionPin(trimmed_orig);
        switch (pin) {
            PkgJsonVersionPin.Patch => {},
            PkgJsonVersionPin.Minor => {
                * = writeByte(out, *pos, '~')?;
            },
            PkgJsonVersionPin.Major => {
                * = writeByte(out, *pos, '^')?;
            },
        }
    }

    * = writeSlice(out, *pos, clean_ver)?;
    return pos;
}

/// `package.json` 编辑选项（对齐 Bun `PackageJSONEditor.EditOptions`）
pub struct PkgJsonEditOptions {
    exact_versions: bool = false,
    add_trusted_dependencies: bool = false,
    only_missing: bool = false,
    move_between_groups: bool = true,
}

/// 单条依赖键值项
pub struct PkgJsonDepEntry {
    name: []const u8 = "",
    version: []const u8 = "",
}

/// 单条 `patchedDependencies` 键值项
pub struct PkgJsonPatchEntry {
    patch_key: []const u8 = "",
    patchfile_path: []const u8 = "",
}

/// 编辑器当前状态统计
pub struct PkgJsonEditStats {
    prod_count: usize = 0,
    dev_count: usize = 0,
    optional_count: usize = 0,
    peer_count: usize = 0,
    trusted_count: usize = 0,
    patched_count: usize = 0,
    total_edits: usize = 0,
}

/// 固定容量有序依赖表（内部辅助容器）
struct PkgDepTable {
    items: [PKG_JSON_MAX_DEPS_PER_GROUP]PkgJsonDepEntry = [PkgJsonDepEntry{}; PKG_JSON_MAX_DEPS_PER_GROUP],
    len: usize = 0,

    fn find(name: []const u8): usize? {
        let i: usize = 0;
        while (i < _.len) {
            if (bytesEql(_.items[i].name, name)) {
                return i;
            }
            i += 1;
        }
        return null;
    }

    fn get(name: []const u8): ([]const u8)? {
        const idx = _.find(name) ?? return null;
        return _.items[idx].version;
    }

    fn upsert(*_, name: []const u8, version: []const u8): bool! {
        if (idx := _.find(name)) {
            _.items[idx].version = version;
            return true;
        }
        if (_.len >= PKG_JSON_MAX_DEPS_PER_GROUP) {
            return err.TableFull;
        }
        // 按 ASCII 字典序插入保持确定性输出
        let insert_at = _.len;
        while (insert_at > 0 && bytesLessThan(name, _.items[insert_at - 1].name)) {
            _.items[insert_at] = _.items[insert_at - 1];
            insert_at -= 1;
        }
        _.items[insert_at] = PkgJsonDepEntry{ name: name, version: version };
        _.len += 1;
        return true;
    }

    fn remove(*_, name: []const u8): bool {
        const idx = _.find(name) ?? return false;
        let i = idx;
        while (i + 1 < _.len) {
            _.items[i] = _.items[i + 1];
            i += 1;
        }
        _.len -= 1;
        _.items[_.len] = PkgJsonDepEntry{};
        return true;
    }
}

/// 零堆分配 `package.json` 依赖/信任列表/补丁编辑器（对齐 Bun `PackageJSONEditor`）
pub struct PkgJsonEditor {
    optional_deps: PkgDepTable = PkgDepTable{},
    dev_deps: PkgDepTable = PkgDepTable{},
    prod_deps: PkgDepTable = PkgDepTable{},
    peer_deps: PkgDepTable = PkgDepTable{},
    trusted: [PKG_JSON_MAX_TRUSTED_DEPS][]const u8 = [""; PKG_JSON_MAX_TRUSTED_DEPS],
    trusted_len: usize = 0,
    patched: [PKG_JSON_MAX_PATCHED_DEPS]PkgJsonPatchEntry = [PkgJsonPatchEntry{}; PKG_JSON_MAX_PATCHED_DEPS],
    patched_len: usize = 0,
    total_edits: usize = 0,

    static fn init(): PkgJsonEditor {
        return PkgJsonEditor{};
    }

    /// 按 Bun 依赖优先级（`Optional -> Dev -> Prod -> Peer`）查找包所在的最高优先级分组
    fn findDepGroup(name: []const u8): PkgJsonDepGroup? {
        if (_.optional_deps.find(name) != null) return PkgJsonDepGroup.Optional;
        if (_.dev_deps.find(name) != null) return PkgJsonDepGroup.Dev;
        if (_.prod_deps.find(name) != null) return PkgJsonDepGroup.Prod;
        if (_.peer_deps.find(name) != null) return PkgJsonDepGroup.Peer;
        return null;
    }

    fn getDepVersion(group: PkgJsonDepGroup, name: []const u8): ([]const u8)? {
        switch (group) {
            PkgJsonDepGroup.Optional => return _.optional_deps.get(name),
            PkgJsonDepGroup.Dev => return _.dev_deps.get(name),
            PkgJsonDepGroup.Prod => return _.prod_deps.get(name),
            PkgJsonDepGroup.Peer => return _.peer_deps.get(name),
        }
    }

    /// 新增或更新指定分组的依赖：
    /// - `opts.only_missing = true` 且任意分组已存在同名包时跳过并返回 `false`；
    /// - `opts.move_between_groups = true` 时自动从其他分组移除同名旧条目，避免跨组冲突；
    /// - `opts.add_trusted_dependencies = true` 时同步将包名加入 `trustedDependencies`。
    fn upsertDep(
        *_,
        group: PkgJsonDepGroup,
        name: []const u8,
        version: []const u8,
        opts: PkgJsonEditOptions,
    ): bool! {
        if (name.len == 0 || hasControlChar(name)) {
            return err.InvalidPackageName;
        }
        if (version.len == 0 || hasControlChar(version)) {
            return err.InvalidVersion;
        }

        if (opts.only_missing && _.findDepGroup(name) != null) {
            return false;
        }

        if (opts.move_between_groups) {
            if (group != PkgJsonDepGroup.Optional) * = _.optional_deps.remove(name);
            if (group != PkgJsonDepGroup.Dev) * = _.dev_deps.remove(name);
            if (group != PkgJsonDepGroup.Prod) * = _.prod_deps.remove(name);
            if (group != PkgJsonDepGroup.Peer) * = _.peer_deps.remove(name);
        }

        switch (group) {
            PkgJsonDepGroup.Optional => {
                * = _.optional_deps.upsert(name, version)?;
            },
            PkgJsonDepGroup.Dev => {
                * = _.dev_deps.upsert(name, version)?;
            },
            PkgJsonDepGroup.Prod => {
                * = _.prod_deps.upsert(name, version)?;
            },
            PkgJsonDepGroup.Peer => {
                * = _.peer_deps.upsert(name, version)?;
            },
        }

        if (opts.add_trusted_dependencies) {
            * = _.addTrustedDependency(name)?;
        }

        _.total_edits += 1;
        return true;
    }

    /// 从所有依赖分组中移除指定包名，返回实际移除的条目数
    fn removeDep(*_, name: []const u8): usize {
        let removed: usize = 0;
        if (_.optional_deps.remove(name)) removed += 1;
        if (_.dev_deps.remove(name)) removed += 1;
        if (_.prod_deps.remove(name)) removed += 1;
        if (_.peer_deps.remove(name)) removed += 1;
        if (removed > 0) {
            _.total_edits += 1;
        }
        return removed;
    }

    /// 幂等向 `trustedDependencies` 添加包名并保持 ASCII 字母序（对齐 Bun `alphabetizeStrings`）
    fn addTrustedDependency(*_, name: []const u8): bool! {
        if (name.len == 0 || hasControlChar(name)) {
            return err.InvalidPackageName;
        }
        let i: usize = 0;
        while (i < _.trusted_len) {
            if (bytesEql(_.trusted[i], name)) {
                return false;
            }
            i += 1;
        }
        if (_.trusted_len >= PKG_JSON_MAX_TRUSTED_DEPS) {
            return err.TableFull;
        }
        let insert_at = _.trusted_len;
        while (insert_at > 0 && bytesLessThan(name, _.trusted[insert_at - 1])) {
            _.trusted[insert_at] = _.trusted[insert_at - 1];
            insert_at -= 1;
        }
        _.trusted[insert_at] = name;
        _.trusted_len += 1;
        _.total_edits += 1;
        return true;
    }

    fn isTrustedDependency(name: []const u8): bool {
        let i: usize = 0;
        while (i < _.trusted_len) {
            if (bytesEql(_.trusted[i], name)) {
                return true;
            }
            i += 1;
        }
        return false;
    }

    fn trustedAt(idx: usize): ([]const u8)? {
        if (idx >= _.trusted_len) {
            return null;
        }
        return _.trusted[idx];
    }

    /// 设置或更新 `patchedDependencies[patch_key] = patchfile_path`（对齐 `editPatchedDependencies`）
    fn setPatchedDependency(*_, patch_key: []const u8, patchfile_path: []const u8): bool! {
        if (patch_key.len == 0 || patchfile_path.len == 0 || hasControlChar(patch_key) || hasControlChar(patchfile_path)) {
            return err.InvalidPatchKey;
        }
        let i: usize = 0;
        while (i < _.patched_len) {
            if (bytesEql(_.patched[i].patch_key, patch_key)) {
                _.patched[i].patchfile_path = patchfile_path;
                _.total_edits += 1;
                return true;
            }
            i += 1;
        }
        if (_.patched_len >= PKG_JSON_MAX_PATCHED_DEPS) {
            return err.TableFull;
        }
        let insert_at = _.patched_len;
        while (insert_at > 0 && bytesLessThan(patch_key, _.patched[insert_at - 1].patch_key)) {
            _.patched[insert_at] = _.patched[insert_at - 1];
            insert_at -= 1;
        }
        _.patched[insert_at] = PkgJsonPatchEntry{
            patch_key: patch_key,
            patchfile_path: patchfile_path,
        };
        _.patched_len += 1;
        _.total_edits += 1;
        return true;
    }

    fn getPatchedDependency(patch_key: []const u8): ([]const u8)? {
        let i: usize = 0;
        while (i < _.patched_len) {
            if (bytesEql(_.patched[i].patch_key, patch_key)) {
                return _.patched[i].patchfile_path;
            }
            i += 1;
        }
        return null;
    }

    fn removePatchedDependency(*_, patch_key: []const u8): bool {
        let i: usize = 0;
        while (i < _.patched_len) {
            if (bytesEql(_.patched[i].patch_key, patch_key)) {
                let j = i;
                while (j + 1 < _.patched_len) {
                    _.patched[j] = _.patched[j + 1];
                    j += 1;
                }
                _.patched_len -= 1;
                _.patched[_.patched_len] = PkgJsonPatchEntry{};
                _.total_edits += 1;
                return true;
            }
            i += 1;
        }
        return false;
    }

    /// 将指定依赖分组序列化为规范有序 JSON 对象片段，如 `{"a": "^1.0.0", "b": "2.0.0"}`
    fn formatGroupJson(group: PkgJsonDepGroup, out: []u8): usize! {
        const tbl = switch (group) {
            PkgJsonDepGroup.Optional => _.optional_deps,
            PkgJsonDepGroup.Dev => _.dev_deps,
            PkgJsonDepGroup.Prod => _.prod_deps,
            PkgJsonDepGroup.Peer => _.peer_deps,
        };
        let pos: usize = 0;
        * = writeByte(out, *pos, '{')?;
        let i: usize = 0;
        while (i < tbl.len) {
            if (i > 0) {
                * = writeSlice(out, *pos, ", ")?;
            }
            * = writeJsonString(out, *pos, tbl.items[i].name)?;
            * = writeSlice(out, *pos, ": ")?;
            * = writeJsonString(out, *pos, tbl.items[i].version)?;
            i += 1;
        }
        * = writeByte(out, *pos, '}')?;
        return pos;
    }

    /// 将 `trustedDependencies` 序列化为字母序 JSON 数组片段，如 `["esbuild", "sharp"]`
    fn formatTrustedJson(out: []u8): usize! {
        let pos: usize = 0;
        * = writeByte(out, *pos, '[')?;
        let i: usize = 0;
        while (i < _.trusted_len) {
            if (i > 0) {
                * = writeSlice(out, *pos, ", ")?;
            }
            * = writeJsonString(out, *pos, _.trusted[i])?;
            i += 1;
        }
        * = writeByte(out, *pos, ']')?;
        return pos;
    }

    fn stats(): PkgJsonEditStats {
        return PkgJsonEditStats{
            prod_count: _.prod_deps.len,
            dev_count: _.dev_deps.len,
            optional_count: _.optional_deps.len,
            peer_count: _.peer_deps.len,
            trusted_count: _.trusted_len,
            patched_count: _.patched_len,
            total_edits: _.total_edits,
        };
    }
}
