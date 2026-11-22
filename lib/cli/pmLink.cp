// Source: Absorbed from /data/dev/bun/src/cli/link_command.cp | Synced: 2026-11-22T15:00:00Z
// Pure-CP zero-allocation `bun link` / `bun link <pkg>` / `bun unlink` CLI Planner:
// - Validates NPM package names (unscoped & `@scope/name`, <= 214 chars, traversal-safe)
// - Classifies CLI invocations (`RegisterLocal`, `LinkIntoProject`, `UnlinkLocal`) with `--save` support
// - Plans cross-platform global `node_modules` link targets (POSIX directory symlink vs Windows junction)
//   and scoped parent directory creation (`@scope`)
// - Formats canonical `"link:<pkg>"` dependency specifiers and registration guidance messages.

/// NPM 包名最大长度（字节）
pub const PM_LINK_MAX_NAME_LEN: usize = 214;
/// 规划路径内联缓冲区最大长度
pub const PM_LINK_MAX_PATH_LEN: usize = 512;
/// 单次 `bun link <pkg1> <pkg2>...` 支持的最大包名数量
pub const PM_LINK_MAX_TARGETS: usize = 16;

/// `pmLink` 规划与校验错误集
pub err PmLinkError {
    EmptyPackageName,
    InvalidPackageName,
    EmptyDirPath,
    TooManyTargets,
    BufferTooSmall,
}

/// 链接命令执行模式
pub enum PmLinkMode: u8 {
    /// 无位置包名：`bun link` 注册当前目录至全局 link 目录
    RegisterLocal = 0,
    /// 指定包名：`bun link <pkg> [--save]` 将已注册全局包链接进当前项目
    LinkIntoProject = 1,
    /// `bun unlink` 注销当前目录的全局链接
    UnlinkLocal = 2,
}

/// 跨平台目录链接机制（对齐 Bun `link_command.cp` POSIX symlink vs Windows junction）
pub enum PmLinkMethod: u8 {
    PosixSymlink = 0,
    WindowsJunction = 1,
}

/// 校验包名是否符合 NPM 链接规范（支持 unscoped 与 `@scope/name`，拦截大写、空白与路径穿越）
pub fn isValidNpmLinkName(name: []const u8): bool {
    if (name.len == 0 || name.len > PM_LINK_MAX_NAME_LEN) {
        return false;
    }
    if (name[0] == '.' || name[0] == '_' || name[0] == '/') {
        return false;
    }

    let slash_count: usize = 0;
    let slash_pos: usize = 0;
    let i: usize = 0;
    while (i < name.len) {
        const c = name[i];
        if (c == '/') {
            slash_count += 1;
            slash_pos = i;
        } else if (c == '\\' || c <= 0x20 || c >= 0x7f) {
            return false;
        } else if (c >= 'A' && c <= 'Z') {
            return false;
        } else if (c == ':' || c == '*' || c == '?' || c == '"' || c == '<' || c == '>' || c == '|') {
            return false;
        }
        i += 1;
    }

    if (name[0] == '@') {
        // Scoped 包必须为 `@scope/pkg`，且 scope 与 pkg 均非空且不得含 `..`
        if (slash_count != 1) return false;
        if (slash_pos <= 1 || slash_pos + 1 >= name.len) return false;
        const scope_part = name[1..slash_pos];
        const pkg_part = name[slash_pos + 1 ..];
        if (isDotSegment(scope_part) || isDotSegment(pkg_part)) return false;
        return true;
    }

    if (slash_count != 0) return false;
    if (isDotSegment(name)) return false;
    return true;
}

/// 提取 scoped 包名的 `@scope` 前缀（非 scoped 包返回 `null`）
pub fn extractLinkScope(name: []const u8): ([]const u8)? {
    if (name.len < 3 || name[0] != '@') {
        return null;
    }
    let i: usize = 1;
    while (i < name.len) {
        if (name[i] == '/') {
            if (i <= 1 || i + 1 >= name.len) return null;
            return name[0..i];
        }
        i += 1;
    }
    return null;
}

