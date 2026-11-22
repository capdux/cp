// Source: Absorbed from /data/dev/bun/src/shell/builtin/mv.cp | Synced: 2026-11-22T07:34:30Z
// Pure-CP zero-allocation POSIX `mv` flag parser (`-f`/`-i`/`-n`/`-h`/`-v`,
// combined short flags, last-flag-wins overwrite policy, `--` sentinel),
// multi-source target-directory validator, and destination path planner.

/// 单次 `mv` 命令支持的最大源路径数量
pub const SHELL_MV_MAX_SOURCES: usize = 64;

/// POSIX `mv` 标志解析与移动规划错误集
pub err ShellMvError {
    ShowUsage,
    IllegalOption,
    TooManySources,
    TargetMustBeDirectory,
    BufferTooSmall,
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

fn pathBasename(path: []const u8): []const u8 {
    let end = path.len;
    while (end > 1 && (path[end - 1] == '/' || path[end - 1] == '\\')) {
        end -= 1;
    }
    let i = end;
    while (i > 0) {
        i -= 1;
        if (path[i] == '/' || path[i] == '\\') {
            return path[i + 1..end];
        }
    }
    return path[0..end];
}

fn stripTrailingSlashes(path: []const u8): []const u8 {
    let end = path.len;
    while (end > 1 && (path[end - 1] == '/' || path[end - 1] == '\\')) {
        end -= 1;
    }
    return path[0..end];
}

/// POSIX `mv` 覆盖策略（`-f` / `-i` / `-n` 互斥，后出现覆盖先出现）
pub enum MvOverwritePolicy: u8 {
    /// `-f`（默认）：直接覆盖已存在目标
    Force = 0,
    /// `-i`：覆盖前交互提示
    Interactive = 1,
    /// `-n`：禁止覆盖已存在目标
    NoClobber = 2,
}

/// `mv` 解析后的标志选项集合（严格对齐 Bun `shell/builtin/mv.cp` `Opts`）
pub struct MvOpts {
    overwrite_policy: MvOverwritePolicy = .Force,
    no_dereference: bool = false,
    verbose: bool = false,

    fn forceOverwrite(): bool {
        return _.overwrite_policy == .Force;
    }

    fn interactiveMode(): bool {
        return _.overwrite_policy == .Interactive;
    }

    fn noOverwrite(): bool {
        return _.overwrite_policy == .NoClobber;
    }

    fn applyShortFlag(*_, ch: u8): bool {
        switch (ch) {
            'f' => {
                _.overwrite_policy = .Force;
                return true;
            },
            'h' => {
                _.no_dereference = true;
                return true;
            },
            'i' => {
                _.overwrite_policy = .Interactive;
                return true;
            },
            'n' => {
                _.overwrite_policy = .NoClobber;
                return true;
            },
            'v' => {
                _.verbose = true;
                return true;
            },
            else => return false,
        }
    }
}

/// 解析后的 `mv` 执行计划（零堆分配）
pub struct MvPlan {
    opts: MvOpts = MvOpts{},
    sources: [SHELL_MV_MAX_SOURCES][]const u8 = [""; SHELL_MV_MAX_SOURCES],
    source_count: usize = 0,
    target: []const u8 = "",

    /// 解析 `mv [OPTIONS] SOURCE... TARGET` 参数切片
    static fn parse(args: []const []const u8): MvPlan! {
        if (args.len == 0) {
            return err.ShowUsage;
        }

        let opts = MvOpts{};
        let idx: usize = 0;
        while (idx < args.len) {
            const arg = args[idx];
            if (arg.len == 0 || arg[0] != '-' || arg.len == 1) {
                break;
            }
            if (arg.len == 2 && arg[1] == '-') {
                idx += 1;
                break;
            }
            let k: usize = 1;
            while (k < arg.len) {
                if (!opts.applyShortFlag(arg[k])) {
                    return err.IllegalOption;
                }
                k += 1;
            }
            idx += 1;
        }

        const remaining = args.len - idx;
        if (remaining < 2) {
            return err.ShowUsage;
        }
        const src_count = remaining - 1;
        if (src_count > SHELL_MV_MAX_SOURCES) {
            return err.TooManySources;
        }

        let plan = MvPlan{
            opts: opts,
            sources: [""; SHELL_MV_MAX_SOURCES],
            source_count: src_count,
            target: args[args.len - 1],
        };
        let s: usize = 0;
        while (s < src_count) {
            plan.sources[s] = args[idx + s];
            s += 1;
        }
        return plan;
    }

    /// 当存在多个源路径时，目标路径必须是已存在目录
    fn requiresDirectoryTarget(): bool {
        return _.source_count > 1;
    }

    /// 校验并计算第 `src_idx` 个源路径对应的最终目标路径
    fn resolveDestPath(src_idx: usize, target_is_dir: bool, out: []u8): usize! {
        if (src_idx >= _.source_count) {
            return err.ShowUsage;
        }
        if (_.source_count > 1 && !target_is_dir) {
            return err.TargetMustBeDirectory;
        }
        if (!target_is_dir) {
            return appendBytes(out, 0, _.target);
        }
        const base_dir = stripTrailingSlashes(_.target);
        const src_base = pathBasename(_.sources[src_idx]);
        let pos = appendBytes(out, 0, base_dir)?;
        if (base_dir.len > 0 && base_dir[base_dir.len - 1] != '/') {
            pos = appendBytes(out, pos, "/")?;
        }
        return appendBytes(out, pos, src_base);
    }

    /// 格式化 `-v` 详细输出行（`'<src>' -> '<dst>'\n`）
    static fn formatVerboseLine(src: []const u8, dst: []const u8, out: []u8): usize! {
        let pos = appendBytes(out, 0, "'")?;
        pos = appendBytes(out, pos, src)?;
        pos = appendBytes(out, pos, "' -> '")?;
        pos = appendBytes(out, pos, dst)?;
        return appendBytes(out, pos, "'\n");
    }
}
