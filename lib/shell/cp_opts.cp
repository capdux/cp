// Source: Absorbed from /data/dev/bun/src/shell/builtin/cp.cp | Synced: 2026-11-22T08:08:00Z
// Pure-CP zero-allocation POSIX `cp` option parser (`-r`/`-R`/`--recursive`, `-f`/`-i`/`-n`,
// `-p`/`--preserve`, `-v`/`--verbose`, `-a`/`--archive`, `-H`/`-L`/`-P` symlink policies,
// combined short flags, `--` terminator) and copy target planner with self-copy/cycle guards.

/// 单次 `cp` 调用支持的最大源路径数量
pub const SHELL_CP_MAX_SOURCES: usize = 64;
const SHELL_CP_MAX_POSITIONALS: usize = 65;

/// `cp` 选项解析与目标规划错误集
pub err ShellCpError {
    MissingSourceOperand,
    MissingDestinationOperand,
    IllegalOption,
    TooManySources,
    DestinationMustBeDir,
    OmittingDirectory,
    CannotCopyIntoSelf,
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

fn stripTrailingSlashes(p: []const u8): []const u8 {
    let end = p.len;
    while (end > 1 && p[end - 1] == '/') {
        end -= 1;
    }
    return p[0..end];
}

fn pathBasename(p: []const u8): []const u8 {
    const trimmed = stripTrailingSlashes(p);
    if (bytesEql(trimmed, "/")) {
        return "/";
    }
    let i = trimmed.len;
    while (i > 0) {
        i -= 1;
        if (trimmed[i] == '/') {
            return trimmed[i + 1..];
        }
    }
    return trimmed;
}

fn hasTrailingSlash(p: []const u8): bool {
    return p.len > 1 && p[p.len - 1] == '/';
}

/// 判定 `target` 是否与 `src` 相同或是 `src` 的子路径（拦截自拷贝与目录递归拷贝到自身）
fn isSameOrSubpath(src: []const u8, target: []const u8): bool {
    const s = stripTrailingSlashes(src);
    const t = stripTrailingSlashes(target);
    if (bytesEql(s, t)) {
        return true;
    }
    if (t.len > s.len && bytesEql(t[0..s.len], s)) {
        if (bytesEql(s, "/") || t[s.len] == '/') {
            return true;
        }
    }
    return false;
}

fn writeBytes(out: []u8, pos: usize, src: []const u8): usize! {
    if (pos + src.len > out.len) {
        return err.BufferTooSmall;
    }
    let i: usize = 0;
    while (i < src.len) {
        out[pos + i] = src[i];
        i += 1;
    }
    return pos + src.len;
}

fn writeByte(out: []u8, pos: usize, b: u8): usize! {
    if (pos >= out.len) {
        return err.BufferTooSmall;
    }
    out[pos] = b;
    return pos + 1;
}

/// `cp` 目标文件覆盖策略（POSIX 后出现的选项覆盖前者）
pub enum CpOverwritePolicy: u8 {
    force = 0,
    interactive = 1,
    no_clobber = 2,
}

/// `cp` 符号链接跟随策略（`-H` / `-L` / `-P`）
pub enum CpSymlinkPolicy: u8 {
    follow_default = 0,
    follow_cmdline = 1,
    follow_all = 2,
    no_dereference = 3,
}

/// 解析后的 `cp` 命令行选项、源列表与目标路径（对齐 Bun `cp.Opts`）
pub struct CpOpts {
    recursive: bool,
    preserve: bool,
    verbose: bool,
    overwrite: CpOverwritePolicy,
    symlink_policy: CpSymlinkPolicy,
    sources: [SHELL_CP_MAX_SOURCES][]const u8,
    source_count: usize,
    destination: []const u8,

    /// 初始化默认选项
    static fn defaults(): CpOpts {
        return CpOpts{
            recursive: false,
            preserve: false,
            verbose: false,
            overwrite: .force,
            symlink_policy: .follow_default,
            sources: [""; SHELL_CP_MAX_SOURCES],
            source_count: 0,
            destination: "",
        };
    }

    /// 解析 `cp` 命令行参数切片（不含命令名本身）
    static fn parse(args: []const []const u8): CpOpts! {
        let opts = CpOpts.defaults();
        let positionals: [SHELL_CP_MAX_POSITIONALS][]const u8 = [""; SHELL_CP_MAX_POSITIONALS];
        let pos_count: usize = 0;

        let i: usize = 0;
        let stop_flags = false;
        while (i < args.len) {
            const arg = args[i];
            if (stop_flags || arg.len < 2 || arg[0] != '-') {
                if (pos_count >= SHELL_CP_MAX_POSITIONALS) {
                    return err.TooManySources;
                }
                positionals[pos_count] = arg;
                pos_count += 1;
                i += 1;
                continue;
            }

            if (bytesEql(arg, "--")) {
                stop_flags = true;
                i += 1;
                continue;
            }

            if (arg[1] == '-') {
                // Long options
                if (bytesEql(arg, "--recursive")) {
                    opts.recursive = true;
                } else if (bytesEql(arg, "--force")) {
                    opts.overwrite = .force;
                } else if (bytesEql(arg, "--interactive")) {
                    opts.overwrite = .interactive;
                } else if (bytesEql(arg, "--no-clobber")) {
                    opts.overwrite = .no_clobber;
                } else if (bytesEql(arg, "--preserve")) {
                    opts.preserve = true;
                } else if (bytesEql(arg, "--verbose")) {
                    opts.verbose = true;
                } else if (bytesEql(arg, "--archive")) {
                    // POSIX/GNU: -a 等价于 -RPp
                    opts.recursive = true;
                    opts.preserve = true;
                    opts.symlink_policy = .no_dereference;
                } else if (bytesEql(arg, "--dereference")) {
                    opts.symlink_policy = .follow_all;
                } else if (bytesEql(arg, "--no-dereference")) {
                    opts.symlink_policy = .no_dereference;
                } else {
                    return err.IllegalOption;
                }
                i += 1;
                continue;
            }

            // Short option cluster
            let j: usize = 1;
            while (j < arg.len) {
                const ch = arg[j];
                if (ch == 'r' || ch == 'R') {
                    opts.recursive = true;
                } else if (ch == 'f') {
                    opts.overwrite = .force;
                } else if (ch == 'i') {
                    opts.overwrite = .interactive;
                } else if (ch == 'n') {
                    opts.overwrite = .no_clobber;
                } else if (ch == 'p') {
                    opts.preserve = true;
                } else if (ch == 'v') {
                    opts.verbose = true;
                } else if (ch == 'a') {
                    opts.recursive = true;
                    opts.preserve = true;
                    opts.symlink_policy = .no_dereference;
                } else if (ch == 'H') {
                    opts.symlink_policy = .follow_cmdline;
                } else if (ch == 'L') {
                    opts.symlink_policy = .follow_all;
                } else if (ch == 'P') {
                    opts.symlink_policy = .no_dereference;
                } else {
                    return err.IllegalOption;
                }
                j += 1;
            }
            i += 1;
        }

        if (pos_count == 0) {
            return err.MissingSourceOperand;
        }
        if (pos_count == 1) {
            return err.MissingDestinationOperand;
        }

        const src_n = pos_count - 1;
        let k: usize = 0;
        while (k < src_n) {
            opts.sources[k] = positionals[k];
            k += 1;
        }
        opts.source_count = src_n;
        opts.destination = positionals[pos_count - 1];
        return opts;
    }

    /// 按索引读取源路径
    fn sourceAt(idx: usize): ([]const u8)? {
        if (idx >= _.source_count) {
            return null;
        }
        return _.sources[idx];
    }

    /// 按 POSIX 规范计算某节点是否应跟随符号链接
    fn effectiveFollowSymlink(is_cmdline_arg: bool): bool {
        if (_.symlink_policy == .follow_all) {
            return true;
        }
        if (_.symlink_policy == .no_dereference) {
            return false;
        }
        if (_.symlink_policy == .follow_cmdline) {
            return is_cmdline_arg;
        }
        // follow_default: 非递归默认跟随，递归默认不跟随
        return !_.recursive;
    }

    /// 是否允许直接覆盖已存在目标（仅 `.force` 返回 `true`）
    fn allowsOverwrite(): bool {
        return _.overwrite == .force;
    }
}

/// 零堆分配 `cp` 目标路径规划与自拷贝防线
pub struct CpPlan {
    opts: CpOpts,
    dest_is_dir: bool,

    /// 构造复制计划并校验多源/尾斜杠目标必须为目录
    static fn build(opts: CpOpts, dest_is_dir: bool): CpPlan! {
        if (opts.source_count == 0) {
            return err.MissingSourceOperand;
        }
        if (opts.destination.len == 0) {
            return err.MissingDestinationOperand;
        }
        if (opts.source_count > 1 && !dest_is_dir) {
            return err.DestinationMustBeDir;
        }
        if (hasTrailingSlash(opts.destination) && !dest_is_dir) {
            return err.DestinationMustBeDir;
        }
        return CpPlan{
            opts: opts,
            dest_is_dir: dest_is_dir,
        };
    }

    /// 解析第 `src_idx` 个源的最终落盘目标路径，并执行目录递归要求与自拷贝/子目录环路拦截
    fn resolveTargetFor(src_idx: usize, src_is_dir: bool, out: []u8): usize! {
        if (src_idx >= _.opts.source_count) {
            return err.MissingSourceOperand;
        }
        if (src_is_dir && !_.opts.recursive) {
            return err.OmittingDirectory;
        }

        const src = _.opts.sources[src_idx];
        let pos: usize = 0;

        if (_.dest_is_dir) {
            const base = pathBasename(src);
            const clean_dest = stripTrailingSlashes(_.opts.destination);
            pos = writeBytes(out, 0, clean_dest)?;
            if (!bytesEql(clean_dest, "/")) {
                pos = writeByte(out, pos, '/')?;
            }
            pos = writeBytes(out, pos, base)?;
        } else {
            pos = writeBytes(out, 0, _.opts.destination)?;
        }

        // 拦截自拷贝（cp a a）或目录递归拷贝到自身子目录（cp -r dir dir/sub）
        if (isSameOrSubpath(src, out[0..pos])) {
            return err.CannotCopyIntoSelf;
        }
        return pos;
    }
}
