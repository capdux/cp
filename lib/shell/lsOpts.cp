// Source: Absorbed from /data/dev/bun/src/shell/builtin/ls.cp | Synced: 2026-11-22T09:26:00Z
// Ported from Bun: /data/dev/bun/src/shell/builtin/ls.cp & /data/dev/bun/docs/runtime/shell.mdx
// Synced: 2026-11-22
// Pure-CP zero-allocation POSIX/GNU `ls` Option Parser, Entry Sorter & Formatter:
// - Parses bundled short flags (-a/-A/-B/-d/-f/-F/-h/-l/-p/-r/-R/-S/-t/-U/-1), long flags & `--` sentinel
// - Filters hidden dotfiles, `.`/`..`, and backup (`~`) files per POSIX/GNU rules
// - Formats 10-char POSIX permission strings (including setuid/setgid/sticky bits) & integer human-readable sizes
// - In-place zero-allocation stable sorter supporting directory-first grouping, size/mtime/name & reverse ordering.

/// 单次 `ls` 命令支持的最大路径操作数数量
pub const SHELL_LS_MAX_PATHS: usize = 32;

/// `ls` 选项解析与格式化错误集
pub err ShellLsError {
    IllegalOption,
    TooManyPaths,
    BufferTooSmall,
}

/// `ls` 排序键类别
pub enum LsSortKey: u8 {
    Name = 0,
    Size = 1,
    Mtime = 2,
    None = 3,
}

/// 文件系统条目类型（用于权限首字符、分类后缀与目录优先分组）
pub enum LsEntryKind: u8 {
    File = 0,
    Directory = 1,
    Symlink = 2,
    Fifo = 3,
    Socket = 4,
    CharDevice = 5,
    BlockDevice = 6,
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

fn strCmp(a: []const u8, b: []const u8): i32 {
    let min_len = a.len;
    if (b.len < min_len) min_len = b.len;
    let i: usize = 0;
    while (i < min_len) {
        if (a[i] < b[i]) return -1;
        if (a[i] > b[i]) return 1;
        i += 1;
    }
    if (a.len < b.len) return -1;
    if (a.len > b.len) return 1;
    return 0;
}

/// `ls` 命令行选项集合（对齐 Bun `shell/builtin/ls.cp` `Opts`）
pub struct LsOpts {
    show_all: bool = false,
    show_almost_all: bool = false,
    ignore_backups: bool = false,
    list_directories: bool = false,
    classify: bool = false,
    slash_indicator: bool = false,
    human_readable: bool = false,
    long_listing: bool = false,
    one_per_line: bool = false,
    reverse_order: bool = false,
    recursive: bool = false,
    group_directories_first: bool = false,
    sort_key: LsSortKey = .Name,

    /// 判断目录项名称在当前选项下是否应当显示
    fn shouldIncludeName(name: []const u8): bool {
        if (name.len == 0) return false;
        if (strEq(name, ".") || strEq(name, "..")) {
            return _.show_all;
        }
        if (name[0] == '.') {
            if (!_.show_all && !_.show_almost_all) return false;
        }
        if (_.ignore_backups && name[name.len - 1] == '~') {
            return false;
        }
        return true;
    }
}

/// `ls` 解析后的执行计划（零动态堆分配）
pub struct LsPlan {
    opts: LsOpts = LsOpts{},
    paths: [SHELL_LS_MAX_PATHS]([]const u8) = [""; SHELL_LS_MAX_PATHS],
    path_count: usize = 0,

