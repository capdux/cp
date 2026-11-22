// Source: Absorbed from /data/dev/bun/src/shell/builtin/which.cp | Synced: 2026-11-22T12:49:17Z
// Ported from Bun: /data/dev/bun/src/shell/builtin/which.cp & /data/dev/bun/docs/runtime/shell.mdx
// Synced: 2026-11-22
// Pure-CP zero-allocation POSIX/Windows `which` Command Resolver & PATH Scanner:
// - Zero-copy PathEnvIter splitting PATH on ':' (POSIX) or ';' (Windows) with empty-segment '.' normalization
// - Direct path detection (relative/absolute commands bypass PATH and resolve against cwd)
// - Pluggable WhichProbeFn callback for deterministic zero-I/O testing and native OS integration
// - Multi-argument WhichPlan & exit-code evaluation matching Bun shell/builtin/which.cp semantics.

/// 单次 `which` 命令支持的最大参数个数
pub const SHELL_WHICH_MAX_ARGS: usize = 32;

/// 单条解析输出路径的最大字节长度
pub const SHELL_WHICH_MAX_PATH_LEN: usize = 512;

/// `which` 解析与格式化错误集
pub err ShellWhichError {
    TooManyArgs,
    InvalidCommand,
    BufferTooSmall,
}

/// 目标路径搜索平台规则
pub enum WhichPlatform: u8 {
    Posix = 0,
    Windows = 1,
}

/// 可执行文件探测回调函数类型（命中可执行常规文件返回 `true`）
pub type WhichProbeFn = fn(candidate_path: []const u8): bool;

fn strEq(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) {
        if (a[i] != b[i]) return false;
        i += 1;
    }
    return true;
}

fn isPathSep(c: u8, platform: WhichPlatform): bool {
    if (platform == .Posix) return c == '/';
    return c == '/' || c == '\\';
}

fn hasNulOrNewline(s: []const u8): bool {
    let i: usize = 0;
    while (i < s.len) {
        if (s[i] == 0 || s[i] == '\n' || s[i] == '\r') return true;
        i += 1;
    }
    return false;
}

/// 零拷贝 `PATH` 环境变量目录迭代器（遵循 POSIX 空段等价于 `"."` 规则）
pub struct PathEnvIter {
    raw: []const u8 = "",
    pos: usize = 0,
    done: bool = false,
    platform: WhichPlatform = .Posix,

    static fn init(path_env: []const u8, platform: WhichPlatform): PathEnvIter {
        return PathEnvIter{
            raw: path_env,
            pos: 0,
            done: path_env.len == 0,
            platform: platform,
        };
    }

    fn next(*_): ([]const u8)? {
        if (_.done) return null;
        const delim: u8 = if (_.platform == .Windows) ';' else ':';
        const start = _.pos;
        let i = start;
        while (i < _.raw.len && _.raw[i] != delim) {
            i += 1;
        }
        const seg = _.raw[start..i];
        if (i < _.raw.len) {
            _.pos = i + 1;
        } else {
            _.done = true;
        }
        if (seg.len == 0) return ".";
        return seg;
    }
}

/// 单条命令的 `which` 解析结果（零动态堆分配）
pub struct WhichResolveOutcome {
    found: bool = false,
    path_buf: [SHELL_WHICH_MAX_PATH_LEN]u8 = [0; SHELL_WHICH_MAX_PATH_LEN],
    path_len: usize = 0,

    fn path(): []const u8 {
        return _.path_buf[0.._.path_len];
    }
}

/// `which` 参数计划（支持 `-a`、`-s`、`--` 与多命令顺序解析）
pub struct WhichPlan {
    args: [SHELL_WHICH_MAX_ARGS]([]const u8) = [""; SHELL_WHICH_MAX_ARGS],
    arg_count: usize = 0,
    all_matches: bool = false,
    silent: bool = false,

    static fn parse(raw_args: []const ([]const u8)): WhichPlan! {
        let plan = WhichPlan{};
        let parsing_flags = true;
        let i: usize = 0;

        while (i < raw_args.len) {
            const arg = raw_args[i];
            if (parsing_flags && arg.len >= 2 && arg[0] == '-') {
                if (strEq(arg, "--")) {
                    parsing_flags = false;
                    i += 1;
                    continue;
                }
                let all_known = true;
                let c: usize = 1;
                while (c < arg.len) {
                    if (arg[c] != 'a' && arg[c] != 's') {
                        all_known = false;
                        break;
                    }
                    c += 1;
                }
                if (all_known) {
                    let k: usize = 1;
                    while (k < arg.len) {
                        if (arg[k] == 'a') plan.all_matches = true;
                        if (arg[k] == 's') plan.silent = true;
                        k += 1;
                    }
                    i += 1;
                    continue;
                }
            }

            if (plan.arg_count >= SHELL_WHICH_MAX_ARGS) {
                return err.TooManyArgs;
            }
            plan.args[plan.arg_count] = arg;
            plan.arg_count += 1;
            i += 1;
        }

        return plan;
    }

