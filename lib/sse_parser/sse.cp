// Source: Absorbed from /data/dev/bun/src/http/HTTPRequestBody.cp (WHATWG Server-Sent Events)

/// 单个 Server-Sent Events (`text/event-stream`) 事件对象
pub struct SseEvent {
    event_type: []const u8 = "message",
    data: []const u8 = "",
    id: []const u8 = "",
    has_id: bool = false,
    retry_ms: u64? = null,
}

fn eqlBytes(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) { return false; }
    let i: usize = 0;
    while (i < a.len) {
        if (a[i] != b[i]) { return false; }
        i += 1;
    }
    return true;
}

fn containsNul(s: []const u8): bool {
    let i: usize = 0;
    while (i < s.len) {
        if (s[i] == 0) { return true; }
        i += 1;
    }
    return false;
}

fn parseAsciiU64(s: []const u8): u64? {
    if (s.len == 0) { return null; }
    let acc: u64 = 0;
    let i: usize = 0;
    while (i < s.len) {
        const c = s[i];
        if (c < '0' || c > '9') { return null; }
        const d: u64 = @as(u64, c - '0');
        if (acc > ((@as(u64, 18446744073709551615) - d) / 10)) { return null; }
        acc = acc * 10 + d;
        i += 1;
    }
    return acc;
}

/// 零堆分配 WHATWG Server-Sent Events (`text/event-stream`) 流式解析器
pub struct SseParser {
    src: []const u8,
    pos: usize = 0,
    last_event_id: []const u8 = "",
    data_buf: [4096]u8 = [0; 4096],

    /// 初始化解析器，自动剥离可选 UTF-8 BOM
    static fn init(src: []const u8): SseParser {
        let start_pos: usize = 0;
        if (src.len >= 3 && src[0] == 0xEF && src[1] == 0xBB && src[2] == 0xBF) {
            start_pos = 3;
        }
        return SseParser{
            src: src,
            pos: start_pos,
        };
    }

    /// 返回当前流中最近一次合法 `id:` 字段记录的 Last-Event-ID
    fn lastEventId(): []const u8 {
        return _.last_event_id;
    }

    fn readLine(*_): ([]const u8)? {
        if (_.pos >= _.src.len) { return null; }
        const start = _.pos;
        while (_.pos < _.src.len && _.src[_.pos] != '\n' && _.src[_.pos] != '\r') {
            _.pos += 1;
        }
        const end = _.pos;
        if (_.pos < _.src.len) {
            if (_.src[_.pos] == '\r') {
                _.pos += 1;
                if (_.pos < _.src.len && _.src[_.pos] == '\n') {
                    _.pos += 1;
                }
            } else if (_.src[_.pos] == '\n') {
                _.pos += 1;
            }
        }
        return _.src[start..end];
    }