    static fn parse(args: []const ([]const u8)): LsPlan! {
        let plan = LsPlan{};
        let parsing_flags = true;
        let idx: usize = 0;

        while (idx < args.len) {
            const arg = args[idx];
            if (parsing_flags && arg.len >= 2 && arg[0] == '-') {
                if (strEq(arg, "--")) {
                    parsing_flags = false;
                    idx += 1;
                    continue;
                }
                if (arg[1] == '-') {
                    if (strEq(arg, "--all")) {
                        plan.opts.show_all = true;
                    } else if (strEq(arg, "--almost-all")) {
                        plan.opts.show_almost_all = true;
                    } else if (strEq(arg, "--ignore-backups")) {
                        plan.opts.ignore_backups = true;
                    } else if (strEq(arg, "--directory")) {
                        plan.opts.list_directories = true;
                    } else if (strEq(arg, "--classify")) {
                        plan.opts.classify = true;
                    } else if (strEq(arg, "--human-readable")) {
                        plan.opts.human_readable = true;
                    } else if (strEq(arg, "--reverse")) {
                        plan.opts.reverse_order = true;
                    } else if (strEq(arg, "--recursive")) {
                        plan.opts.recursive = true;
                    } else if (strEq(arg, "--group-directories-first")) {
                        plan.opts.group_directories_first = true;
                    } else {
                        return err.IllegalOption;
                    }
                    idx += 1;
                    continue;
                }

                let c: usize = 1;
                while (c < arg.len) {
                    const ch = arg[c];
                    if (ch == 'a') {
                        plan.opts.show_all = true;
                    } else if (ch == 'A') {
                        plan.opts.show_almost_all = true;
                    } else if (ch == 'B') {
                        plan.opts.ignore_backups = true;
                    } else if (ch == 'd') {
                        plan.opts.list_directories = true;
                    } else if (ch == 'f') {
                        plan.opts.show_all = true;
                        plan.opts.sort_key = .None;
                    } else if (ch == 'F') {
                        plan.opts.classify = true;
                    } else if (ch == 'h') {
                        plan.opts.human_readable = true;
                    } else if (ch == 'l') {
                        plan.opts.long_listing = true;
                        plan.opts.one_per_line = false;
                    } else if (ch == '1') {
                        plan.opts.one_per_line = true;
                        plan.opts.long_listing = false;
                    } else if (ch == 'p') {
                        plan.opts.slash_indicator = true;
                    } else if (ch == 'r') {
                        plan.opts.reverse_order = true;
                    } else if (ch == 'R') {
                        plan.opts.recursive = true;
                    } else if (ch == 'S') {
                        plan.opts.sort_key = .Size;
                    } else if (ch == 't') {
                        plan.opts.sort_key = .Mtime;
                    } else if (ch == 'U') {
                        plan.opts.sort_key = .None;
                    } else {
                        return err.IllegalOption;
                    }
                    c += 1;
                }
                idx += 1;
                continue;
            }

            if (plan.path_count >= SHELL_LS_MAX_PATHS) {
                return err.TooManyPaths;
            }
            plan.paths[plan.path_count] = arg;
            plan.path_count += 1;
            idx += 1;
        }

        // 无路径参数时默认列出当前工作目录 "."
        if (plan.path_count == 0) {
            plan.paths[0] = ".";
            plan.path_count = 1;
        }
        return plan;
    }

    fn pathsSlice(): []const ([]const u8) {
        return _.paths[0.._.path_count];
    }
}

/// 待排序与格式化的目录项元数据
pub struct LsEntryMeta {
    name: []const u8 = "",
    kind: LsEntryKind = .File,
    mode: u32 = 0o644,
    size: u64 = 0,
    mtime_sec: i64 = 0,

    fn isExecutable(): bool {
        return _.kind == .File && (_.mode & 0o111) != 0;
    }

