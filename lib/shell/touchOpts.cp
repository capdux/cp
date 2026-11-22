// Source: Absorbed from /data/dev/bun/src/shell/builtin/touch.cp | Synced: 2026-11-22T08:04:12Z
// Pure-CP zero-allocation POSIX `touch` option parser (`-a`, `-c`/`--no-create`,
// `-m`, `-h`/`--no-dereference`, `-d`/`--date`, `-r`/`--reference`, `-t`, `--time`,
// combined short flags, `--` terminator) and POSIX `[[CC]YY]MMDDhhmm[.ss]` timestamp parser/formatter.

/// 单次 `touch` 调用支持的最大文件路径操作数数量
pub const SHELL_TOUCH_MAX_PATHS: usize = 64;

/// `touch` 选项与 POSIX 时间戳解析错误集
pub err ShellTouchError {
    MissingOperand,
    MissingOptionValue,
    IllegalOption,
    InvalidTimeWord,
    InvalidTimestamp,
    ConflictingTimeSource,
    TooManyPaths,
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

fn startsWith(s: []const u8, prefix: []const u8): bool {
    if (s.len < prefix.len) {
        return false;
    }
    return bytesEql(s[0..prefix.len], prefix);
}

fn parseTwoDigits(s: []const u8): u8? {
    if (s.len != 2) {
        return null;
    }
    if (s[0] < '0' || s[0] > '9' || s[1] < '0' || s[1] > '9') {
        return null;
    }
    return ((s[0] - '0') * 10) + (s[1] - '0');
}

fn isLeapYear(year: u16): bool {
    return (year % 4 == 0 && year % 100 != 0) || (year % 400 == 0);
}

fn daysInMonth(year: u16, has_year: bool, month: u8): u8 {
    if (month == 1 || month == 3 || month == 5 || month == 7 || month == 8 || month == 10 || month == 12) {
        return 31;
    }
    if (month == 4 || month == 6 || month == 9 || month == 11) {
        return 30;
    }
    if (month == 2) {
        if (!has_year) {
            return 29;
        }
        return if (isLeapYear(year)) 29 else 28;
    }
    return 0;
}

fn writeByte(out: []u8, pos: usize, b: u8): usize! {
    if (pos >= out.len) {
        return err.BufferTooSmall;
    }
    out[pos] = b;
    return pos + 1;
}

fn writeTwoDigits(out: []u8, start: usize, val: u8): usize! {
    let pos = writeByte(out, start, ('0' as u8) + (val / 10))?;
    pos = writeByte(out, pos, ('0' as u8) + (val % 10))?;
    return pos;
}

fn writeFourDigits(out: []u8, start: usize, val: u16): usize! {
    let pos = writeTwoDigits(out, start, (val / 100) as u8)?;
    pos = writeTwoDigits(out, pos, (val % 100) as u8)?;
    return pos;
}

/// `touch` 时间更新目标（访问时间 / 修改时间 / 两者同时）
pub enum TouchTimeTarget: u8 {
    both = 0,
    access_only = 1,
    modify_only = 2,
}

/// POSIX `touch -t [[CC]YY]MMDDhhmm[.ss]` 解析后的日历时间戳
pub struct TouchTimestamp {
    /// 是否显式指定了年份（8 位 `MMDDhhmm` 时为 `false`）
    has_year: bool,
    /// 公历年份（10 位 `YY`: `69..99 => 1969..1999`, `00..68 => 2000..2068`；无年份时为 `0`）
    year: u16,
    month: u8,
    day: u8,
    hour: u8,
    minute: u8,
    /// 秒数（0..60，POSIX 允许 60 表示闰秒）
    second: u8,

    /// 解析 POSIX `[[CC]YY]MMDDhhmm[.ss]` 时间戳并执行完整日历/闰年校验
    static fn parse(raw: []const u8): TouchTimestamp! {
        if (raw.len < 8) {
            return err.InvalidTimestamp;
        }

        // 查找可选 `.ss` 后缀
        let main_part = raw;
        let second: u8 = 0;
        let dot_pos: usize = raw.len;
        let i: usize = 0;
        while (i < raw.len) {
            if (raw[i] == '.') {
                if (dot_pos != raw.len) {
                    return err.InvalidTimestamp;
                }
                dot_pos = i;
            }
            i += 1;
        }
        if (dot_pos != raw.len) {
            main_part = raw[0..dot_pos];
            const sec_str = raw[dot_pos + 1..];
            if (s := parseTwoDigits(sec_str)) {
                if (s > 60) {
                    return err.InvalidTimestamp;
                }
                second = s;
            } else {
                return err.InvalidTimestamp;
            }
        }

        let has_year = false;
        let year: u16 = 0;
        let offset: usize = 0;

        if (main_part.len == 8) {
            has_year = false;
            year = 0;
            offset = 0;
        } else if (main_part.len == 10) {
            if (yy := parseTwoDigits(main_part[0..2])) {
                has_year = true;
                year = if (yy >= 69) ((1900 as u16) + (yy as u16)) else ((2000 as u16) + (yy as u16));
                offset = 2;
            } else {
                return err.InvalidTimestamp;
            }
        } else if (main_part.len == 12) {
            const cc_opt = parseTwoDigits(main_part[0..2]);
            const yy_opt = parseTwoDigits(main_part[2..4]);
            if (cc := cc_opt) {
                if (yy := yy_opt) {
                    has_year = true;
                    year = (cc as u16) * 100 + (yy as u16);
                    offset = 4;
                } else {
                    return err.InvalidTimestamp;
                }
            } else {
                return err.InvalidTimestamp;
            }
        } else {
            return err.InvalidTimestamp;
        }

        const mo_opt = parseTwoDigits(main_part[offset..offset + 2]);
        const dy_opt = parseTwoDigits(main_part[offset + 2..offset + 4]);
        const hr_opt = parseTwoDigits(main_part[offset + 4..offset + 6]);
        const mn_opt = parseTwoDigits(main_part[offset + 6..offset + 8]);

        if (month := mo_opt) {
            if (day := dy_opt) {
                if (hour := hr_opt) {
                    if (minute := mn_opt) {
                        if (month < 1 || month > 12) {
                            return err.InvalidTimestamp;
                        }
                        const max_day = daysInMonth(year, has_year, month);
                        if (day < 1 || day > max_day) {
                            return err.InvalidTimestamp;
                        }
                        if (hour > 23 || minute > 59) {
                            return err.InvalidTimestamp;
                        }
                        return TouchTimestamp{
                            has_year: has_year,
                            year: year,
                            month: month,
                            day: day,
                            hour: hour,
                            minute: minute,
                            second: second,
                        };
                    }
                }
            }
        }
        return err.InvalidTimestamp;
    }

    /// 语义等价比较
    fn eql(other: TouchTimestamp): bool {
        return _.has_year == other.has_year &&
            _.year == other.year &&
            _.month == other.month &&
            _.day == other.day &&
            _.hour == other.hour &&
            _.minute == other.minute &&
            _.second == other.second;
    }

    /// 格式化为标准 POSIX 字符串（`CCYYMMDDhhmm.ss` 或 `MMDDhhmm.ss`，支持 Round-Trip 幂等）
    fn formatPosix(out: []u8): usize! {
        let pos: usize = 0;
        if (_.has_year) {
            pos = writeFourDigits(out, pos, _.year)?;
        }
        pos = writeTwoDigits(out, pos, _.month)?;
        pos = writeTwoDigits(out, pos, _.day)?;
        pos = writeTwoDigits(out, pos, _.hour)?;
        pos = writeTwoDigits(out, pos, _.minute)?;
        pos = writeByte(out, pos, '.')?;
        pos = writeTwoDigits(out, pos, _.second)?;
        return pos;
    }
}

fn applyTimeWord(opts: *TouchOpts, word: []const u8): usize! {
    if (bytesEql(word, "access") || bytesEql(word, "atime") || bytesEql(word, "use")) {
        opts.access_time_only = true;
        return 1;
    }
    if (bytesEql(word, "modify") || bytesEql(word, "mtime")) {
        opts.modification_time_only = true;
        return 2;
    }
    return err.InvalidTimeWord;
}

/// 解析后的 `touch` 命令行选项与文件路径列表（对齐 Bun `touch.Opts`）
pub struct TouchOpts {
    access_time_only: bool,
    modification_time_only: bool,
    no_create: bool,
    no_dereference: bool,
    date: []const u8,
    has_date: bool,
    reference: []const u8,
    has_reference: bool,
    timestamp_raw: []const u8,
    timestamp: TouchTimestamp?,
    paths: [SHELL_TOUCH_MAX_PATHS][]const u8,
    path_count: usize,

    /// 初始化默认选项
    static fn defaults(): TouchOpts {
        return TouchOpts{
            access_time_only: false,
            modification_time_only: false,
            no_create: false,
            no_dereference: false,
            date: "",
            has_date: false,
            reference: "",
            has_reference: false,
            timestamp_raw: "",
            timestamp: null,
            paths: [""; SHELL_TOUCH_MAX_PATHS],
            path_count: 0,
        };
    }

    /// 解析 `touch` 命令行参数切片（不含命令名本身）
    static fn parse(args: []const []const u8): TouchOpts! {
        let opts = TouchOpts.defaults();
        let i: usize = 0;
        let stop_flags = false;

        while (i < args.len) {
            const arg = args[i];
            if (stop_flags || arg.len < 2 || arg[0] != '-') {
                if (opts.path_count >= SHELL_TOUCH_MAX_PATHS) {
                    return err.TooManyPaths;
                }
                opts.paths[opts.path_count] = arg;
                opts.path_count += 1;
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
                if (bytesEql(arg, "--no-create")) {
                    opts.no_create = true;
                } else if (bytesEql(arg, "--no-dereference")) {
                    opts.no_dereference = true;
                } else if (bytesEql(arg, "--date")) {
                    i += 1;
                    if (i >= args.len) return err.MissingOptionValue;
                    opts.date = args[i];
                    opts.has_date = true;
                } else if (startsWith(arg, "--date=")) {
                    opts.date = arg[7..];
                    opts.has_date = true;
                } else if (bytesEql(arg, "--reference")) {
                    i += 1;
                    if (i >= args.len) return err.MissingOptionValue;
                    opts.reference = args[i];
                    opts.has_reference = true;
                } else if (startsWith(arg, "--reference=")) {
                    opts.reference = arg[12..];
                    opts.has_reference = true;
                } else if (bytesEql(arg, "--time")) {
                    i += 1;
                    if (i >= args.len) return err.MissingOptionValue;
                    * = applyTimeWord(&opts, args[i])?;
                } else if (startsWith(arg, "--time=")) {
                    * = applyTimeWord(&opts, arg[7..])?;
                } else {
                    return err.IllegalOption;
                }
                i += 1;
                continue;
            }

            // Short option cluster: -a, -c, -m, -h, -d <val>, -r <val>, -t <val>, -acm, -ct202601010000
            let j: usize = 1;
            while (j < arg.len) {
                const ch = arg[j];
                if (ch == 'a') {
                    opts.access_time_only = true;
                    j += 1;
                } else if (ch == 'c') {
                    opts.no_create = true;
                    j += 1;
                } else if (ch == 'm') {
                    opts.modification_time_only = true;
                    j += 1;
                } else if (ch == 'h') {
                    opts.no_dereference = true;
                    j += 1;
                } else if (ch == 'd' || ch == 'r' || ch == 't') {
                    let val: []const u8 = "";
                    if (j + 1 < arg.len) {
                        val = arg[j + 1..];
                    } else {
                        i += 1;
                        if (i >= args.len) {
                            return err.MissingOptionValue;
                        }
                        val = args[i];
                    }
                    if (ch == 'd') {
                        opts.date = val;
                        opts.has_date = true;
                    } else if (ch == 'r') {
                        opts.reference = val;
                        opts.has_reference = true;
                    } else {
                        const parsed_ts = TouchTimestamp.parse(val)?;
                        opts.timestamp_raw = val;
                        opts.timestamp = parsed_ts;
                    }
                    break;
                } else {
                    return err.IllegalOption;
                }
            }
            i += 1;
        }

        // 互斥时间源校验：-d / -r / -t 最多只能指定一个
        let time_sources: usize = 0;
        if (opts.has_date) time_sources += 1;
        if (opts.has_reference) time_sources += 1;
        if (opts.timestamp != null) time_sources += 1;
        if (time_sources > 1) {
            return err.ConflictingTimeSource;
        }

        if (opts.path_count == 0) {
            return err.MissingOperand;
        }
        return opts;
    }

    /// 计算最终生效的时间更新目标（`-a` 与 `-m` 同设或均未设时为 `.both`）
    fn effectiveTarget(): TouchTimeTarget {
        if (_.access_time_only && !_.modification_time_only) {
            return .access_only;
        }
        if (_.modification_time_only && !_.access_time_only) {
            return .modify_only;
        }
        return .both;
    }

    /// 当目标文件不存在时是否应创建空文件
    fn shouldCreateMissing(): bool {
        return !_.no_create;
    }

    /// 按索引读取目标文件路径
    fn pathAt(idx: usize): ([]const u8)? {
        if (idx >= _.path_count) {
            return null;
        }
        return _.paths[idx];
    }
}
