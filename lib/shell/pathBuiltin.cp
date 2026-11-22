// Source: Absorbed from /data/dev/bun/src/shell/builtin/basename.cp | Synced: 2026-11-22T16:52:27Z
// Pure-CP zero-allocation POSIX & Bun Shell `basename` / `dirname` builtin commands.
// Supports trailing-slash normalization, suffix stripping, `-a`/`-s`/`-z`/`--` CLI flags,
// and caller-buffered batch output with zero heap allocation.

pub const SHELL_PATH_BUILTIN_MAX_ARGS: usize = 64;

pub err PathBuiltinError {
    MissingOperand,
    ExtraOperand,
    InvalidOption,
    OutputTooSmall,
    TooManyArgs,
}

fn strEq(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) {
        if (a[i] != b[i]) return false;
        i += 1;
    }
    return true;
}

fn hasPrefix(s: []const u8, prefix: []const u8): bool {
    if (prefix.len > s.len) return false;
    let i: usize = 0;
    while (i < prefix.len) {
        if (s[i] != prefix[i]) return false;
        i += 1;
    }
    return true;
}

fn hasSuffix(s: []const u8, suffix: []const u8): bool {
    if (suffix.len > s.len) return false;
    const off = s.len - suffix.len;
    let i: usize = 0;
    while (i < suffix.len) {
        if (s[off + i] != suffix[i]) return false;
        i += 1;
    }
    return true;
}

fn appendBytes(out: []u8, pos: *usize, s: []const u8): ! {
    if (pos.* + s.len > out.len) return PathBuiltinError.OutputTooSmall;
    let i: usize = 0;
    while (i < s.len) {
        out[pos.* + i] = s[i];
        i += 1;
    }
    pos.* += s.len;
}

fn appendByte(out: []u8, pos: *usize, b: u8): ! {
    if (pos.* >= out.len) return PathBuiltinError.OutputTooSmall;
    out[pos.*] = b;
    pos.* += 1;
}

/// POSIX `basename` pure slice extraction (zero allocation):
/// - `""` -> `""`
/// - `"/"` or `"///"` -> `"/"`
/// - `"/usr/bin/"` -> `"bin"`
/// - `"foo.txt"` -> `"foo.txt"`
pub fn posixBasename(path: []const u8): []const u8 {
    if (path.len == 0) return "";
    let end: usize = path.len;
    while (end > 0 && path[end - 1] == '/') {
        end -= 1;
    }
    if (end == 0) return "/";

    let start: usize = end;
    while (start > 0 && path[start - 1] != '/') {
        start -= 1;
    }
    return path[start..end];
}

/// POSIX `basename` with optional suffix stripping.
/// Preserves the entire basename when `suffix` equals the whole basename.
pub fn posixBasenameStripSuffix(path: []const u8, suffix: []const u8): []const u8 {
    const base = posixBasename(path);
    if (suffix.len > 0 && base.len > suffix.len && hasSuffix(base, suffix)) {
        return base[0..base.len - suffix.len];
    }
    return base;
}

/// POSIX `dirname` pure slice extraction (zero allocation):
/// - `""` or `"foo"` -> `"."`
/// - `"/"` or `"///"` -> `"/"`
/// - `"/usr"` -> `"/"`
/// - `"/usr/bin/"` -> `"/usr"`
/// - `"a/b/c"` -> `"a/b"`
pub fn posixDirname(path: []const u8): []const u8 {
    if (path.len == 0) return ".";
    let end: usize = path.len;
    while (end > 0 && path[end - 1] == '/') {
        end -= 1;
    }
    if (end == 0) return "/";

    while (end > 0 && path[end - 1] != '/') {
        end -= 1;
    }
    if (end == 0) return ".";

    while (end > 1 && path[end - 1] == '/') {
        end -= 1;
    }
    return path[0..end];
}

pub struct BasenameOpts {
    multiple: bool = false,
    zero_terminated: bool = false,
    suffix: []const u8 = "",
    paths: [SHELL_PATH_BUILTIN_MAX_ARGS][]const u8 = [""; SHELL_PATH_BUILTIN_MAX_ARGS],
    path_count: usize = 0,