    fn argsSlice(): []const ([]const u8) {
        return _.args[0.._.arg_count];
    }

    fn isEmptyArgs(): bool {
        return _.arg_count == 0;
    }
}

/// 判断命令名是否直接包含路径分隔符（若是，则不扫描 `PATH`，直接按路径探测）
pub fn isDirectCommandPath(cmd: []const u8, platform: WhichPlatform): bool {
    let i: usize = 0;
    while (i < cmd.len) {
        if (isPathSep(cmd[i], platform)) return true;
        if (platform == .Windows && i == 1 && cmd[1] == ':') return true;
        i += 1;
    }
    return false;
}

fn isAbsolutePath(path: []const u8, platform: WhichPlatform): bool {
    if (path.len == 0) return false;
    if (isPathSep(path[0], platform)) return true;
    if (platform == .Windows && path.len >= 3 && path[1] == ':' && isPathSep(path[2], platform)) {
        return true;
    }
    return false;
}

fn copySlice(out: []u8, pos: *usize, s: []const u8): ! {
    if (pos.* + s.len > out.len) return err.BufferTooSmall;
    let i: usize = 0;
    while (i < s.len) {
        out[pos.* + i] = s[i];
        i += 1;
    }
    pos.* += s.len;
}

/// 将目录与命令名拼接为候选路径（自动去重冗余斜杠）
pub fn joinDirAndCommand(
    dir: []const u8,
    cmd: []const u8,
    platform: WhichPlatform,
    out: []u8,
): usize! {
    let pos: usize = 0;
    const sep: u8 = if (platform == .Windows) '\\' else '/';

    let d_end = dir.len;
    while (d_end > 1 && isPathSep(dir[d_end - 1], platform)) {
        d_end -= 1;
    }

    if (d_end > 0) {
        copySlice(out, &pos, dir[0..d_end])?;
        if (!isPathSep(dir[d_end - 1], platform)) {
            if (pos >= out.len) return err.BufferTooSmall;
            out[pos] = sep;
            pos += 1;
        }
    }
    copySlice(out, &pos, cmd)?;
    return pos;
}

/// 格式化未命中提示（对齐 Bun `shell/builtin/which.cp`：`"{cmd} not found\n"`）
pub fn formatWhichNotFound(cmd: []const u8, out: []u8): usize! {
    let pos: usize = 0;
    copySlice(out, &pos, cmd)?;
    copySlice(out, &pos, " not found\n")?;
    return pos;
}

/// 解析单个命令的可执行路径（零动态堆分配）
pub fn resolveWhichSingle(
    cmd: []const u8,
    path_env: []const u8,
    cwd: []const u8,
    platform: WhichPlatform,
    probe: WhichProbeFn,
): WhichResolveOutcome! {
    if (cmd.len == 0 || hasNulOrNewline(cmd)) {
        return err.InvalidCommand;
    }

    let outcome = WhichResolveOutcome{};

    // 1. 直接路径（含 '/' 或 Windows '\\' / 盘符）：跳过 PATH
    if (isDirectCommandPath(cmd, platform)) {
        let len: usize = 0;
        if (isAbsolutePath(cmd, platform) || cwd.len == 0) {
            let p: usize = 0;
            copySlice(outcome.path_buf[0..], &p, cmd)?;
            len = p;
        } else {
            len = joinDirAndCommand(cwd, cmd, platform, outcome.path_buf[0..])?;
        }
        if (probe(outcome.path_buf[0..len])) {
            outcome.found = true;
            outcome.path_len = len;
        }
        return outcome;
    }

    // 2. 依次扫描 PATH 中每个目录
    let iter = PathEnvIter.init(path_env, platform);
    while (dir := iter.next()) {
        const effective_dir = if (strEq(dir, ".") && cwd.len > 0) cwd else dir;
        const len = joinDirAndCommand(effective_dir, cmd, platform, outcome.path_buf[0..])?;
        if (probe(outcome.path_buf[0..len])) {
            outcome.found = true;
            outcome.path_len = len;
            return outcome;
        }
    }

    return outcome;
}

/// 评估整个 `WhichPlan` 的标准退出码（对齐 Bun `shell/builtin/which.cp`：空参或任一未命中返回 `1`，全部命中返回 `0`）
pub fn evaluateWhichExitCode(
    plan: WhichPlan,
    path_env: []const u8,
    cwd: []const u8,
    platform: WhichPlatform,
    probe: WhichProbeFn,
): u8! {
    if (plan.isEmptyArgs()) return 1;

    let had_not_found = false;
    let i: usize = 0;
    while (i < plan.arg_count) {
        const res = resolveWhichSingle(plan.args[i], path_env, cwd, platform, probe)?;
        if (!res.found) {
            had_not_found = true;
        }
        i += 1;
    }
    if (had_not_found) return 1;
    return 0;
}