    /// 按 `-F` / `-p` 规则返回条目类型指示后缀字符
    fn indicatorChar(opts: LsOpts): u8? {
        if (opts.classify) {
            if (_.kind == .Directory) return '/';
            if (_.kind == .Symlink) return '@';
            if (_.kind == .Fifo) return '|';
            if (_.kind == .Socket) return '=';
            if (_.isExecutable()) return '*';
            return null;
        }
        if (opts.slash_indicator && _.kind == .Directory) {
            return '/';
        }
        return null;
    }
}

/// 将文件类型与 POSIX 权限位格式化为标准 10 字节权限串（如 `drwxr-xr-x`）
pub fn formatLsMode(kind: LsEntryKind, mode: u32, out: []u8): usize! {
    if (out.len < 10) return err.BufferTooSmall;

    let type_ch: u8 = '-';
    if (kind == .Directory) {
        type_ch = 'd';
    } else if (kind == .Symlink) {
        type_ch = 'l';
    } else if (kind == .Fifo) {
        type_ch = 'p';
    } else if (kind == .Socket) {
        type_ch = 's';
    } else if (kind == .CharDevice) {
        type_ch = 'c';
    } else if (kind == .BlockDevice) {
        type_ch = 'b';
    }
    out[0] = type_ch;

    out[1] = if ((mode & 0o400) != 0) 'r' else '-';
    out[2] = if ((mode & 0o200) != 0) 'w' else '-';
    if ((mode & 0o4000) != 0) {
        out[3] = if ((mode & 0o100) != 0) 's' else 'S';
    } else {
        out[3] = if ((mode & 0o100) != 0) 'x' else '-';
    }

    out[4] = if ((mode & 0o040) != 0) 'r' else '-';
    out[5] = if ((mode & 0o020) != 0) 'w' else '-';
    if ((mode & 0o2000) != 0) {
        out[6] = if ((mode & 0o010) != 0) 's' else 'S';
    } else {
        out[6] = if ((mode & 0o010) != 0) 'x' else '-';
    }

    out[7] = if ((mode & 0o004) != 0) 'r' else '-';
    out[8] = if ((mode & 0o002) != 0) 'w' else '-';
    if ((mode & 0o1000) != 0) {
        out[9] = if ((mode & 0o001) != 0) 't' else 'T';
    } else {
        out[9] = if ((mode & 0o001) != 0) 'x' else '-';
    }

    return 10;
}

fn pushU64(val: u64, out: []u8, pos: *usize): ! {
    let digits: [20]u8 = [0; 20];
    let d_len: usize = 0;
    let n = val;
    if (n == 0) {
        digits[0] = '0';
        d_len = 1;
    } else {
        while (n > 0) {
            const rem: u8 = @intCast(n % 10);
            digits[d_len] = (48 as u8) + rem;
            d_len += 1;
            n = n / 10;
        }
    }
    if (pos.* + d_len > out.len) return err.BufferTooSmall;
    let k: usize = d_len;
    while (k > 0) {
        k -= 1;
        out[pos.*] = digits[k];
        pos.* += 1;
    }
}

/// 纯整数格式化人类可读字节数（对齐 `ls -h`：`512` -> `"512"`, `1536` -> `"1.5K"`, `1048576` -> `"1.0M"`）
pub fn formatLsHumanSize(bytes: u64, out: []u8): usize! {
    let pos: usize = 0;
    if (bytes < 1024) {
        pushU64(bytes, out, &pos)?;
        return pos;
    }

    const units: [5]u8 = ['K', 'M', 'G', 'T', 'P'];
    let unit_idx: usize = 0;
    let divisor: u64 = 1024;
    while (unit_idx + 1 < units.len && bytes >= divisor * 1024) {
        divisor = divisor * 1024;
        unit_idx += 1;
    }

    const whole = bytes / divisor;
    const rem = bytes % divisor;
    // 对 < 10 的值显示 1 位小数（四舍五入至十分位），>= 10 显示整数
    if (whole < 10) {
        let tenths = (rem * 10 + (divisor / 2)) / divisor;
        let adj_whole = whole;
        if (tenths >= 10) {
            adj_whole += 1;
            tenths = 0;
        }
        pushU64(adj_whole, out, &pos)?;
        if (adj_whole < 10) {
            if (pos + 2 > out.len) return err.BufferTooSmall;
            out[pos] = '.';
            pos += 1;
            const t_digit: u8 = @intCast(tenths);
            out[pos] = (48 as u8) + t_digit;
            pos += 1;
        }
    } else {
        let rounded = whole;
        if (rem * 2 >= divisor) rounded += 1;
        pushU64(rounded, out, &pos)?;
    }

    if (pos >= out.len) return err.BufferTooSmall;
    out[pos] = units[unit_idx];
    pos += 1;
    return pos;
}

fn entryShouldComeBefore(a: LsEntryMeta, b: LsEntryMeta, opts: LsOpts): bool {
    if (opts.group_directories_first) {
        const a_dir = a.kind == .Directory;
        const b_dir = b.kind == .Directory;
        if (a_dir != b_dir) return a_dir;
    }

    let cmp: i32 = 0;
    if (opts.sort_key == .Size) {
        // 默认大文件在前，同大小按名称升序
        if (a.size > b.size) {
            cmp = -1;
        } else if (a.size < b.size) {
            cmp = 1;
        } else {
            cmp = strCmp(a.name, b.name);
        }
    } else if (opts.sort_key == .Mtime) {
        // 默认最新修改在前，同时间按名称升序
        if (a.mtime_sec > b.mtime_sec) {
            cmp = -1;
        } else if (a.mtime_sec < b.mtime_sec) {
            cmp = 1;
        } else {
            cmp = strCmp(a.name, b.name);
        }
    } else {
        cmp = strCmp(a.name, b.name);
    }

    if (opts.reverse_order) {
        return cmp > 0;
    }
    return cmp < 0;
}

/// 零堆分配原地稳定排序目录项切片（支持 `--group-directories-first`、`-S`、`-t`、`-r`、`-f`/`-U`）
pub fn sortLsEntries(entries: []LsEntryMeta, opts: LsOpts) {
    if (opts.sort_key == .None || entries.len <= 1) return;
    let i: usize = 1;
    while (i < entries.len) {
        const key = entries[i];
        let j: usize = i;
        while (j > 0 && entryShouldComeBefore(key, entries[j - 1], opts)) {
            entries[j] = entries[j - 1];
            j -= 1;
        }
        entries[j] = key;
        i += 1;
    }
}