    static fn parse(args: []const []const u8): BasenameOpts! {
        let opts = BasenameOpts {};
        let i: usize = 0;
        let stop_opts = false;

        while (i < args.len) {
            const arg = args[i];
            if (!stop_opts && arg.len > 1 && arg[0] == '-') {
                if (strEq(arg, "--")) {
                    stop_opts = true;
                    i += 1;
                    continue;
                }
                if (strEq(arg, "-a") || strEq(arg, "--multiple")) {
                    opts.multiple = true;
                    i += 1;
                    continue;
                }
                if (strEq(arg, "-z") || strEq(arg, "--zero")) {
                    opts.zero_terminated = true;
                    i += 1;
                    continue;
                }
                if (strEq(arg, "-s")) {
                    if (i + 1 >= args.len) return PathBuiltinError.MissingOperand;
                    opts.suffix = args[i + 1];
                    opts.multiple = true;
                    i += 2;
                    continue;
                }
                if (hasPrefix(arg, "--suffix=")) {
                    opts.suffix = arg[9..];
                    opts.multiple = true;
                    i += 1;
                    continue;
                }
                return PathBuiltinError.InvalidOption;
            }
            if (opts.path_count >= SHELL_PATH_BUILTIN_MAX_ARGS) {
                return PathBuiltinError.TooManyArgs;
            }
            opts.paths[opts.path_count] = arg;
            opts.path_count += 1;
            i += 1;
        }

        if (opts.path_count == 0) return PathBuiltinError.MissingOperand;
        if (!opts.multiple && opts.path_count == 2) {
            opts.suffix = opts.paths[1];
            opts.path_count = 1;
        } else if (!opts.multiple && opts.path_count > 2) {
            return PathBuiltinError.ExtraOperand;
        }
        return opts;
    }
}

pub struct DirnameOpts {
    zero_terminated: bool = false,
    paths: [SHELL_PATH_BUILTIN_MAX_ARGS][]const u8 = [""; SHELL_PATH_BUILTIN_MAX_ARGS],
    path_count: usize = 0,

    static fn parse(args: []const []const u8): DirnameOpts! {
        let opts = DirnameOpts {};
        let i: usize = 0;
        let stop_opts = false;

        while (i < args.len) {
            const arg = args[i];
            if (!stop_opts && arg.len > 1 && arg[0] == '-') {
                if (strEq(arg, "--")) {
                    stop_opts = true;
                    i += 1;
                    continue;
                }
                if (strEq(arg, "-z") || strEq(arg, "--zero")) {
                    opts.zero_terminated = true;
                    i += 1;
                    continue;
                }
                return PathBuiltinError.InvalidOption;
            }
            if (opts.path_count >= SHELL_PATH_BUILTIN_MAX_ARGS) {
                return PathBuiltinError.TooManyArgs;
            }
            opts.paths[opts.path_count] = arg;
            opts.path_count += 1;
            i += 1;
        }

        if (opts.path_count == 0) return PathBuiltinError.MissingOperand;
        return opts;
    }
}

/// Executes the `basename` builtin into caller-provided `out`, returning bytes written.
pub fn runBasenameBuiltin(args: []const []const u8, out: []u8): usize! {
    const opts = BasenameOpts.parse(args)?;
    const delim: u8 = if (opts.zero_terminated) 0 else '\n';
    let pos: usize = 0;
    let i: usize = 0;
    while (i < opts.path_count) {
        const res = posixBasenameStripSuffix(opts.paths[i], opts.suffix);
        appendBytes(out, &pos, res)?;
        appendByte(out, &pos, delim)?;
        i += 1;
    }
    return pos;
}

/// Executes the `dirname` builtin into caller-provided `out`, returning bytes written.
pub fn runDirnameBuiltin(args: []const []const u8, out: []u8): usize! {
    const opts = DirnameOpts.parse(args)?;
    const delim: u8 = if (opts.zero_terminated) 0 else '\n';
    let pos: usize = 0;
    let i: usize = 0;
    while (i < opts.path_count) {
        const res = posixDirname(opts.paths[i]);
        appendBytes(out, &pos, res)?;
        appendByte(out, &pos, delim)?;
        i += 1;
    }
    return pos;
}