/// 剥离路径末尾多余的 `/` 或 `\`（保留单独的根 `/` 或驱动器根 `C:\`）
pub fn trimTrailingPathSep(path: []const u8): []const u8 {
    let end = path.len;
    while (end > 1 && (path[end - 1] == '/' || path[end - 1] == '\\')) {
        // 保留 Windows 盘符根如 `C:\`
        if (end == 3 && path[1] == ':') {
            break;
        }
        end -= 1;
    }
    return path[0..end];
}

/// 格式化 `package.json` 依赖值 `"link:<name>"`
pub fn formatLinkDependencySpec(name: []const u8, out: []u8): usize! {
    if (!isValidNpmLinkName(name)) {
        return err.InvalidPackageName;
    }
    let pos: usize = 0;
    pos = appendBytes(out, pos, "link:")?;
    pos = appendBytes(out, pos, name)?;
    return pos;
}

/// 格式化 `bun link` 注册成功提示（对齐 Bun `link_command.cp` L194）
pub fn formatLinkSuccessMessage(name: []const u8, out: []u8): usize! {
    if (!isValidNpmLinkName(name)) {
        return err.InvalidPackageName;
    }
    let pos: usize = 0;
    pos = appendBytes(out, pos, "Success! Registered \"")?;
    pos = appendBytes(out, pos, name)?;
    pos = appendBytes(out, pos, "\"\n\nTo use ")?;
    pos = appendBytes(out, pos, name)?;
    pos = appendBytes(out, pos, " in a project, run:\n  bun link ")?;
    pos = appendBytes(out, pos, name)?;
    pos = appendBytes(out, pos, "\n\nOr add it in dependencies in your package.json file:\n  \"")?;
    pos = appendBytes(out, pos, name)?;
    pos = appendBytes(out, pos, "\": \"link:")?;
    pos = appendBytes(out, pos, name)?;
    pos = appendBytes(out, pos, "\"\n")?;
    return pos;
}

/// 解析后的 `bun link` / `bun unlink` CLI 调用意图
pub struct PmLinkInvocation {
    mode: PmLinkMode = .RegisterLocal,
    save_to_pkg_json: bool = false,
    targets: [PM_LINK_MAX_TARGETS]([]const u8) = [""; PM_LINK_MAX_TARGETS],
    target_count: usize = 0,

    /// 解析 `bun link` 或 `bun unlink` 后的参数列表
    static fn parseArgs(args: []const ([]const u8), is_unlink: bool): PmLinkInvocation! {
        let inv = PmLinkInvocation{
            mode: if (is_unlink) .UnlinkLocal else .RegisterLocal,
        };
        let i: usize = 0;
        while (i < args.len) {
            const a = args[i];
            i += 1;
            if (a.len == 0) continue;
            if (bytesEql(a, "--save") || bytesEql(a, "-s")) {
                inv.save_to_pkg_json = true;
                continue;
            }
            if (a[0] == '-') {
                // 其他全局 CLI 标志由上层解析，此处跳过
                continue;
            }
            if (!isValidNpmLinkName(a)) {
                return err.InvalidPackageName;
            }
            if (inv.target_count >= PM_LINK_MAX_TARGETS) {
                return err.TooManyTargets;
            }
            inv.targets[inv.target_count] = a;
            inv.target_count += 1;
        }
        if (!is_unlink && inv.target_count > 0) {
            inv.mode = .LinkIntoProject;
        }
        return inv;
    }

    fn targetSlice(): []const ([]const u8) {
        return _.targets[0.._.target_count];
    }
}

