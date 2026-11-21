// Source: Absorbed from /data/dev/bun/src/ (JSONL / NDJSON Streaming Line Framer)

fn isWsNoNewline(c: u8): bool {
    return c == ' ' || c == '\t' || c == '\r';
}

fn isWs(c: u8): bool {
    return c == ' ' || c == '\t' || c == '\r' || c == '\n';
}

fn trimWs(s: []const u8): []const u8 {
    let start: usize = 0;
    let end: usize = s.len;
    while (start < end && isWs(s[start])) {
        start += 1;
    }
    while (end > start && isWs(s[end - 1])) {
        end -= 1;
    }
    return s[start..end];
}

fn isBlankLine(s: []const u8): bool {
    let i: usize = 0;
    while (i < s.len) {
        if (!isWsNoNewline(s[i])) {
            return false;
        }
        i += 1;
    }
    return true;
}

fn stripUtf8Bom(s: []const u8): []const u8 {
    if (s.len >= 3 && s[0] == 0xEF && s[1] == 0xBB && s[2] == 0xBF) {
        return s[3..s.len];
    }
    return s;
}

/// 增量分块扫描结果（用于网络流/文件流 JSONL 拼接）
export struct JsonlChunkResult {
    consumed_bytes: usize;
    line_count: usize;
    has_incomplete_tail: bool;
}

/// JSONL / NDJSON 零分配流式行迭代器（自动跳过 UTF-8 BOM 与空白行，支持 `\n` / `\r\n`）
export struct JsonlIter {
    src: []const u8;
    pos: usize;

    fn init(input: []const u8): JsonlIter {
        let start: usize = 0;
        if (input.len >= 3 && input[0] == 0xEF && input[1] == 0xBB && input[2] == 0xBF) {
            start = 3;
        }
        return JsonlIter{
            src: input,
            pos: start,
        };
    }

    fn next(*_): ([]const u8)? {
        while (_.pos < _.src.len) {
            const line_start = _.pos;
            while (_.pos < _.src.len && _.src[_.pos] != '\n') {
                _.pos += 1;
            }
            let line_end = _.pos;
            if (_.pos < _.src.len && _.src[_.pos] == '\n') {
                _.pos += 1;
            }
            if (line_end > line_start && _.src[line_end - 1] == '\r') {
                line_end -= 1;
            }
            const line = _.src[line_start..line_end];
            if (!isBlankLine(line)) {
                return line;
            }
        }
        return null;
    }
}

/// 增量分块扫描器：仅统计以 `\n` 结尾的完整非空记录数与已消费字节偏移，
/// 并标记末尾是否存在未完结半行（调用方可用 `chunk[res.consumed_bytes..]` 保留残余半行）
export fn scanCompleteJsonlChunk(chunk: []const u8): JsonlChunkResult {
    let consumed: usize = 0;
    let line_count: usize = 0;
    let line_start: usize = 0;
    let is_first = true;

    let i: usize = 0;
    while (i < chunk.len) {
        if (chunk[i] == '\n') {
            let line_end = i;
            if (line_end > line_start && chunk[line_end - 1] == '\r') {
                line_end -= 1;
            }
            let line = chunk[line_start..line_end];
            if (is_first) {
                line = stripUtf8Bom(line);
                is_first = false;
            }
            if (!isBlankLine(line)) {
                line_count += 1;
            }
            consumed = i + 1;
            line_start = i + 1;
        }
        i += 1;
    }

    return JsonlChunkResult{
        consumed_bytes: consumed,
        line_count: line_count,
        has_incomplete_tail: consumed < chunk.len,
    };
}

/// 零分配校验单行 JSON 记录的括号/引号嵌套平衡与非空性，
/// 拦截未闭合字符串、截断对象/数组、嵌套错配、粘连多值及非法控制字符
export fn isBalancedJsonRecord(line: []const u8): bool {
    const trimmed = trimWs(stripUtf8Bom(line));
    if (trimmed.len == 0) {
        return false;
    }

    const first = trimmed[0];
    const is_container = first == '{' || first == '[';
    const is_str = first == '"';
    const is_primitive = first == 't' || first == 'f' || first == 'n'
        || first == '-' || (first >= '0' && first <= '9');

    if (!is_container && !is_str && !is_primitive) {
        return false;
    }

    let stack: [64]u8 = [
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    ];
    let depth: usize = 0;
    let in_string = false;
    let escaped = false;
    let root_closed = false;

    let i: usize = 0;
    while (i < trimmed.len) {
        const c = trimmed[i];
        i += 1;

        if (c == '\r' || c == '\n') {
            return false;
        }

        if (in_string) {
            if (escaped) {
                escaped = false;
            } else if (c == '\\') {
                escaped = true;
            } else if (c == '"') {
                in_string = false;
                if (is_str && depth == 0) {
                    root_closed = true;
                }
            } else if (c < 0x20) {
                return false;
            }
            continue;
        }

        if (root_closed) {
            if (!isWsNoNewline(c)) {
                return false;
            }
            continue;
        }

        if (c == '"') {
            if (is_primitive) {
                return false;
            }
            in_string = true;
        } else if (c == '{') {
            if (is_primitive || depth >= 64) {
                return false;
            }
            stack[depth] = '}';
            depth += 1;
        } else if (c == '[') {
            if (is_primitive || depth >= 64) {
                return false;
            }
            stack[depth] = ']';
            depth += 1;
        } else if (c == '}' || c == ']') {
            if (depth == 0 || stack[depth - 1] != c) {
                return false;
            }
            depth -= 1;
            if (is_container && depth == 0) {
                root_closed = true;
            }
        }
    }

    if (in_string || escaped || depth != 0) {
        return false;
    }
    if (is_container || is_str) {
        return root_closed;
    }
    return true;
}

/// 校验记录非空且不含裸 `\r`/`\n`，并将 `record + "\n"` 追加写入输出缓冲 `out[pos.*..]`
export fn appendJsonlRecord(record: []const u8, out: []u8, pos: *usize): bool {
    if (record.len == 0) {
        return false;
    }
    if (pos.* > out.len || (out.len - pos.*) < record.len + 1) {
        return false;
    }

    let i: usize = 0;
    while (i < record.len) {
        if (record[i] == '\r' || record[i] == '\n') {
            return false;
        }
        i += 1;
    }

    i = 0;
    while (i < record.len) {
        out[pos.* + i] = record[i];
        i += 1;
    }
    out[pos.* + record.len] = '\n';
    pos.* += record.len + 1;
    return true;
}
