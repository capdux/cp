// Source: Absorbed from /data/dev/bun/src/install/lockfile/printer/tree_printer.cp | Synced: 2026-11-22T06:58:00Z
// Pure-CP zero-allocation lockfile installation diff & tree summary printer:
// workspace-grouped upgrade (`↑`/`^`) & install (`+`) sections, duplicate dependency
// deduplication, `(vX.Y.Z available)` cache hint formatting, `--verbose` multi-workspace
// reporting, full lockfile fallback listing, and explicit `installed ... with binaries:` summary.

/// `InstallTreePrinter` 支持的最大包条目数
pub const TREE_PRINTER_MAX_PACKAGES: usize = 64;

/// `InstallTreePrinter` 支持的最大依赖边数
pub const TREE_PRINTER_MAX_DEPS: usize = 128;

/// 单个包支持的最大可执行二进制（`bin`）条目数
pub const TREE_PRINTER_MAX_BINS_PER_PKG: usize = 8;

/// 单次安装事务支持的最大显式更新请求数（如 `bun add foo bar`）
pub const TREE_PRINTER_MAX_UPDATES: usize = 32;

const INVALID_DEP_ID: usize = 0xFFFF_FFFF;

const ANSI_RESET: []const u8 = "\x1b[0m";
const ANSI_BOLD: []const u8 = "\x1b[1m";
const ANSI_DIM: []const u8 = "\x1b[2m";
const ANSI_GREEN: []const u8 = "\x1b[32m";
const ANSI_BLUE: []const u8 = "\x1b[34m";
const ANSI_CYAN: []const u8 = "\x1b[36m";

