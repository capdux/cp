// Source: Absorbed from /data/dev/bun/src/ (Zero-Alloc NCSA Common & Combined HTTP Access Log Parser)

/// 单条 NCSA Common / Apache-Nginx Combined HTTP 访问日志记录（零拷贝切片）
pub struct ClfEntry {
    host: []const u8,
    ident: []const u8,
    user: []const u8,
    timestamp: []const u8,
    request_line: []const u8,
    method: []const u8,
    path: []const u8,
    protocol: []const u8,
    status: u16,
    bytes_sent: u64,
    has_bytes: bool,
    referer: []const u8,
    user_agent: []const u8,
    is_combined: bool,

    fn is2xx(): bool {
        return _.status >= 200 && _.status < 300;
    }

    fn is3xx(): bool {
        return _.status >= 300 && _.status < 400;
    }

    fn is4xx(): bool {
        return _.status >= 400 && _.status < 500;
    }

    fn is5xx(): bool {
        return _.status >= 500 && _.status < 600;
    }
}

/// 访问日志单遍流量与状态码统计汇总
pub struct ClfStats {
    total_requests: u32,
    status_2xx: u32,
    status_3xx: u32,
    status_4xx: u32,
    status_5xx: u32,
    total_bytes_sent: u64,
    combined_count: u32,
    malformed_lines: u32,
}

// ---------------------------------------------------------------------------
// 内部零分配词法扫描与数值解析辅助函数
// ---------------------------------------------------------------------------

fn isSpaceOrTab(c: u8): bool {
    return c == ' ' || c == '\t';
}

fn isDigit(c: u8): bool {
    return c >= '0' && c <= '9';
}

fn stripLineEnd(line: []const u8): []const u8 {
    let end = line.len;
    while (end > 0 && (line[end - 1] == '\r' || line[end - 1] == '\n')) {
        end -= 1;
    }
    return line[0..end];
}

fn skipSpaces(line: []const u8, pos: *usize) {
    while (pos.* < line.len && isSpaceOrTab(line[pos.*])) {
        pos.* += 1;
    }
}

fn nextToken(line: []const u8, pos: *usize): ([]const u8)? {
    skipSpaces(line, pos);
    if (pos.* >= line.len) {
        return null;
    }
    const start = pos.*;
    while (pos.* < line.len && !isSpaceOrTab(line[pos.*])) {
        pos.* += 1;
    }
    return line[start..pos.*];
}

/// 解析双引号字符串（支持 \" 与 \\ 转义），返回引号内部零拷贝切片并推进 pos 至闭合引号后
fn parseQuoted(line: []const u8, pos: *usize): ([]const u8)? {
    skipSpaces(line, pos);
    if (pos.* >= line.len || line[pos.*] != '"') {
        return null;
    }
    const start = pos.* + 1;
    let i = start;
    while (i < line.len) {
        if (line[i] == '\\') {
            if (i + 1 >= line.len) {
                return null;
            }
            i += 2;
            continue;
        }
        if (line[i] == '"') {
            pos.* = i + 1;
            return line[start..i];
        }
        i += 1;
    }
    return null;
}

/// 解析方括号时间戳 [10/Oct/2000:13:55:36 -0700]，返回括号内部切片
fn parseBracketed(line: []const u8, pos: *usize): ([]const u8)? {
    skipSpaces(line, pos);
    if (pos.* >= line.len || line[pos.*] != '[') {
        return null;
    }
    const start = pos.* + 1;
    let i = start;
    while (i < line.len && line[i] != ']') {
        i += 1;
    }
    if (i >= line.len || i == start) {
        return null;
    }
    pos.* = i + 1;
    return line[start..i];
}

fn parseU64Strict(s: []const u8): u64? {
    if (s.len == 0) {
        return null;
    }
    let acc: u64 = 0;
    let i: usize = 0;
    while (i < s.len) {
        if (!isDigit(s[i])) {
            return null;
        }
        const d = u64(s[i] - '0');
        // 溢出保护 (u64 max = 18446744073709551615)
        if (acc > 1844674407370955161) {
            return null;
        }
        acc = acc * 10 + d;
        i += 1;
    }
    return acc;
}

fn nextLine(data: []const u8, pos: *usize): ([]const u8)? {
    const start = pos.*;
    if (start >= data.len) {
        return null;
    }
    let i = start;
    while (i < data.len && data[i] != '\n') {
        i += 1;
    }
    let line_end = i;
    if (line_end > start && data[line_end - 1] == '\r') {
        line_end -= 1;
    }
    pos.* = if (i < data.len) (i + 1) else data.len;
    return data[start..line_end];
}

// ---------------------------------------------------------------------------
// 公开 API
// ---------------------------------------------------------------------------

