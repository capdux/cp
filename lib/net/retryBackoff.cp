// Source: Absorbed from /data/dev/bun/src/http/HTTPContext.cp | Synced: 2026-11-22T11:12:13Z (Exponential Backoff & RFC 9110 Retry-After)

/// 指数退避随机抖动策略
export enum JitterMode {
    none,
    full,
    equal,
}

/// 指数退避调度配置
export struct BackoffConfig {
    base_delay_ms: u64;
    max_delay_ms: u64;
    max_attempts: u32;
    jitter: JitterMode;

    /// 返回生产推荐默认配置（base=100ms, max=30s, max_attempts=5, full jitter）
    static fn defaultConfig(): BackoffConfig {
        return BackoffConfig{
            base_delay_ms: 100,
            max_delay_ms: 30_000,
            max_attempts: 5,
            jitter: JitterMode.full,
        };
    }
}

/// 确定性指数退避状态机（内置 SplitMix64 PRNG，零堆分配）
export struct BackoffState {
    attempt: u32;
    rng_state: u64;

    /// 使用给定种子初始化退避状态（相同种子保证确定性可复现）
    static fn init(seed: u64): BackoffState {
        return BackoffState{
            attempt: 0,
            rng_state: seed,
        };
    }

    /// 重置重试计数（保留当前 PRNG 状态）
    fn reset(*_) {
        _.attempt = 0;
    }

    /// 是否已达到最大重试次数上限
    fn isExhausted(cfg: BackoffConfig): bool {
        return _.attempt >= cfg.max_attempts;
    }

    fn nextRandom(*_): u64 {
        _.rng_state +%= 0x9E37_79B9_7F4A_7C15;
        let z: u64 = _.rng_state;
        z = (z ^ (z >> 30)) *% 0xBF58_476D_1CE4_E5B9;
        z = (z ^ (z >> 27)) *% 0x94D0_49BB_1331_11EB;
        return z ^ (z >> 31);
    }

    /// 计算下一次退避等待毫秒数；超过 max_attempts 时返回 null
    fn nextDelayMs(*_, cfg: BackoffConfig): u64? {
        if (_.attempt >= cfg.max_attempts) {
            return null;
        }

        // 防溢出饱和计算 min(base_delay_ms * 2^attempt, max_delay_ms)
        let cap: u64 = if (cfg.base_delay_ms > cfg.max_delay_ms) cfg.max_delay_ms else cfg.base_delay_ms;
        let step: u32 = 0;
        while (step < _.attempt && cap < cfg.max_delay_ms) {
            if (cap > cfg.max_delay_ms / 2) {
                cap = cfg.max_delay_ms;
                break;
            }
            cap = cap * 2;
            step += 1;
        }

        _.attempt += 1;

        return switch (cfg.jitter) {
            .none => cap,
            .full => blk: {
                if (cap == 0) break :blk u64(0);
                const r = _.nextRandom();
                if (cap == 0xFFFF_FFFF_FFFF_FFFF) break :blk r;
                break :blk r % (cap + 1);
            },
            .equal => blk: {
                if (cap == 0) break :blk u64(0);
                const half = cap / 2;
                const span = cap - half;
                const r = _.nextRandom();
                const offset = if (span == 0xFFFF_FFFF_FFFF_FFFF) r else (r % (span + 1));
                break :blk half + offset;
            },
        };
    }
}

/// 解析后的 RFC 9110 Retry-After 头部结果
export struct RetryAfter {
    delay_secs: u64;
    is_http_date: bool;
}

fn isSpace(c: u8): bool {
    return c == ' ' || c == '\t' || c == '\r' || c == '\n';
}

fn trimAscii(s: []const u8): []const u8 {
    let start: usize = 0;
    let end: usize = s.len;
    while (start < end && isSpace(s[start])) {
        start += 1;
    }
    while (end > start && isSpace(s[end - 1])) {
        end -= 1;
    }
    return s[start..end];
}

