// Source: Absorbed from /data/dev/bun/src/shell/builtin/mkdir.cp | Synced: 2026-11-22T07:42:24Z
// Pure-CP zero-allocation POSIX `mkdir` flag parser (`-p`/`--parents`,
// `-v`/`--verbose`, `-m`/`--mode`, combined short flags, `--` terminator),
// octal permission mode parser, and zero-alloc `-p` parent-directory prefix iterator.

/// 默认新建目录权限掩码（POSIX 0o777，实际生效时由进程 umask 裁剪）
pub const DEFAULT_MKDIR_MODE: u32 = 0o777;

/// `mkdir` 参数与模式解析错误集
pub err MkdirParseError {
    MissingOperand,
    MissingModeValue,
    InvalidMode,
    IllegalOption,
}

/// 解析 1~4 位 POSIX 八进制权限位（如 `"755"`, `"0755"`, `"1777"`）
pub fn parseOctalMode(raw: string): u32! {
    if (raw.len == 0 || raw.len > 4) {
        return err.InvalidMode;
    }
    let val: u32 = 0;
    let i: usize = 0;
    while (i < raw.len) {
        const c = raw[i];
        if (c < '0' || c > '7') {
            return err.InvalidMode;
        }
        val = (val << 3) | @as(u32, c - '0');
        i += 1;
    }
    if (val > 0o7777) {
        return err.InvalidMode;
    }
    return val;
}

/// 解析后的 `mkdir` 选项配置与目录操作数起始索引（对齐 Bun `mkdir.Opts`）
pub struct MkdirOpts {
    mode: u32? = null,
    parents: bool = false,
    verbose: bool = false,
    paths_start: usize = 0,

    fn effectiveMode(): u32 {
        return _.mode ?? DEFAULT_MKDIR_MODE;
    }

    /// 解析 `mkdir` 命令行参数切片（不含命令名本身）
    static fn parse(args: []const string): MkdirOpts! {
        let opts = MkdirOpts{
            mode: null,
            parents: false,
            verbose: false,
            paths_start: 0,
        };

        let i: usize = 0;
        while (i < args.len) {
            const arg = args[i];
            if (arg.len < 2 || arg[0] != '-') {
                break;
            }
            if (std.mem.eql(u8, arg, "--")) {
                i += 1;
                break;
            }

            if (arg[1] == '-') {
                // Long option
                if (std.mem.eql(u8, arg, "--parents")) {
                    opts.parents = true;
                } else if (std.mem.eql(u8, arg, "--verbose")) {
                    opts.verbose = true;
                } else if (std.mem.eql(u8, arg, "--mode")) {
                    i += 1;
                    if (i >= args.len) {
                        return err.MissingModeValue;
                    }
                    opts.mode = parseOctalMode(args[i])?;
                } else if (arg.len >= 7 && std.mem.eql(u8, arg[0..7], "--mode=")) {
                    opts.mode = parseOctalMode(arg[7..])?;
                } else {
                    return err.IllegalOption;
                }
                i += 1;
                continue;
            }

            // Short option cluster: -p, -v, -pv, -m 0755, -m0755, -pm750
            let k: usize = 1;
            while (k < arg.len) {
                const c = arg[k];
                if (c == 'p') {
                    opts.parents = true;
                    k += 1;
                } else if (c == 'v') {
                    opts.verbose = true;
                    k += 1;
                } else if (c == 'm') {
                    if (k + 1 < arg.len) {
                        opts.mode = parseOctalMode(arg[k + 1 ..])?;
                    } else {
                        i += 1;
                        if (i >= args.len) {
                            return err.MissingModeValue;
                        }
                        opts.mode = parseOctalMode(args[i])?;
                    }
                    break;
                } else {
                    return err.IllegalOption;
                }
            }
            i += 1;
        }

        if (i >= args.len) {
            return err.MissingOperand;
        }
        opts.paths_start = i;
        return opts;
    }
}

/// 零堆分配逐级父目录前缀迭代器（用于 `mkdir -p` 顺序建目录规划）
/// 例如 `"/a/b/c"` -> `"/a"`, `"/a/b"`, `"/a/b/c"`；`"x//y/"` -> `"x"`, `"x//y"`
pub struct ParentDirIter {
    path: string,
    pos: usize = 0,
    trimmed_len: usize = 0,

    static fn init(path: string): ParentDirIter {
        let end = path.len;
        // Trim trailing slashes while preserving root "/"
        while (end > 1 && path[end - 1] == '/') {
            end -= 1;
        }
        let start: usize = 0;
        // Skip leading slashes so first segment starts at first component
        while (start < end && path[start] == '/') {
            start += 1;
        }
        return ParentDirIter{
            path: path,
            pos: start,
            trimmed_len: end,
        };
    }

    fn next(*_): string? {
        if (_.trimmed_len == 0 || _.pos >= _.trimmed_len) {
            return null;
        }
        // Advance through current component
        while (_.pos < _.trimmed_len && _.path[_.pos] != '/') {
            _.pos += 1;
        }
        const prefix = _.path[0.._.pos];
        // Skip any consecutive slashes before next component
        while (_.pos < _.trimmed_len && _.path[_.pos] == '/') {
            _.pos += 1;
        }
        return prefix;
    }
}