/// `cap:pkg` 安装树渲染器错误集
pub err TreePrintError {
    BufferTooSmall,
    TooManyPackages,
    TooManyDeps,
    TooManyBins,
    TooManyUpdates,
    InvalidPackageId,
    UpdateFailed,
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

/// Workspace 分段标题渲染控制（对应 Bun `PrintSectionHeader`）
pub enum TreePrintSectionHeader: u8 {
    DontPrint = 0,
    PrintHeader = 1,
}

/// 安装日志详细级别（对应 Bun `PackageManager.Options.LogLevel`）
pub enum TreePrintLogLevel: u8 {
    Silent = 0,
    Default = 1,
    Verbose = 2,

    fn isVerbose(): bool {
        return _ == .Verbose;
    }

    fn isSilent(): bool {
        return _ == .Silent;
    }
}

/// 依赖打印决策结果（对应 Bun `ShouldPrintPackageInstallResult`）
pub enum TreePrintDecisionKind: u8 {
    Yes = 0,
    No = 1,
    Abort = 2,
    Update = 3,
}

/// 包二进制声明类型（对应 Bun `Bin.Tag`）
pub enum TreePackageBinKind: u8 {
    None = 0,
    Dir = 1,
    File = 2,
    NamedFile = 3,
    Map = 4,

    fn hasNamedBinaries(): bool {
        return _ == .File || _ == .NamedFile || _ == .Map;
    }
}

/// 已解析包条目（对应 lockfile 中的单个包元数据快照）
pub struct TreePackageEntry {
    name: []const u8 = "",
    version: []const u8 = "",
    is_workspace: bool = false,
    installed: bool = false,
    later_version: []const u8 = "",
    bin_kind: TreePackageBinKind = .None,
    bin_names: [TREE_PRINTER_MAX_BINS_PER_PKG][]const u8 = [""; TREE_PRINTER_MAX_BINS_PER_PKG],
    bin_count: usize = 0,
}

/// Workspace/Root 到目标包的依赖边
pub struct TreeDepEdge {
    owner_pkg_id: usize = 0,
    dep_name: []const u8 = "",
    target_pkg_id: usize = 0,
    is_workspace_dep: bool = false,
    is_peer: bool = false,
    is_disabled: bool = false,
    original_version: []const u8 = "",
}

/// 显式安装/更新请求条目（对应 `bun add <pkg>` / `bun update <pkg>` 的 `printer.updates`）
pub struct TreeUpdateRequest {
    name: []const u8 = "",
    failed: bool = false,
}

/// `InstallTreePrinter.render` 渲染选项
pub struct TreePrintOptions {
    ansi_colors: bool = false,
    log_level: TreePrintLogLevel = .Default,
    has_installed_filter: bool = true,
    current_workspace_id: usize = 0,
}

/// `InstallTreePrinter.render` 渲染统计结果
pub struct TreePrintStats {
    bytes_written: usize = 0,
    updated_count: usize = 0,
    installed_count: usize = 0,
    explicit_request_count: usize = 0,
    bin_count: usize = 0,
    workspace_sections: usize = 0,
}

/// 格式化单行包升级摘要：
/// - `ansi_colors=false`: `^ <name> <old> -> <new>\n`
/// - `ansi_colors=true`: ANSI 彩色 `↑ <name> <old> → <new>\n`
pub fn formatUpdatedPackageLine(
    name: []const u8,
    old_version: []const u8,
    new_version: []const u8,
    ansi_colors: bool,
    out: []u8,
): usize! {
    let pos: usize = 0;
    if (ansi_colors) {
        pos = appendBytes(out, pos, ANSI_RESET)?;
        pos = appendBytes(out, pos, ANSI_CYAN)?;
        pos = appendBytes(out, pos, "↑")?;
        pos = appendBytes(out, pos, ANSI_RESET)?;
        pos = appendBytes(out, pos, " ")?;
        pos = appendBytes(out, pos, ANSI_BOLD)?;
        pos = appendBytes(out, pos, name)?;
        pos = appendBytes(out, pos, ANSI_RESET)?;
        pos = appendBytes(out, pos, ANSI_DIM)?;
        pos = appendBytes(out, pos, " ")?;
        pos = appendBytes(out, pos, ANSI_BOLD)?;
        pos = appendBytes(out, pos, old_version)?;
        pos = appendBytes(out, pos, " →")?;
        pos = appendBytes(out, pos, ANSI_RESET)?;
        pos = appendBytes(out, pos, " ")?;
        pos = appendBytes(out, pos, ANSI_BOLD)?;
        pos = appendBytes(out, pos, ANSI_CYAN)?;
        pos = appendBytes(out, pos, new_version)?;
        pos = appendBytes(out, pos, ANSI_RESET)?;
        pos = appendBytes(out, pos, "\n")?;
    } else {
        pos = appendBytes(out, pos, "^ ")?;
        pos = appendBytes(out, pos, name)?;
        pos = appendBytes(out, pos, " ")?;
        pos = appendBytes(out, pos, old_version)?;
        pos = appendBytes(out, pos, " -> ")?;
        pos = appendBytes(out, pos, new_version)?;
        pos = appendBytes(out, pos, "\n")?;
    }
    return pos;
}

/// 格式化单行新增安装包摘要：
/// - 无更新缓存提示：`+ <name>@<version>\n`
/// - 有更新缓存提示：`+ <name>@<version> (v<later_version> available)\n`
pub fn formatInstalledPackageLine(
    name: []const u8,
    version: []const u8,
    later_version: []const u8,
    ansi_colors: bool,
    out: []u8,
): usize! {
    let pos: usize = 0;
    if (ansi_colors) {
        pos = appendBytes(out, pos, ANSI_RESET)?;
        pos = appendBytes(out, pos, ANSI_GREEN)?;
        pos = appendBytes(out, pos, "+")?;
        pos = appendBytes(out, pos, ANSI_RESET)?;
        pos = appendBytes(out, pos, " ")?;
        pos = appendBytes(out, pos, ANSI_BOLD)?;
        pos = appendBytes(out, pos, name)?;
        pos = appendBytes(out, pos, ANSI_RESET)?;
        pos = appendBytes(out, pos, ANSI_DIM)?;
        pos = appendBytes(out, pos, "@")?;
        pos = appendBytes(out, pos, version)?;
        pos = appendBytes(out, pos, ANSI_RESET)?;
        if (later_version.len > 0) {
            pos = appendBytes(out, pos, " ")?;
            pos = appendBytes(out, pos, ANSI_DIM)?;
            pos = appendBytes(out, pos, "(")?;
            pos = appendBytes(out, pos, ANSI_BLUE)?;
            pos = appendBytes(out, pos, "v")?;
            pos = appendBytes(out, pos, later_version)?;
            pos = appendBytes(out, pos, " available")?;
            pos = appendBytes(out, pos, ANSI_RESET)?;
            pos = appendBytes(out, pos, ANSI_DIM)?;
            pos = appendBytes(out, pos, ")")?;
            pos = appendBytes(out, pos, ANSI_RESET)?;
        }
        pos = appendBytes(out, pos, "\n")?;
    } else {
        pos = appendBytes(out, pos, "+ ")?;
        pos = appendBytes(out, pos, name)?;
        pos = appendBytes(out, pos, "@")?;
        pos = appendBytes(out, pos, version)?;
        if (later_version.len > 0) {
            pos = appendBytes(out, pos, " (v")?;
            pos = appendBytes(out, pos, later_version)?;
            pos = appendBytes(out, pos, " available)")?;
        }
        pos = appendBytes(out, pos, "\n")?;
    }
    return pos;
}

/// 零分配 Lockfile 安装差异与依赖树摘要渲染器（吸收自 Bun `lockfile/printer/tree_printer.cp`）
pub struct InstallTreePrinter {
    packages: [TREE_PRINTER_MAX_PACKAGES]TreePackageEntry,
    package_count: usize,
    deps: [TREE_PRINTER_MAX_DEPS]TreeDepEdge,
    dep_count: usize,
    updates: [TREE_PRINTER_MAX_UPDATES]TreeUpdateRequest,
    update_count: usize,

    static fn init(): InstallTreePrinter {
        return InstallTreePrinter{
            packages: [TreePackageEntry{}; TREE_PRINTER_MAX_PACKAGES],
            package_count: 0,
            deps: [TreeDepEdge{}; TREE_PRINTER_MAX_DEPS],
            dep_count: 0,
            updates: [TreeUpdateRequest{}; TREE_PRINTER_MAX_UPDATES],
            update_count: 0,
        };
    }

    /// 注册包条目并返回其 `pkg_id`（`0` 通常为根项目 workspace）
    fn addPackage(
        *_,
        name: []const u8,
        version: []const u8,
        is_workspace: bool,
        installed: bool,
    ): usize! {
        if (_.package_count >= TREE_PRINTER_MAX_PACKAGES) {
            return err.TooManyPackages;
        }
        const id = _.package_count;
        _.packages[id] = TreePackageEntry{
            name: name,
            version: version,
            is_workspace: is_workspace,
            installed: installed,
            later_version: "",
            bin_kind: .None,
            bin_names: [""; TREE_PRINTER_MAX_BINS_PER_PKG],
            bin_count: 0,
        };
        _.package_count += 1;
        return id;
    }

    /// 设置某包在本地缓存中发现的更新版本号（渲染为 `(vX.Y.Z available)`）
    fn setPackageLaterVersion(*_, pkg_id: usize, later_version: []const u8)! {
        if (pkg_id >= _.package_count) {
            return err.InvalidPackageId;
        }
        _.packages[pkg_id].later_version = later_version;
    }

    /// 为指定包附加一个可执行二进制名称（对应 `package.json#bin`）
    fn addPackageBin(*_, pkg_id: usize, bin_kind: TreePackageBinKind, bin_name: []const u8)! {
        if (pkg_id >= _.package_count) {
            return err.InvalidPackageId;
        }
        if (_.packages[pkg_id].bin_count >= TREE_PRINTER_MAX_BINS_PER_PKG) {
            return err.TooManyBins;
        }
        _.packages[pkg_id].bin_kind = bin_kind;
        if (bin_name.len > 0) {
            const idx = _.packages[pkg_id].bin_count;
            _.packages[pkg_id].bin_names[idx] = bin_name;
            _.packages[pkg_id].bin_count += 1;
        }
    }

    /// 注册一条依赖解析边
    fn addDep(*_, edge: TreeDepEdge): usize! {
        if (_.dep_count >= TREE_PRINTER_MAX_DEPS) {
            return err.TooManyDeps;
        }
        const id = _.dep_count;
        _.deps[id] = edge;
        _.dep_count += 1;
        return id;
    }

    /// 注册一条显式安装/更新请求（如 `bun add zod`）
    fn addUpdateRequest(*_, name: []const u8, failed: bool): usize! {
        if (_.update_count >= TREE_PRINTER_MAX_UPDATES) {
            return err.TooManyUpdates;
        }
        const id = _.update_count;
        _.updates[id] = TreeUpdateRequest{
            name: name,
            failed: failed,
        };
        _.update_count += 1;
        return id;
    }

    fn evaluateDep(
        dep_idx: usize,
        id_map: (*[TREE_PRINTER_MAX_UPDATES]usize)?,
    ): TreePrintDecisionKind {
        if (dep_idx >= _.dep_count) {
            return .No;
        }
        const dep = _.deps[dep_idx];
        if (dep.is_workspace_dep || dep.target_pkg_id >= _.package_count) {
            return .No;
        }

        if (map_ptr := id_map) {
            let u: usize = 0;
            while (u < _.update_count) {
                const upd = _.updates[u];
                if (upd.failed) {
                    return .Abort;
                }
                if (bytesEql(upd.name, dep.dep_name)) {
                    if (map_ptr.*[u] == INVALID_DEP_ID) {
                        map_ptr.*[u] = dep_idx;
                    }
                    return .No;
                }
                u += 1;
            }
        }

        const pkg = _.packages[dep.target_pkg_id];
        if (!pkg.installed) {
            return .No;
        }
        if (dep.is_disabled) {
            return .No;
        }

        if (dep.original_version.len > 0 && !bytesEql(dep.original_version, pkg.version)) {
            return .Update;
        }
        return .Yes;
    }

    /// 无副作用决策查询：判断指定依赖边在当前状态下是新增、升级、跳过还是中止
    fn shouldPrintDep(dep_idx: usize, use_update_requests: bool): TreePrintDecisionKind {
        if (use_update_requests) {
            let tmp_map: [TREE_PRINTER_MAX_UPDATES]usize = [INVALID_DEP_ID; TREE_PRINTER_MAX_UPDATES];
            let opt_ptr: (*[TREE_PRINTER_MAX_UPDATES]usize)? = &tmp_map;
            return _.evaluateDep(dep_idx, opt_ptr);
        }
        return _.evaluateDep(dep_idx, null);
    }

    fn writeSectionHeader(
        workspace_name: []const u8,
        ansi_colors: bool,
        out: []u8,
        pos: usize,
    ): usize! {
        let p = pos;
        if (ansi_colors) {
            p = appendBytes(out, p, ANSI_RESET)?;
            p = appendBytes(out, p, "\n")?;
            p = appendBytes(out, p, ANSI_CYAN)?;
            p = appendBytes(out, p, workspace_name)?;
            p = appendBytes(out, p, ANSI_RESET)?;
            p = appendBytes(out, p, ANSI_DIM)?;
            p = appendBytes(out, p, ":")?;
            p = appendBytes(out, p, ANSI_RESET)?;
            p = appendBytes(out, p, "\n")?;
        } else {
            p = appendBytes(out, p, "\n")?;
            p = appendBytes(out, p, workspace_name)?;
            p = appendBytes(out, p, ":\n")?;
        }
        return p;
    }

    fn renderWorkspaceSection(
        workspace_pkg_id: usize,
        section_header: TreePrintSectionHeader,
        ansi_colors: bool,
        id_map: (*[TREE_PRINTER_MAX_UPDATES]usize)?,
        out: []u8,
        pos: *usize,
        had_printed_new_install: *bool,
        stats: *TreePrintStats,
    )! {
        if (workspace_pkg_id >= _.package_count) {
            return;
        }
        let printed_section_header = false;
        let printed_update = false;

        // Pass 1: upgraded packages
        let i: usize = 0;
        while (i < _.dep_count) {
            const dep = _.deps[i];
            if (dep.owner_pkg_id == workspace_pkg_id) {
                const decision = _.evaluateDep(i, id_map);
                if (decision == .Abort) {
                    return err.UpdateFailed;
                }
                if (decision == .Update) {
                    had_printed_new_install.* = true;
                    printed_update = true;
                    if (section_header == .PrintHeader && !printed_section_header) {
                        printed_section_header = true;
                        stats.*.workspace_sections += 1;
                        pos.* = _.writeSectionHeader(_.packages[workspace_pkg_id].name, ansi_colors, out, pos.*)?;
                    }
                    const pkg = _.packages[dep.target_pkg_id];
                    const n = formatUpdatedPackageLine(
                        dep.dep_name,
                        dep.original_version,
                        pkg.version,
                        ansi_colors,
                        out[pos.*..],
                    )?;
                    pos.* += n;
                    stats.*.updated_count += 1;
                }
            }
            i += 1;
        }

        // Pass 2: newly installed packages with dep_name deduplication
        let seen_names: [TREE_PRINTER_MAX_DEPS][]const u8 = [""; TREE_PRINTER_MAX_DEPS];
        let seen_count: usize = 0;

        i = 0;
        while (i < _.dep_count) {
            const dep = _.deps[i];
            if (dep.owner_pkg_id == workspace_pkg_id) {
                const decision = _.evaluateDep(i, id_map);
                if (decision == .Abort) {
                    return err.UpdateFailed;
                }
                if (decision == .Yes) {
                    let dup = false;
                    let s: usize = 0;
                    while (s < seen_count) {
                        if (bytesEql(seen_names[s], dep.dep_name)) {
                            dup = true;
                            break;
                        }
                        s += 1;
                    }
                    if (!dup) {
                        seen_names[seen_count] = dep.dep_name;
                        seen_count += 1;
                        had_printed_new_install.* = true;

                        if (section_header == .PrintHeader && !printed_section_header) {
                            printed_section_header = true;
                            stats.*.workspace_sections += 1;
                            pos.* = _.writeSectionHeader(_.packages[workspace_pkg_id].name, ansi_colors, out, pos.*)?;
                        }
                        if (printed_update) {
                            printed_update = false;
                            pos.* = appendBytes(out, pos.*, "\n")?;
                        }
                        const pkg = _.packages[dep.target_pkg_id];
                        const n = formatInstalledPackageLine(
                            dep.dep_name,
                            pkg.version,
                            pkg.later_version,
                            ansi_colors,
                            out[pos.*..],
                        )?;
                        pos.* += n;
                        stats.*.installed_count += 1;
                    }
                }
            }
            i += 1;
        }
    }

    /// 渲染完整安装摘要到调用方提供的 `out` 缓冲区（零堆分配）
    fn render(opts: TreePrintOptions, out: []u8): TreePrintStats! {
        let stats = TreePrintStats{};
        if (opts.log_level.isSilent()) {
            return stats;
        }

        let pos: usize = 0;
        pos = appendBytes(out, pos, "\n")?;
        if (_.package_count == 0 || _.dep_count == 0) {
            stats.bytes_written = pos;
            return stats;
        }

        let id_map: [TREE_PRINTER_MAX_UPDATES]usize = [INVALID_DEP_ID; TREE_PRINTER_MAX_UPDATES];
        let had_printed_new_install = false;

        if (opts.has_installed_filter) {
            if (opts.log_level.isVerbose()) {
                // Root workspace first (no section header)
                _.renderWorkspaceSection(
                    0,
                    .DontPrint,
                    opts.ansi_colors,
                    null,
                    out,
                    &pos,
                    &had_printed_new_install,
                    &stats,
                )?;

                // Each child workspace referenced by root
                let d: usize = 0;
                while (d < _.dep_count) {
                    const edge = _.deps[d];
                    if (edge.owner_pkg_id == 0 && edge.is_workspace_dep && edge.target_pkg_id < _.package_count) {
                        _.renderWorkspaceSection(
                            edge.target_pkg_id,
                            .PrintHeader,
                            opts.ansi_colors,
                            null,
                            out,
                            &pos,
                            &had_printed_new_install,
                            &stats,
                        )?;
                    }
                    d += 1;
                }
            } else {
                let ws_id = opts.current_workspace_id;
                if (ws_id >= _.package_count) {
                    ws_id = 0;
                }
                let opt_map: (*[TREE_PRINTER_MAX_UPDATES]usize)? = &id_map;
                _.renderWorkspaceSection(
                    ws_id,
                    .DontPrint,
                    opts.ansi_colors,
                    opt_map,
                    out,
                    &pos,
                    &had_printed_new_install,
                    &stats,
                )?;
            }
        } else {
            // Fallback full lockfile mode (when `successfully_installed` is null)
            let d: usize = 0;
            while (d < _.dep_count) {
                const dep = _.deps[d];
                if (dep.target_pkg_id < _.package_count && !dep.is_peer) {
                    let matched_update = false;
                    let u: usize = 0;
                    while (u < _.update_count) {
                        if (_.updates[u].failed) {
                            return err.UpdateFailed;
                        }
                        if (bytesEql(_.updates[u].name, dep.dep_name)) {
                            if (id_map[u] == INVALID_DEP_ID) {
                                id_map[u] = d;
                            }
                            matched_update = true;
                            break;
                        }
                        u += 1;
                    }
                    if (!matched_update) {
                        const pkg = _.packages[dep.target_pkg_id];
                        if (opts.ansi_colors) {
                            pos = appendBytes(out, pos, " ")?;
                            pos = appendBytes(out, pos, ANSI_RESET)?;
                            pos = appendBytes(out, pos, ANSI_BOLD)?;
                            pos = appendBytes(out, pos, dep.dep_name)?;
                            pos = appendBytes(out, pos, ANSI_RESET)?;
                            pos = appendBytes(out, pos, ANSI_DIM)?;
                            pos = appendBytes(out, pos, "@")?;
                            pos = appendBytes(out, pos, ANSI_BOLD)?;
                            pos = appendBytes(out, pos, pkg.version)?;
                            pos = appendBytes(out, pos, ANSI_RESET)?;
                            pos = appendBytes(out, pos, "\n")?;
                        } else {
                            pos = appendBytes(out, pos, " ")?;
                            pos = appendBytes(out, pos, dep.dep_name)?;
                            pos = appendBytes(out, pos, "@")?;
                            pos = appendBytes(out, pos, pkg.version)?;
                            pos = appendBytes(out, pos, "\n")?;
                        }
                        stats.installed_count += 1;
                    }
                }
                d += 1;
            }
        }

        if (had_printed_new_install) {
            pos = appendBytes(out, pos, "\n")?;
        }

        // Explicit update request summary (`installed <pkg>@<ver> [with binaries:]`)
        let printed_explicit = false;
        let u: usize = 0;
        while (u < _.update_count) {
            const dep_idx = id_map[u];
            if (dep_idx != INVALID_DEP_ID && dep_idx < _.dep_count) {
                const dep = _.deps[dep_idx];
                const pkg = _.packages[dep.target_pkg_id];
                printed_explicit = true;
                stats.explicit_request_count += 1;

                if (pkg.bin_kind.hasNamedBinaries() && pkg.bin_count > 0) {
                    if (opts.ansi_colors) {
                        pos = appendBytes(out, pos, ANSI_RESET)?;
                        pos = appendBytes(out, pos, ANSI_GREEN)?;
                        pos = appendBytes(out, pos, "installed")?;
                        pos = appendBytes(out, pos, ANSI_RESET)?;
                        pos = appendBytes(out, pos, " ")?;
                        pos = appendBytes(out, pos, dep.dep_name)?;
                        pos = appendBytes(out, pos, ANSI_RESET)?;
                        pos = appendBytes(out, pos, ANSI_DIM)?;
                        pos = appendBytes(out, pos, "@")?;
                        pos = appendBytes(out, pos, pkg.version)?;
                        pos = appendBytes(out, pos, ANSI_RESET)?;
                        pos = appendBytes(out, pos, " with binaries:\n")?;
                    } else {
                        pos = appendBytes(out, pos, "installed ")?;
                        pos = appendBytes(out, pos, dep.dep_name)?;
                        pos = appendBytes(out, pos, "@")?;
                        pos = appendBytes(out, pos, pkg.version)?;
                        pos = appendBytes(out, pos, " with binaries:\n")?;
                    }
                    let b: usize = 0;
                    while (b < pkg.bin_count) {
                        if (opts.ansi_colors) {
                            pos = appendBytes(out, pos, ANSI_RESET)?;
                            pos = appendBytes(out, pos, " ")?;
                            pos = appendBytes(out, pos, ANSI_DIM)?;
                            pos = appendBytes(out, pos, "- ")?;
                            pos = appendBytes(out, pos, ANSI_RESET)?;
                            pos = appendBytes(out, pos, ANSI_BOLD)?;
                            pos = appendBytes(out, pos, pkg.bin_names[b])?;
                            pos = appendBytes(out, pos, ANSI_RESET)?;
                            pos = appendBytes(out, pos, "\n")?;
                        } else {
                            pos = appendBytes(out, pos, " - ")?;
                            pos = appendBytes(out, pos, pkg.bin_names[b])?;
                            pos = appendBytes(out, pos, "\n")?;
                        }
                        stats.bin_count += 1;
                        b += 1;
                    }
                } else {
                    if (opts.ansi_colors) {
                        pos = appendBytes(out, pos, ANSI_RESET)?;
                        pos = appendBytes(out, pos, ANSI_GREEN)?;
                        pos = appendBytes(out, pos, "installed")?;
                        pos = appendBytes(out, pos, ANSI_RESET)?;
                        pos = appendBytes(out, pos, " ")?;
                        pos = appendBytes(out, pos, ANSI_BOLD)?;
                        pos = appendBytes(out, pos, dep.dep_name)?;
                        pos = appendBytes(out, pos, ANSI_RESET)?;
                        pos = appendBytes(out, pos, ANSI_DIM)?;
                        pos = appendBytes(out, pos, "@")?;
                        pos = appendBytes(out, pos, pkg.version)?;
                        pos = appendBytes(out, pos, ANSI_RESET)?;
                        pos = appendBytes(out, pos, "\n")?;
                    } else {
                        pos = appendBytes(out, pos, "installed ")?;
                        pos = appendBytes(out, pos, dep.dep_name)?;
                        pos = appendBytes(out, pos, "@")?;
                        pos = appendBytes(out, pos, pkg.version)?;
                        pos = appendBytes(out, pos, "\n")?;
                    }
                }
            }
            u += 1;
        }

        if (printed_explicit) {
            pos = appendBytes(out, pos, "\n")?;
        }

        stats.bytes_written = pos;
        return stats;
    }
}