fn strEql(a: []const u8, b: []const u8): bool {
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

fn parseFixedDigits(s: []const u8): u32? {
    if (s.len == 0) {
        return null;
    }
    let val: u32 = 0;
    for (c of s) {
        if (c < '0' || c > '9') {
            return null;
        }
        val = val * 10 + u32(c - '0');
    }
    return val;
}

fn parseU64Decimal(s: []const u8): u64? {
    if (s.len == 0) {
        return null;
    }
    let val: u64 = 0;
    for (c of s) {
        if (c < '0' || c > '9') {
            return null;
        }
        const digit: u64 = u64(c - '0');
        if (val > (0xFFFF_FFFF_FFFF_FFFF - digit) / 10) {
            return null;
        }
        val = val * 10 + digit;
    }
    return val;
}

fn isValidWeekday(s: []const u8): bool {
    return strEql(s, "Mon")
        || strEql(s, "Tue")
        || strEql(s, "Wed")
        || strEql(s, "Thu")
        || strEql(s, "Fri")
        || strEql(s, "Sat")
        || strEql(s, "Sun");
}

fn parseMonth(s: []const u8): u32? {
    if (strEql(s, "Jan")) return 1;
    if (strEql(s, "Feb")) return 2;
    if (strEql(s, "Mar")) return 3;
    if (strEql(s, "Apr")) return 4;
    if (strEql(s, "May")) return 5;
    if (strEql(s, "Jun")) return 6;
    if (strEql(s, "Jul")) return 7;
    if (strEql(s, "Aug")) return 8;
    if (strEql(s, "Sep")) return 9;
    if (strEql(s, "Oct")) return 10;
    if (strEql(s, "Nov")) return 11;
    if (strEql(s, "Dec")) return 12;
    return null;
}

fn isLeapYear(year: u32): bool {
    return (year % 4 == 0 && year % 100 != 0) || (year % 400 == 0);
}

fn daysInMonth(year: u32, month: u32): u32 {
    return switch (month) {
        1, 3, 5, 7, 8, 10, 12 => 31,
        4, 6, 9, 11 => 30,
        2 => if (isLeapYear(year)) 29 else 28,
        else => 0,
    };
}

/// Howard Hinnant 算法：将格里高利历 (year, month, day) 转为自 1970-01-01 起的 UTC 天数
fn daysFromCivil(year_in: u32, month_in: u32, day_in: u32): i64 {
    let y: i64 = i64(year_in);
    const m: i64 = i64(month_in);
    const d: i64 = i64(day_in);
    if (m <= 2) {
        y -= 1;
    }
    const era: i64 = @divFloor(y, 400);
    const yoe: i64 = y - era * 400;
    const mp: i64 = if (m > 2) (m - 3) else (m + 9);
    const doy: i64 = @divTrunc(153 * mp + 2, 5) + d - 1;
    const doe: i64 = yoe * 365 + @divTrunc(yoe, 4) - @divTrunc(yoe, 100) + doy;
    return era * 146097 + doe - 719468;
}

/// 解析 RFC 9110 §10.2.3 `Retry-After` 头部（支持十进制秒数与 IMF-fixdate；过去时间钳制为 0 秒）
export fn parseRetryAfter(header: []const u8, now_epoch_sec: i64): RetryAfter? {
    const s = trimAscii(header);
    if (s.len == 0) {
        return null;
    }

    // 1. 纯十进制非负秒数（如 "120"）
    if (secs := parseU64Decimal(s)) {
        return RetryAfter{
            delay_secs: secs,
            is_http_date: false,
        };
    }

    // 2. RFC 9110 IMF-fixdate（固定 29 字节：e.g. "Wed, 21 Oct 2015 07:28:00 GMT"）
    if (s.len != 29) {
        return null;
    }
    if (s[3] != ',' || s[4] != ' ' || s[7] != ' ' || s[11] != ' '
        || s[16] != ' ' || s[19] != ':' || s[22] != ':' || s[25] != ' ')
    {
        return null;
    }
    if (!isValidWeekday(s[0..3]) || !strEql(s[26..29], "GMT")) {
        return null;
    }

    const day = parseFixedDigits(s[5..7]) ?? return null;
    const month = parseMonth(s[8..11]) ?? return null;
    const year = parseFixedDigits(s[12..16]) ?? return null;
    const hour = parseFixedDigits(s[17..19]) ?? return null;
    const minute = parseFixedDigits(s[20..22]) ?? return null;
    const second = parseFixedDigits(s[23..25]) ?? return null;

    if (year < 1970 || day < 1 || day > daysInMonth(year, month)) {
        return null;
    }
    if (hour > 23 || minute > 59 || second > 59) {
        return null;
    }

    const days = daysFromCivil(year, month, day);
    const target_epoch: i64 = days * 86400 + i64(hour) * 3600 + i64(minute) * 60 + i64(second);
    const delay: u64 = if (target_epoch <= now_epoch_sec) 0 else u64(target_epoch - now_epoch_sec);
    return RetryAfter{
        delay_secs: delay,
        is_http_date: true,
    };
}

/// 判断 HTTP 状态码是否属于标准瞬态可重试错误（408/425/429/500/502/503/504）
export fn isRetryableHttpStatus(status: u16): bool {
    return switch (status) {
        408, 425, 429, 500, 502, 503, 504 => true,
        else => false,
    };
}