    /// 读取并派发下一个完整 SSE 事件（注意：返回事件中的 `data` 切片指向内部缓冲区，在下一次 `next()` 前有效）
    fn next(*_): SseEvent? {
        let event_type: []const u8 = "message";
        let data_len: usize = 0;
        let has_data = false;
        let has_id = false;
        let retry_ms: u64? = null;

        while (line := _.readLine()) {
            // 空行触发事件派发（仅当包含 data: 字段时派发）
            if (line.len == 0) {
                if (has_data) {
                    const final_len = if (data_len > 0 && _.data_buf[data_len - 1] == '\n') data_len - 1 else data_len;
                    return SseEvent{
                        event_type: event_type,
                        data: _.data_buf[0..final_len],
                        id: _.last_event_id,
                        has_id: has_id || _.last_event_id.len > 0,
                        retry_ms: retry_ms,
                    };
                }
                // 无 data 的块重置临时字段并继续（last_event_id 持久保留）
                event_type = "message";
                data_len = 0;
                has_id = false;
                retry_ms = null;
                continue;
            }

            // 冒号开头的注释行直接忽略
            if (line[0] == ':') {
                continue;
            }

            // 拆分 field: value（去除冒号后最多 1 个前导空格）
            let colon_idx: usize? = null;
            let k: usize = 0;
            while (k < line.len) {
                if (line[k] == ':') {
                    colon_idx = k;
                    break;
                }
                k += 1;
            }

            let field: []const u8 = line;
            let val: []const u8 = line[0..0];
            if (ci := colon_idx) {
                field = line[0..ci];
                const raw_val = line[(ci + 1)..line.len];
                if (raw_val.len > 0 && raw_val[0] == ' ') {
                    val = raw_val[1..raw_val.len];
                } else {
                    val = raw_val;
                }
            }

            if (eqlBytes(field, "data")) {
                has_data = true;
                let j: usize = 0;
                while (j < val.len && data_len < 4096) {
                    _.data_buf[data_len] = val[j];
                    data_len += 1;
                    j += 1;
                }
                if (data_len < 4096) {
                    _.data_buf[data_len] = '\n';
                    data_len += 1;
                }
            } else if (eqlBytes(field, "event")) {
                event_type = if (val.len > 0) val else "message";
            } else if (eqlBytes(field, "id")) {
                if (!containsNul(val)) {
                    _.last_event_id = val;
                    has_id = true;
                }
            } else if (eqlBytes(field, "retry")) {
                if (ms := parseAsciiU64(val)) {
                    retry_ms = ms;
                }
            }
        }

        return null;
    }
}

struct OutWriter {
    buf: []u8,
    pos: usize = 0,
    overflow: bool = false,

    fn writeByte(*_, c: u8) {
        if (_.pos < _.buf.len) {
            _.buf[_.pos] = c;
            _.pos += 1;
        } else {
            _.overflow = true;
        }
    }

    fn writeStr(*_, s: []const u8) {
        let i: usize = 0;
        while (i < s.len) {
            _.writeByte(s[i]);
            i += 1;
        }
    }

    fn writeU64(*_, val: u64) {
        let digits: [24]u8 = [0; 24];
        let dlen: usize = 0;
        let tmp = val;
        if (tmp == 0) {
            _.writeByte('0');
            return;
        }
        while (tmp > 0 && dlen < 24) {
            digits[dlen] = @intCast((tmp % 10) + '0');
            tmp /= 10;
            dlen += 1;
        }
        let k = dlen;
        while (k > 0) {
            k -= 1;
            _.writeByte(digits[k]);
        }
    }

    fn finish(): ([]const u8)? {
        if (_.overflow) { return null; }
        return _.buf[0.._.pos];
    }
}

/// 将 `SseEvent` 格式化为标准 `text/event-stream` 帧写入 `out`
/// 自动将含 `\n` 的 `data` 拆分为多条 `data: ...` 行并以 `\n\n` 结尾
pub fn formatSseEvent(ev: SseEvent, out: []u8): ([]const u8)? {
    let w = OutWriter{ buf: out };

    if (ev.event_type.len > 0 && !eqlBytes(ev.event_type, "message")) {
        w.writeStr("event: ");
        w.writeStr(ev.event_type);
        w.writeByte('\n');
    }
    if (ev.has_id) {
        w.writeStr("id: ");
        w.writeStr(ev.id);
        w.writeByte('\n');
    }
    if (ms := ev.retry_ms) {
        w.writeStr("retry: ");
        w.writeU64(ms);
        w.writeByte('\n');
    }

    if (ev.data.len == 0) {
        w.writeStr("data: \n");
    } else {
        let i: usize = 0;
        while (i <= ev.data.len) {
            const start = i;
            while (i < ev.data.len && ev.data[i] != '\n') {
                i += 1;
            }
            let end = i;
            if (end > start && ev.data[end - 1] == '\r') {
                end -= 1;
            }
            w.writeStr("data: ");
            w.writeStr(ev.data[start..end]);
            w.writeByte('\n');
            if (i == ev.data.len) { break; }
            i += 1;
        }
    }

    w.writeByte('\n');
    return w.finish();
}
