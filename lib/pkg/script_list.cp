// Source: Absorbed from /data/dev/bun/src/install/lockfile/Package/Scripts.cp | Synced: 2026-11-22T06:52:30Z
// Pure-CP zero-allocation package lifecycle script slot table, `node_modules` relative CWD trimmer,
// Bun-compatible completed/untrusted/info formatter, and `trustedDependencies` source-aware allowlist evaluator.

/// 生命周期脚本固定槽位数（`preinstall` / `install` / `postinstall` / `preprepare` / `prepare` / `postprepare`）
pub const PKG_SCRIPT_SLOT_COUNT: usize = 6;

/// 生命周期脚本列表与格式化错误集
pub err PkgScriptListError {
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

fn indexOfSubstr(haystack: []const u8, needle: []const u8): usize? {
    if (needle.len == 0) {
        return 0;
    }
    if (needle.len > haystack.len) {
        return null;
    }
    let i: usize = 0;
    while (i + needle.len <= haystack.len) {
        if (bytesEql(haystack[i..i + needle.len], needle)) {
            return i;
        }
        i += 1;
    }
    return null;
}

fn appendBytes(out: []u8, pos: usize, src: []const u8): usize! {
    if (src.len > out.len - pos) {
        return err.BufferTooSmall;
    }
    let i: usize = 0;
    while (i < src.len) {
        out[pos + i] = src[i];
        i += 1;
    }
    return pos + src.len;
}

fn stripTrailingSlashes(s: []const u8): []const u8 {
    let end = s.len;
    while (end > 1 && s[end - 1] == '/') {
        end -= 1;
    }
    return s[0..end];
}

/// 6 大生命周期脚本槽位枚举（顺序严格对齐 Bun `Lockfile.Scripts.names`）
pub enum PkgScriptSlotKind: u8 {
    Preinstall = 0,
    Install = 1,
    Postinstall = 2,
    Preprepare = 3,
    Prepare = 4,
    Postprepare = 5,

    fn asName(): []const u8 {
        return switch (_) {
            .Preinstall => "preinstall",
            .Install => "install",
            .Postinstall => "postinstall",
            .Preprepare => "preprepare",
            .Prepare => "prepare",
            .Postprepare => "postprepare",
        };
    }

    fn slotIndex(): usize {
        return switch (_) {
            .Preinstall => 0,
            .Install => 1,
            .Postinstall => 2,
            .Preprepare => 3,
            .Prepare => 4,
            .Postprepare => 5,
        };
    }

    static fn fromIndex(idx: usize): PkgScriptSlotKind? {
        return switch (idx) {
            0 => .Preinstall,
            1 => .Install,
            2 => .Postinstall,
            3 => .Preprepare,
            4 => .Prepare,
            5 => .Postprepare,
            else => null,
        };
    }

    static fn fromName(raw: []const u8): PkgScriptSlotKind? {
        if (bytesEql(raw, "preinstall")) return .Preinstall;
        if (bytesEql(raw, "install")) return .Install;
        if (bytesEql(raw, "postinstall")) return .Postinstall;
        if (bytesEql(raw, "preprepare")) return .Preprepare;
        if (bytesEql(raw, "prepare")) return .Prepare;
        if (bytesEql(raw, "postprepare")) return .Postprepare;
        return null;
    }
}

/// 脚本列表渲染格式模式（对齐 Bun `FormatType`: `completed` / `untrusted` / `info`）
pub enum PkgScriptFormatMode: u8 {
    Completed = 0,
    Untrusted = 1,
    Info = 2,

    fn linePrefix(): []const u8 {
        return switch (_) {
            .Completed => " ✓ [",
            .Untrusted => " » [",
            .Info => " [",
        };
    }
}

/// 包安装来源分类（用于 `trustedDependencies` 默认白名单边界判定）
pub enum PkgScriptTrustSource: u8 {
    NpmRegistry = 0,
    LocalFile = 1,
    Symlink = 2,
    Git = 3,
    Tarball = 4,
}

fn listContainsName(list: []const []const u8, name: []const u8): bool {
    let i: usize = 0;
    while (i < list.len) {
        if (bytesEql(list[i], name)) {
            return true;
        }
        i += 1;
    }
    return false;
}

/// 判定指定包的生命周期脚本是否允许执行（严格遵循 Bun `lifecycle.mdx` 安全边界）
///
/// 1. `--ignore-scripts`（`ignore_scripts == true`）拥有最高优先级，一票否决；
/// 2. 显式列入 `trustedDependencies`（`explicit_trusted`）的包允许执行（不传递至子依赖）；
/// 3. 默认白名单（`default_trusted`）**仅适用于 npm Registry 来源**，
///    严禁 `file:` / `link:` / `git:` / 直接 Tarball 来源通过同名仿冒绕过白名单。
pub fn isScriptExecutionAllowed(
    pkg_name: []const u8,
    source: PkgScriptTrustSource,
    ignore_scripts: bool,
    explicit_trusted: []const []const u8,
    default_trusted: []const []const u8,
): bool {
    if (ignore_scripts || pkg_name.len == 0) {
        return false;
    }
    if (listContainsName(explicit_trusted, pkg_name)) {
        return true;
    }
    if (source == .NpmRegistry && listContainsName(default_trusted, pkg_name)) {
        return true;
    }
    return false;
}

/// 裁剪并格式化包含 `/node_modules/` 的包工作目录路径（对齐 Bun `ScriptsList.printScripts`）
///
/// 例：`"/repo/app/node_modules/esbuild/"` -> `"./node_modules/esbuild"`。
pub fn formatNodeModulesRelativeCwd(cwd: []const u8, out: []u8): usize! {
    if (nm_pos := indexOfSubstr(cwd, "/node_modules/")) {
        const trimmed = stripTrailingSlashes(cwd[nm_pos + 1..]);
        let pos = appendBytes(out, 0, "./")?;
        return appendBytes(out, pos, trimmed);
    }
    return appendBytes(out, 0, stripTrailingSlashes(cwd));
}

/// 零堆分配包生命周期脚本清单（对齐 Bun `Lockfile.Package.Scripts.List`）
pub struct PkgScriptList {
    items: [PKG_SCRIPT_SLOT_COUNT]([]const u8)? = [null; PKG_SCRIPT_SLOT_COUNT],
    package_name: []const u8 = "",
    version: []const u8 = "",
    cwd: []const u8 = "",
    first_index: usize = PKG_SCRIPT_SLOT_COUNT,
    total: usize = 0,

    static fn init(package_name: []const u8, version: []const u8, cwd: []const u8): PkgScriptList {
        return PkgScriptList{
            package_name: package_name,
            version: version,
            cwd: cwd,
        };
    }

    fn recomputeMeta(*_) {
        let cnt: usize = 0;
        let first_idx: usize = PKG_SCRIPT_SLOT_COUNT;
        let i: usize = 0;
        while (i < PKG_SCRIPT_SLOT_COUNT) {
            if (_.items[i] != null) {
                if (first_idx == PKG_SCRIPT_SLOT_COUNT) {
                    first_idx = i;
                }
                cnt += 1;
            }
            i += 1;
        }
        _.total = cnt;
        _.first_index = first_idx;
    }

    /// 设置或清空（传入空串）指定阶段的脚本命令
    fn set(*_, slot: PkgScriptSlotKind, script: []const u8) {
        const idx = slot.slotIndex();
        if (script.len == 0) {
            _.items[idx] = null;
        } else {
            _.items[idx] = script;
        }
        _.recomputeMeta();
    }

    fn get(slot: PkgScriptSlotKind): ([]const u8)? {
        return _.items[slot.slotIndex()];
    }

    fn count(): usize {
        return _.total;
    }

    fn isEmpty(): bool {
        return _.total == 0;
    }

    /// 返回按生命周期顺序排在最前的非空脚本命令
    fn first(): ([]const u8)? {
        if (_.first_index >= PKG_SCRIPT_SLOT_COUNT) {
            return null;
        }
        return _.items[_.first_index];
    }

    /// 比较两个脚本清单的 6 个阶段脚本内容是否完全一致（对齐 Bun `Scripts.eql`）
    fn eql(other: PkgScriptList): bool {
        let i: usize = 0;
        while (i < PKG_SCRIPT_SLOT_COUNT) {
            if (s1 := _.items[i]) {
                if (s2 := other.items[i]) {
                    if (!bytesEql(s1, s2)) {
                        return false;
                    }
                } else {
                    return false;
                }
            } else if (other.items[i] != null) {
                return false;
            }
            i += 1;
        }
        return true;
    }

    /// 按指定格式模式渲染包目录头与各阶段脚本行（对齐 Bun `ScriptsList.printScripts`）
    fn format(mode: PkgScriptFormatMode, out: []u8): usize! {
        let pos = formatNodeModulesRelativeCwd(_.cwd, out)?;
        if (_.version.len > 0) {
            pos = appendBytes(out, pos, " @")?;
            pos = appendBytes(out, pos, _.version)?;
        }
        pos = appendBytes(out, pos, "\n")?;

        let i: usize = 0;
        while (i < PKG_SCRIPT_SLOT_COUNT) {
            if (script := _.items[i]) {
                const slot = PkgScriptSlotKind.fromIndex(i) ?? .Preinstall;
                pos = appendBytes(out, pos, mode.linePrefix())?;
                pos = appendBytes(out, pos, slot.asName())?;
                pos = appendBytes(out, pos, "]: ")?;
                pos = appendBytes(out, pos, script)?;
                pos = appendBytes(out, pos, "\n")?;
            }
            i += 1;
        }
        return pos;
    }
}
