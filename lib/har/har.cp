// Origin: Pure-CP Stdlib (Zero-Alloc W3C HTTP Archive .har 1.2 Parser) | Created: 2026-11-22

/// W3C HAR 1.2 头部元数据与零拷贝 entries 数组切片
pub struct HarHeader {
    version: []const u8,
    creator_name: []const u8,
    creator_version: []const u8,
    browser_name: []const u8,
    browser_version: []const u8,
    entries_slice: []const u8,
}

/// W3C HAR 1.2 单次请求阶段耗时（单位 ms，-1 表示未发生）
pub struct HarTimings {
    blocked_ms: f64,
    dns_ms: f64,
    connect_ms: f64,
    ssl_ms: f64,
    send_ms: f64,
    wait_ms: f64,
    receive_ms: f64,
}

/// W3C HAR 1.2 单条网络请求/响应记录（零拷贝切片）
pub struct HarEntry {
    started_date_time: []const u8,
    time_ms: f64,
    server_ip: []const u8,
    method: []const u8,
    url: []const u8,
    http_version: []const u8,
    req_body_size: i64,
    status: u16,
    status_text: []const u8,
    mime_type: []const u8,
    resp_body_size: i64,
    content_size: i64,
    timings: HarTimings,

    /// 状态码 >= 400 或 0（连接失败/中止）视为错误
    fn isError(): bool {
        return _.status >= 400 || _.status == 0;
    }

    /// 状态码 2xx 视为成功
    fn isSuccess(): bool {
        return _.status >= 200 && _.status < 300;
    }
}

/// W3C HAR 1.2 单遍性能与状态码汇总统计
pub struct HarStats {
    entry_count: u32,
    success_count: u32,
    redirect_count: u32,
    client_error_count: u32,
    server_error_count: u32,
    total_transfer_bytes: u64,
    total_time_ms: f64,
    max_time_ms: f64,
    avg_ttfb_ms: f64,
}

// ---------------------------------------------------------------------------
// 内部零拷贝 JSON 扫描与标量解析引擎（无外部依赖、无递归栈风险）
// ---------------------------------------------------------------------------

fn isWs(c: u8): bool {
    return c == ' ' || c == '\t' || c == '\r' || c == '\n';
}

fn isDigit(c: u8): bool {
    return c >= '0' && c <= '9';
}

