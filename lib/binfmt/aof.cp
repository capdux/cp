// Origin: Pure-CP Stdlib (Zero-Alloc Redis AOF RESP Command Log Parser) | Created: 2026-11-22

/// 零拷贝 AOF 命令参数迭代器（支持任意含 `\r\n` 与 `\0` 的二进制安全 Bulk String）
pub struct AofArgIter {
    data: []const u8,
    pos: usize = 0,
    remaining: usize = 0,

    static fn init(raw_args: []const u8, arg_count: usize): AofArgIter {
        return AofArgIter {
            data: raw_args,
            pos: 0,
            remaining: arg_count,
        };
    }

    /// 读取下一条零拷贝参数切片；到达末尾或遇到格式异常时返回 `null`
    fn next(*_): ([]const u8)? {
        if (_.remaining == 0 || _.pos >= _.data.len) return null;
        if (_.data[_.pos] != '$') {
            _.remaining = 0;
            return null;
        }
        const crlf_opt = findCrlf(_.data, _.pos + 1);
        if (crlf_opt == null) {
            _.remaining = 0;
            return null;
        }
        const crlf = crlf_opt ?? 0;

        const len_opt = parseUint(_.data[_.pos + 1 .. crlf]);
        if (len_opt == null) {
            _.remaining = 0;
            return null;
        }
        const len_u64 = len_opt ?? 0;
        const len: usize = len_u64 as usize;
        const payload_start: usize = crlf + 2;
        if (len > _.data.len - payload_start) {
            _.remaining = 0;
            return null;
        }
        const payload_end: usize = payload_start + len;
        if (_.data.len - payload_end < 2 || _.data[payload_end] != '\r' || _.data[payload_end + 1] != '\n') {
            _.remaining = 0;
            return null;
        }
        _.pos = payload_end + 2;
        _.remaining -= 1;
        return _.data[payload_start..payload_end];
    }
}

/// 零拷贝单条 Redis AOF 命令视图（含 Redis 7.0 `#TS:` 时间戳关联）
pub struct AofCommand {
    arg_count: usize,
    raw_args_slice: []const u8,
    timestamp: i64?,
    byte_offset: usize,
    byte_len: usize,

    /// 创建参数零拷贝迭代器（含命令名与全部参数）
    fn args(): AofArgIter {
        return AofArgIter.init(_.raw_args_slice, _.arg_count);
    }

    /// 返回命令名（首个参数，如 `"SET"`, `"SELECT"`, `"HSET"`）
    fn name(): ([]const u8)? {
        let it = _.args();
        return it.next();
    }

    /// 按下标返回第 `idx` 个参数（`0` 为命令名）
    fn argAt(idx: usize): ([]const u8)? {
        if (idx >= _.arg_count) return null;
        let it = _.args();
        let i: usize = 0;
        while (arg := it.next()) {
            if (i == idx) return arg;
            i += 1;
        }
        return null;
    }
}

/// 零堆分配 Redis AOF 流式解析迭代器（支持 `#TS:` 注释与截断尾安全断点）
pub struct AofIter {
    data: []const u8,
    pos: usize = 0,
    had_error: bool = false,
    pending_ts: i64? = null,

    static fn init(data: []const u8): AofIter {
        return AofIter {
            data: data,
            pos: 0,
            had_error: false,
            pending_ts: null,
        };
    }

    /// 返回已成功校验并消费的安全字节偏移（可用于截断尾部修复）
    fn consumedBytes(): usize {
        return _.pos;
    }