/// 解析单条 NCSA Common 或 Apache/Nginx Combined 访问日志行
pub fn parseClfLine(raw_line: []const u8): ClfEntry? {
    const line = stripLineEnd(raw_line);
    let pos: usize = 0;

    // 1. host, ident, user
    const host = nextToken(line, *pos) ?? return null;
    const ident = nextToken(line, *pos) ?? return null;
    const user = nextToken(line, *pos) ?? return null;

    // 2. [timestamp]
    const timestamp = parseBracketed(line, *pos) ?? return null;

    // 3. "METHOD PATH PROTOCOL"
    const request_line = parseQuoted(line, *pos) ?? return null;

    // 4. 3 位 HTTP 状态码 (100..599)
    const status_tok = nextToken(line, *pos) ?? return null;
    if (status_tok.len != 3) {
        return null;
    }
    const status_u64 = parseU64Strict(status_tok) ?? return null;
    if (status_u64 < 100 || status_u64 > 599) {
        return null;
    }
    const status = status_u64 as u16;

    // 5. bytes_sent ("-" 表示 0 且 has_bytes = false)
    const bytes_tok = nextToken(line, *pos) ?? return null;
    let bytes_sent: u64 = 0;
    let has_bytes = false;
    if (bytes_tok.len == 1 && bytes_tok[0] == '-') {
        bytes_sent = 0;
        has_bytes = false;
    } else {
        bytes_sent = parseU64Strict(bytes_tok) ?? return null;
        has_bytes = true;
    }

    // 6. 自动拆分 request_line 为 method / path / protocol
    const empty = line[0..0];
    let method = empty;
    let path = empty;
    let protocol = empty;

    let rpos: usize = 0;
    if (m := nextToken(request_line, *rpos)) {
        method = m;
        skipSpaces(request_line, *rpos);
        if (rpos < request_line.len) {
            const rest_start = rpos;
            // 从末尾反向找最后一个空格以拆分 protocol（兼容 path 内极端空格场景）
            let last_sp = request_line.len;
            let k = request_line.len;
            while (k > rest_start) {
                k -= 1;
                if (isSpaceOrTab(request_line[k])) {
                    last_sp = k;
                    break;
                }
            }
            if (last_sp < request_line.len) {
                let path_end = last_sp;
                while (path_end > rest_start && isSpaceOrTab(request_line[path_end - 1])) {
                    path_end -= 1;
                }
                path = request_line[rest_start..path_end];
                protocol = request_line[last_sp + 1..request_line.len];
            } else {
                path = request_line[rest_start..request_line.len];
            }
        }
    }

    // 7. 可选 Combined 扩展："referer" "user-agent"
    let referer = empty;
    let user_agent = empty;
    let is_combined = false;

    skipSpaces(line, *pos);
    if (pos < line.len && line[pos] == '"') {
        const ref_val = parseQuoted(line, *pos) ?? return null;
        skipSpaces(line, *pos);
        if (pos < line.len && line[pos] == '"') {
            const ua_val = parseQuoted(line, *pos) ?? return null;
            referer = ref_val;
            user_agent = ua_val;
            is_combined = true;
        } else {
            return null;
        }
    }

    return ClfEntry{
        host: host,
        ident: ident,
        user: user,
        timestamp: timestamp,
        request_line: request_line,
        method: method,
        path: path,
        protocol: protocol,
        status: status,
        bytes_sent: bytes_sent,
        has_bytes: has_bytes,
        referer: referer,
        user_agent: user_agent,
        is_combined: is_combined,
    };
}

/// 快速检测缓冲区首个非空行是否为合法的 CLF/Combined 访问日志
pub fn isClf(data: []const u8): bool {
    let pos: usize = 0;
    while (line := nextLine(data, *pos)) {
        let p: usize = 0;
        skipSpaces(line, *p);
        if (p >= line.len) {
            continue;
        }
        return parseClfLine(line) != null;
    }
    return false;
}

/// 零分配逐行迭代器（跳过空行，兼容 \r\n/\n，畸形行递增 skipped_malformed 并继续）
pub struct ClfIter {
    data: []const u8,
    pos: usize,
    skipped_malformed: u32,

    fn init(data: []const u8): ClfIter {
        return ClfIter{
            data: data,
            pos: 0,
            skipped_malformed: 0,
        };
    }

    fn next(*_): ClfEntry? {
        while (line := nextLine(_.data, *_.pos)) {
            let p: usize = 0;
            skipSpaces(line, *p);
            if (p >= line.len) {
                continue;
            }
            if (entry := parseClfLine(line)) {
                return entry;
            }
            _.skipped_malformed += 1;
        }
        return null;
    }
}

/// 单遍统计访问日志的总请求数、各状态码分桶、总流量字节、Combined 条数与畸形行数
pub fn summarizeClf(data: []const u8): ClfStats {
    let stats = ClfStats{
        total_requests: 0,
        status_2xx: 0,
        status_3xx: 0,
        status_4xx: 0,
        status_5xx: 0,
        total_bytes_sent: 0,
        combined_count: 0,
        malformed_lines: 0,
    };

    let it = ClfIter.init(data);
    while (e := it.next()) {
        stats.total_requests += 1;
        if (e.is2xx()) {
            stats.status_2xx += 1;
        } else if (e.is3xx()) {
            stats.status_3xx += 1;
        } else if (e.is4xx()) {
            stats.status_4xx += 1;
        } else if (e.is5xx()) {
            stats.status_5xx += 1;
        }
        stats.total_bytes_sent += e.bytes_sent;
        if (e.is_combined) {
            stats.combined_count += 1;
        }
    }
    stats.malformed_lines = it.skipped_malformed;

    return stats;
}