/// 全局包注册链接计划（描述目标链接路径、作用域子目录、源目录与跨平台链接方式）
pub struct PmGlobalLinkPlan {
    package_name: []const u8 = "",
    scope_dir: ([]const u8)? = null,
    method: PmLinkMethod = .PosixSymlink,
    has_global_bins: bool = false,
    source_dir_buf: [PM_LINK_MAX_PATH_LEN]u8 = [0; PM_LINK_MAX_PATH_LEN],
    source_dir_len: usize = 0,
    dest_link_buf: [PM_LINK_MAX_PATH_LEN]u8 = [0; PM_LINK_MAX_PATH_LEN],
    dest_link_len: usize = 0,

    /// 为本地包生成全局 `node_modules/<pkg>` 链接计划
    static fn planRegister(
        pkg_name: []const u8,
        local_root: []const u8,
        global_dir: []const u8,
        is_windows: bool,
        has_bins: bool,
    ): PmGlobalLinkPlan! {
        if (pkg_name.len == 0) return err.EmptyPackageName;
        if (!isValidNpmLinkName(pkg_name)) return err.InvalidPackageName;
        if (local_root.len == 0 || global_dir.len == 0) return err.EmptyDirPath;

        const clean_src = trimTrailingPathSep(local_root);
        const clean_global = trimTrailingPathSep(global_dir);
        if (clean_src.len > PM_LINK_MAX_PATH_LEN) return err.BufferTooSmall;

        let plan = PmGlobalLinkPlan{
            package_name: pkg_name,
            scope_dir: extractLinkScope(pkg_name),
            method: if (is_windows) .WindowsJunction else .PosixSymlink,
            has_global_bins: has_bins,
            source_dir_len: clean_src.len,
        };
        copyBytes(plan.source_dir_buf[0..clean_src.len], clean_src);

        const sep: u8 = if (is_windows) '\\' else '/';
        let pos: usize = 0;
        pos = appendBytes(plan.dest_link_buf[0..], pos, clean_global)?;
        if (pos > 0 && plan.dest_link_buf[pos - 1] != '/' && plan.dest_link_buf[pos - 1] != '\\') {
            pos = appendChar(plan.dest_link_buf[0..], pos, sep)?;
        }
        pos = appendBytes(plan.dest_link_buf[0..], pos, "node_modules")?;
        pos = appendChar(plan.dest_link_buf[0..], pos, sep)?;

        // Windows 下将 `@scope/pkg` 中的 `/` 规范为 `\`
        let i: usize = 0;
        while (i < pkg_name.len) {
            const c = pkg_name[i];
            const out_c = if (is_windows && c == '/') '\\' else c;
            pos = appendChar(plan.dest_link_buf[0..], pos, out_c)?;
            i += 1;
        }
        plan.dest_link_len = pos;
        return plan;
    }

    fn sourceDir(): []const u8 {
        return _.source_dir_buf[0.._.source_dir_len];
    }

    fn destLinkPath(): []const u8 {
        return _.dest_link_buf[0.._.dest_link_len];
    }

    fn needsScopeMkdir(): bool {
        return _.scope_dir != null;
    }
}

// ----------------------------------------------------------------------------
// 内部零分配辅助函数
// ----------------------------------------------------------------------------

fn isDotSegment(s: []const u8): bool {
    return bytesEql(s, ".") || bytesEql(s, "..");
}

fn bytesEql(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) {
        if (a[i] != b[i]) return false;
        i += 1;
    }
    return true;
}

fn copyBytes(dst: []u8, src: []const u8) {
    let i: usize = 0;
    while (i < src.len) {
        dst[i] = src[i];
        i += 1;
    }
}

fn appendBytes(out: []u8, pos: usize, src: []const u8): usize! {
    if (src.len > out.len - pos) return err.BufferTooSmall;
    let i: usize = 0;
    while (i < src.len) {
        out[pos + i] = src[i];
        i += 1;
    }
    return pos + src.len;
}

fn appendChar(out: []u8, pos: usize, ch: u8): usize! {
    if (pos >= out.len) return err.BufferTooSmall;
    out[pos] = ch;
    return pos + 1;
}