    /// 解析下一条完整 RESP AOF 命令；到达流尾或遇到截断/畸形命令时返回 `null`
    fn next(*_): AofCommand? {
        if (_.had_error) return null;

        // 跳过空行与 '#' 注释行（提取 Redis 7.0 #TS:<unix_sec> 时间戳）
        while (_.pos < _.data.len) {
            const b = _.data[_.pos];
            if (b == '\r') {
                if (_.pos + 1 < _.data.len && _.data[_.pos + 1] == '\n') {
                    _.pos += 2;
                    continue;
                }
                _.had_error = true;
                return null;
            }
            if (b == '\n') {
                _.pos += 1;
                continue;
            }
            if (b == '#') {
                const lf_opt = findLf(_.data, _.pos);
                if (lf_opt == null) {
                    _.had_error = true;
                    return null;
                }
                const lf = lf_opt ?? 0;
                const line_end: usize = if (lf > _.pos && _.data[lf - 1] == '\r') lf - 1 else lf;
                const line = _.data[_.pos..line_end];
                if (line.len > 4
                    && line[0] == '#'
                    && line[1] == 'T'
                    && line[2] == 'S'
                    && line[3] == ':') {
                    if (ts := parseI64(line[4..line.len])) {
                        _.pending_ts = ts;
                    }
                }
                _.pos = lf + 1;
                continue;
            }
            break;
        }

        if (_.pos >= _.data.len) return null;

        const cmd_start = _.pos;
        if (_.data[cmd_start] != '*') {
            _.had_error = true;
            return null;
        }

        const hdr_crlf_opt = findCrlf(_.data, cmd_start + 1);
        if (hdr_crlf_opt == null) {
            _.had_error = true;
            return null;
        }
        const hdr_crlf = hdr_crlf_opt ?? 0;

        const argc_opt = parseUint(_.data[cmd_start + 1 .. hdr_crlf]);
        if (argc_opt == null) {
            _.had_error = true;
            return null;
        }
        const argc: usize = (argc_opt ?? 0) as usize;

        const args_start: usize = hdr_crlf + 2;
        let cur: usize = args_start;
        let i: usize = 0;

        while (i < argc) {
            if (cur >= _.data.len || _.data[cur] != '$') {
                _.had_error = true;
                return null;
            }
            const arg_crlf_opt = findCrlf(_.data, cur + 1);
            if (arg_crlf_opt == null) {
                _.had_error = true;
                return null;
            }
            const arg_crlf = arg_crlf_opt ?? 0;

            const arg_len_opt = parseUint(_.data[cur + 1 .. arg_crlf]);
            if (arg_len_opt == null) {
                _.had_error = true;
                return null;
            }
            const arg_len: usize = (arg_len_opt ?? 0) as usize;
            const p_start: usize = arg_crlf + 2;
            if (arg_len > _.data.len - p_start) {
                _.had_error = true;
                return null;
            }
            const p_end: usize = p_start + arg_len;
            if (_.data.len - p_end < 2 || _.data[p_end] != '\r' || _.data[p_end + 1] != '\n') {
                _.had_error = true;
                return null;
            }
            cur = p_end + 2;
            i += 1;
        }

        const ts = _.pending_ts;
        _.pending_ts = null;
        _.pos = cur;

        return AofCommand {
            arg_count: argc,
            raw_args_slice: _.data[args_start..cur],
            timestamp: ts,
            byte_offset: cmd_start,
            byte_len: cur - cmd_start,
        };
    }
}

/// 单遍统计的 Redis AOF 汇总结果
pub struct AofStats {
    command_count: u64,
    arg_count: u64,
    consumed_bytes: usize,
    truncated_or_invalid: bool,
}

fn findLf(data: []const u8, start: usize): usize? {
    let i = start;
    while (i < data.len) {
        if (data[i] == '\n') return i;
        i += 1;
    }
    return null;
}

fn findCrlf(data: []const u8, start: usize): usize? {
    let i = start;
    while (i + 1 < data.len) {
        if (data[i] == '\r' && data[i + 1] == '\n') return i;
        i += 1;
    }
    return null;
}

fn parseUint(s: []const u8): u64? {
    if (s.len == 0) return null;
    let acc: u64 = 0;
    let i: usize = 0;
    while (i < s.len) {
        const ch = s[i];
        if (ch < '0' || ch > '9') return null;
        if (acc > 1844674407370955161) return null;
        acc = acc * 10 + ((ch - '0') as u64);
        i += 1;
    }
    return acc;
}

fn parseI64(s: []const u8): i64? {
    if (s.len == 0) return null;
    let neg: bool = false;
    let start: usize = 0;
    if (s[0] == '-') {
        neg = true;
        start = 1;
    } else if (s[0] == '+') {
        start = 1;
    }
    if (start >= s.len) return null;
    const u = parseUint(s[start..s.len]) ?? return null;
    if (neg) {
        if (u > 9223372036854775808) return null;
        if (u == 9223372036854775808) return -9223372036854775807 - 1;
        return -(u as i64);
    }
    if (u > 9223372036854775807) return null;
    return u as i64;
}

/// 检测 AOF 文件是否以 Redis 混合 RDB+AOF 的 `"REDIS"` 前导开头
pub fn isAofMixedRdb(data: []const u8): bool {
    if (data.len < 5) return false;
    return data[0] == 'R'
        && data[1] == 'E'
        && data[2] == 'D'
        && data[3] == 'I'
        && data[4] == 'S';
}

/// 检测缓冲区是否以 RESP Multi-Bulk 数组 `'*'` 或 Redis 7.0 AOF 注释 `'#'` 开头
pub fn isAofResp(data: []const u8): bool {
    if (data.len == 0) return false;
    return data[0] == '*' || data[0] == '#';
}

/// 单遍汇总 AOF 日志的命令总数、参数总数、安全消费偏移及是否含截断尾
pub fn summarizeAof(data: []const u8): AofStats {
    let it = AofIter.init(data);
    let cmds: u64 = 0;
    let args: u64 = 0;
    while (cmd := it.next()) {
        cmds += 1;
        args += cmd.arg_count as u64;
    }
    return AofStats {
        command_count: cmds,
        arg_count: args,
        consumed_bytes: it.consumedBytes(),
        truncated_or_invalid: it.had_error,
    };
}