fn eqlBytes(a: []const u8, b: []const u8): bool {
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

fn skipBom(data: []const u8): usize {
    if (data.len >= 3 && data[0] == 0xEF && data[1] == 0xBB && data[2] == 0xBF) {
        return 3;
    }
    return 0;
}

fn skipWs(data: []const u8, pos: *usize) {
    while (pos.* < data.len && isWs(data[pos.*])) {
        pos.* += 1;
    }
}

/// 解析 JSON 字符串并推进 pos 至闭合引号之后，返回引号内部零拷贝切片
fn parseJsonString(data: []const u8, pos: *usize): ([]const u8)? {
    if (pos.* >= data.len || data[pos.*] != '"') {
        return null;
    }
    const content_start = pos.* + 1;
    let i = content_start;
    while (i < data.len) {
        const c = data[i];
        if (c == '\\') {
            if (i + 1 >= data.len) {
                return null;
            }
            i += 2;
            continue;
        }
        if (c == '"') {
            pos.* = i + 1;
            return data[content_start..i];
        }
        i += 1;
    }
    return null;
}

/// 迭代跳过任意完整 JSON 值（对象/数组/字符串/数字/布尔/null），成功返回 true 并推进 pos
fn skipJsonValue(data: []const u8, pos: *usize): bool {
    skipWs(data, pos);
    if (pos.* >= data.len) {
        return false;
    }
    const c = data[pos.*];

    if (c == '"') {
        return parseJsonString(data, pos) != null;
    }

    if (c == '{' || c == '[') {
        let depth: u32 = 0;
        let i = pos.*;
        while (i < data.len) {
            const ch = data[i];
            if (ch == '"') {
                let s_pos = i;
                if (parseJsonString(data, *s_pos) == null) {
                    return false;
                }
                i = s_pos;
                continue;
            }
            if (ch == '{' || ch == '[') {
                depth += 1;
            } else if (ch == '}' || ch == ']') {
                if (depth == 0) {
                    return false;
                }
                depth -= 1;
                if (depth == 0) {
                    pos.* = i + 1;
                    return true;
                }
            }
            i += 1;
        }
        return false;
    }

    // 数字 / true / false / null
    const start = pos.*;
    while (pos.* < data.len) {
        const ch = data[pos.*];
        if (isWs(ch) || ch == ',' || ch == '}' || ch == ']') {
            break;
        }
        pos.* += 1;
    }
    return pos.* > start;
}

/// 在 JSON 对象切片顶层查找指定键名对应的原始值切片
fn findObjectField(obj_slice: []const u8, target_key: []const u8): ([]const u8)? {
    let pos: usize = skipBom(obj_slice);
    skipWs(obj_slice, *pos);
    if (pos >= obj_slice.len || obj_slice[pos] != '{') {
        return null;
    }
    pos += 1;

    while (pos < obj_slice.len) {
        skipWs(obj_slice, *pos);
        if (pos >= obj_slice.len) {
            return null;
        }
        if (obj_slice[pos] == '}') {
            return null;
        }
        if (obj_slice[pos] == ',') {
            pos += 1;
            continue;
        }

        const key = parseJsonString(obj_slice, *pos) ?? return null;
        skipWs(obj_slice, *pos);
        if (pos >= obj_slice.len || obj_slice[pos] != ':') {
            return null;
        }
        pos += 1;
        skipWs(obj_slice, *pos);

        const val_start = pos;
        if (!skipJsonValue(obj_slice, *pos)) {
            return null;
        }
        const val_end = pos;

        if (eqlBytes(key, target_key)) {
            return obj_slice[val_start..val_end];
        }
    }
    return null;
}

fn extractString(val_slice: []const u8): ([]const u8)? {
    let pos: usize = 0;
    skipWs(val_slice, *pos);
    return parseJsonString(val_slice, *pos);
}

fn parseF64Strict(raw: []const u8): f64? {
    let i: usize = 0;
    while (i < raw.len && isWs(raw[i])) {
        i += 1;
    }
    let end = raw.len;
    while (end > i && isWs(raw[end - 1])) {
        end -= 1;
    }
    const s = raw[i..end];
    if (s.len == 0) {
        return null;
    }

    let p: usize = 0;
    let neg = false;
    if (s[0] == '-') {
        neg = true;
        p = 1;
    } else if (s[0] == '+') {
        p = 1;
    }
    if (p >= s.len) {
        return null;
    }

    let val: f64 = 0.0;
    let digits: u32 = 0;
    while (p < s.len && isDigit(s[p])) {
        val = val * 10.0 + f64(s[p] - '0');
        digits += 1;
        p += 1;
    }
    if (p < s.len && s[p] == '.') {
        p += 1;
        let frac: f64 = 0.1;
        while (p < s.len && isDigit(s[p])) {
            val += f64(s[p] - '0') * frac;
            frac *= 0.1;
            digits += 1;
            p += 1;
        }
    }
    if (digits == 0) {
        return null;
    }
    if (p < s.len && (s[p] == 'e' || s[p] == 'E')) {
        p += 1;
        let exp_neg = false;
        if (p < s.len && s[p] == '-') {
            exp_neg = true;
            p += 1;
        } else if (p < s.len && s[p] == '+') {
            p += 1;
        }
        if (p >= s.len || !isDigit(s[p])) {
            return null;
        }
        let exp_val: i32 = 0;
        while (p < s.len && isDigit(s[p])) {
            if (exp_val < 1000) {
                exp_val = exp_val * 10 + i32(s[p] - '0');
            }
            p += 1;
        }
        if (exp_val > 308) {
            return null;
        }
        let scale: f64 = 1.0;
        let k: i32 = 0;
        while (k < exp_val) {
            scale *= 10.0;
            k += 1;
        }
        if (exp_neg) {
            val /= scale;
        } else {
            val *= scale;
        }
    }
    if (p != s.len) {
        return null;
    }
    return if (neg) -val else val;
}

fn parseI64Strict(raw: []const u8): i64? {
    let i: usize = 0;
    while (i < raw.len && isWs(raw[i])) {
        i += 1;
    }
    let end = raw.len;
    while (end > i && isWs(raw[end - 1])) {
        end -= 1;
    }
    const s = raw[i..end];
    if (s.len == 0) {
        return null;
    }
    let p: usize = 0;
    let neg = false;
    if (s[0] == '-') {
        neg = true;
        p = 1;
    } else if (s[0] == '+') {
        p = 1;
    }
    if (p >= s.len) {
        return null;
    }
    let acc: u64 = 0;
    while (p < s.len) {
        if (!isDigit(s[p])) {
            return null;
        }
        acc = acc * 10 + u64(s[p] - '0');
        if (acc > 9223372036854775807) {
            return null;
        }
        p += 1;
    }
    const signed = acc as i64;
    return if (neg) -signed else signed;
}

fn getStrField(obj: []const u8, key: []const u8): []const u8 {
    if (v := findObjectField(obj, key)) {
        if (s := extractString(v)) {
            return s;
        }
    }
    return obj[0..0];
}

fn getF64Field(obj: []const u8, key: []const u8, default_val: f64): f64 {
    if (v := findObjectField(obj, key)) {
        if (f := parseF64Strict(v)) {
            return f;
        }
    }
    return default_val;
}

fn getI64Field(obj: []const u8, key: []const u8, default_val: i64): i64 {
    if (v := findObjectField(obj, key)) {
        if (n := parseI64Strict(v)) {
            return n;
        }
    }
    return default_val;
}

fn parseTimingsObj(obj: []const u8): HarTimings {
    return HarTimings{
        blocked_ms: getF64Field(obj, "blocked", -1.0),
        dns_ms: getF64Field(obj, "dns", -1.0),
        connect_ms: getF64Field(obj, "connect", -1.0),
        ssl_ms: getF64Field(obj, "ssl", -1.0),
        send_ms: getF64Field(obj, "send", -1.0),
        wait_ms: getF64Field(obj, "wait", -1.0),
        receive_ms: getF64Field(obj, "receive", -1.0),
    };
}

fn parseEntryObj(entry_obj: []const u8): HarEntry {
    const empty = entry_obj[0..0];
    let entry = HarEntry{
        started_date_time: getStrField(entry_obj, "startedDateTime"),
        time_ms: getF64Field(entry_obj, "time", 0.0),
        server_ip: getStrField(entry_obj, "serverIPAddress"),
        method: empty,
        url: empty,
        http_version: empty,
        req_body_size: -1,
        status: 0,
        status_text: empty,
        mime_type: empty,
        resp_body_size: -1,
        content_size: -1,
        timings: HarTimings{
            blocked_ms: -1.0,
            dns_ms: -1.0,
            connect_ms: -1.0,
            ssl_ms: -1.0,
            send_ms: -1.0,
            wait_ms: -1.0,
            receive_ms: -1.0,
        },
    };

    if (req_obj := findObjectField(entry_obj, "request")) {
        entry.method = getStrField(req_obj, "method");
        entry.url = getStrField(req_obj, "url");
        entry.http_version = getStrField(req_obj, "httpVersion");
        entry.req_body_size = getI64Field(req_obj, "bodySize", -1);
    }

    if (resp_obj := findObjectField(entry_obj, "response")) {
        const st = getI64Field(resp_obj, "status", 0);
        if (st >= 0 && st <= 65535) {
            entry.status = st as u16;
        }
        entry.status_text = getStrField(resp_obj, "statusText");
        entry.resp_body_size = getI64Field(resp_obj, "bodySize", -1);
        if (content_obj := findObjectField(resp_obj, "content")) {
            entry.mime_type = getStrField(content_obj, "mimeType");
            entry.content_size = getI64Field(content_obj, "size", -1);
        }
    }

    if (tim_obj := findObjectField(entry_obj, "timings")) {
        entry.timings = parseTimingsObj(tim_obj);
    }

    return entry;
}

// ---------------------------------------------------------------------------
// 公开 API
// ---------------------------------------------------------------------------

/// 解析 HAR 1.2 顶层 log 头部元数据与零拷贝 entries 数组切片
pub fn parseHarHeader(data: []const u8): HarHeader? {
    const log_obj = findObjectField(data, "log") ?? return null;
    const has_ver = findObjectField(log_obj, "version") != null;
    const entries_val = findObjectField(log_obj, "entries");
    if (!has_ver && entries_val == null) {
        return null;
    }

    const empty = data[0..0];
    let hdr = HarHeader{
        version: getStrField(log_obj, "version"),
        creator_name: empty,
        creator_version: empty,
        browser_name: empty,
        browser_version: empty,
        entries_slice: empty,
    };

    if (creator_obj := findObjectField(log_obj, "creator")) {
        hdr.creator_name = getStrField(creator_obj, "name");
        hdr.creator_version = getStrField(creator_obj, "version");
    }
    if (browser_obj := findObjectField(log_obj, "browser")) {
        hdr.browser_name = getStrField(browser_obj, "name");
        hdr.browser_version = getStrField(browser_obj, "version");
    }
    if (ev := entries_val) {
        if (ev.len > 0 && ev[0] == '[') {
            hdr.entries_slice = ev;
        }
    }

    return hdr;
}

/// 快速检测 JSON 数据是否为合法 W3C HAR 文档（顶层含 "log" 且内含 "version" 或 "entries"）
pub fn isHar(data: []const u8): bool {
    return parseHarHeader(data) != null;
}

/// 零分配迭代 entries_slice（或完整 HAR 文档）中的全部网络请求条目
pub struct HarEntryIter {
    data: []const u8,
    pos: usize,
    done: bool,
    malformed: bool,

    fn init(slice: []const u8): HarEntryIter {
        let arr = slice;
        let p: usize = skipBom(slice);
        skipWs(slice, *p);
        if (p < slice.len && slice[p] == '{') {
            // 兼容直接传入完整 HAR 文档
            if (hdr := parseHarHeader(slice)) {
                arr = hdr.entries_slice;
                p = 0;
            } else {
                return HarEntryIter{
                    data: slice,
                    pos: slice.len,
                    done: true,
                    malformed: true,
                };
            }
        }
        skipWs(arr, *p);
        if (p >= arr.len || arr[p] != '[') {
            return HarEntryIter{
                data: arr,
                pos: arr.len,
                done: true,
                malformed: arr.len > 0,
            };
        }
        return HarEntryIter{
            data: arr,
            pos: p + 1,
            done: false,
            malformed: false,
        };
    }

    fn next(*_): HarEntry? {
        if (_.done) {
            return null;
        }
        while (_.pos < _.data.len) {
            skipWs(_.data, *_.pos);
            if (_.pos >= _.data.len) {
                _.done = true;
                _.malformed = true;
                return null;
            }
            const c = _.data[_.pos];
            if (c == ']') {
                _.done = true;
                return null;
            }
            if (c == ',') {
                _.pos += 1;
                continue;
            }
            if (c != '{') {
                _.done = true;
                _.malformed = true;
                return null;
            }
            const obj_start = _.pos;
            if (!skipJsonValue(_.data, *_.pos)) {
                _.done = true;
                _.malformed = true;
                return null;
            }
            return parseEntryObj(_.data[obj_start.._.pos]);
        }
        _.done = true;
        return null;
    }
}

/// 单遍统计 HAR 网络抓包的性能指标与状态码分布
pub fn summarizeHar(data: []const u8): HarStats {
    let stats = HarStats{
        entry_count: 0,
        success_count: 0,
        redirect_count: 0,
        client_error_count: 0,
        server_error_count: 0,
        total_transfer_bytes: 0,
        total_time_ms: 0.0,
        max_time_ms: 0.0,
        avg_ttfb_ms: 0.0,
    };

    const hdr = parseHarHeader(data) ?? return stats;
    if (hdr.entries_slice.len == 0) {
        return stats;
    }

    let ttfb_sum: f64 = 0.0;
    let ttfb_count: u32 = 0;

    let it = HarEntryIter.init(hdr.entries_slice);
    while (e := it.next()) {
        stats.entry_count += 1;
        if (e.status >= 200 && e.status < 300) {
            stats.success_count += 1;
        } else if (e.status >= 300 && e.status < 400) {
            stats.redirect_count += 1;
        } else if (e.status >= 400 && e.status < 500) {
            stats.client_error_count += 1;
        } else if (e.status >= 500) {
            stats.server_error_count += 1;
        }

        if (e.resp_body_size > 0) {
            stats.total_transfer_bytes += e.resp_body_size as u64;
        } else if (e.content_size > 0) {
            stats.total_transfer_bytes += e.content_size as u64;
        }

        if (e.time_ms > 0.0) {
            stats.total_time_ms += e.time_ms;
            if (e.time_ms > stats.max_time_ms) {
                stats.max_time_ms = e.time_ms;
            }
        }

        if (e.timings.wait_ms >= 0.0) {
            ttfb_sum += e.timings.wait_ms;
            ttfb_count += 1;
        }
    }

    if (ttfb_count > 0) {
        stats.avg_ttfb_ms = ttfb_sum / f64(ttfb_count);
    }

    return stats;
}
